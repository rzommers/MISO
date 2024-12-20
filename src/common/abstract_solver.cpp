#include "default_options.hpp"
#include "mfem_extensions.hpp"
#include "utils.hpp"

#include "abstract_solver.hpp"

namespace
{
void logState(miso::DataLogger &logger,
              const mfem::Vector &state,
              std::string fieldname,
              int timestep,
              double time,
              int rank)
{
   std::visit([&](auto &&log)
              { log.saveState(state, fieldname, timestep, time, rank); },
              logger);
}

}  // namespace

namespace miso
{
AbstractSolver2::AbstractSolver2(MPI_Comm incomm,
                                 const nlohmann::json &solver_options)
 : diff_stack(getDiffStack())
{
   /// Set the options; the defaults are overwritten by the values in the file
   /// using the merge_patch method
   options = default_options;
   options.merge_patch(solver_options);

   MPI_Comm_dup(incomm, &comm);
   MPI_Comm_rank(comm, &rank);

   bool silent = options.value("silent", false);
   out = getOutStream(rank, silent);
   if (options["print-options"])
   {
      *out << std::setw(3) << options << std::endl;
   }
}

void AbstractSolver2::setState_(std::any function,
                                const std::string &name,
                                mfem::Vector &state)
{
   useAny(function,
          [&](std::function<void(mfem::Vector &)> &fun) { fun(state); });
}

double AbstractSolver2::calcStateError_(std::any ex_sol,
                                        const std::string &name,
                                        const mfem::Vector &state)
{
   return useAny(
       ex_sol,
       [&](std::function<void(mfem::Vector &)> &fun)
       {
          work.SetSize(state.Size());
          fun(work);
          subtract(work, state, work);
          return work.Norml2();
       },
       [&](mfem::Vector &vec)
       {
          if (vec.Size() != state.Size())
          {
             throw MISOException(
                 "Input vector for exact solution is not "
                 "the same size as the "
                 "state vector!");
          }
          work.SetSize(state.Size());
          subtract(vec, state, work);
          return work.Norml2();
       });
}

void AbstractSolver2::solveForState(const MISOInputs &inputs,
                                    mfem::Vector &state)
{
   if (spatial_res)
   {
      setInputs(*spatial_res, inputs);
   }

   /// if solving an unsteady problem
   if (ode)
   {
      auto ode_opts = options["time-dis"];
      double t = ode_opts["t-initial"].get<double>();
      auto t_final = ode_opts["t-final"].get<double>();
      *out << "t_final is " << t_final << '\n';
      int ti = 0;
      double dt = 0.0;
      initialHook(state);
      for (ti = 0; ti < ode_opts["max-iter"].get<int>(); ++ti)
      {  
         dt = calcStepSize(ti, t, t_final, dt, state);
         *out << "iter " << ti << ": time = " << t << ": dt = " << dt;
         if (!ode_opts["steady"].get<bool>())
         {
            *out << " (" << round(100 * t / t_final) << "% complete)";
         }
         *out << std::endl;
         iterationHook(ti, t, dt, state);
         ode->step(state, t, dt);
         if (iterationExit(ti, t, t_final, dt, state))
         {
            break;
         }
      }
      terminalHook(ti, t, state);
   }
   else  /// steady problem, use Newton on spatial residual directly
   {
      initialHook(state);

      /// use input state as initial guess
      nonlinear_solver->iterative_mode = true;

      mfem::Vector zero;
      nonlinear_solver->Mult(zero, state);

      /// log final state
      for (auto &pair : loggers)
      {
         auto &logger = pair.first;
         logState(logger, state, "state", 1, 1.0, rank);
      }
   }
}

void AbstractSolver2::solveForAdjoint(const MISOInputs &inputs,
                                      const mfem::Vector &state_bar,
                                      mfem::Vector &adjoint)
{
   if (spatial_res)
   {
      setInputs(*spatial_res, inputs);
   }

   /// if solving an unsteady problem
   if (ode)
   {
      throw MISOException(
          "AbstractSolver2::solveForAdjoint not implemented for unsteady "
          "problems!\n");
   }
   else  /// steady problem
   {
      /// Create adjoint linear solver if we have not already
      if (!adj_solver)
      {
         auto *prec = getPreconditioner(*spatial_res);
         adj_solver = constructLinearSolver(comm, options["adj-solver"], prec);
      }

      work.SetSize(state_bar.Size());
      work = state_bar;
      setUpAdjointSystem(*spatial_res, *adj_solver, inputs, work, adjoint);

      adj_solver->Mult(work, adjoint);

      /// log final state
      for (auto &pair : loggers)
      {
         auto &logger = pair.first;
         logState(logger, adjoint, "adjoint", 0, 0.0, rank);
      }
   }
}

void AbstractSolver2::calcResidual(const mfem::Vector &state,
                                   mfem::Vector &residual) const
{
   MISOInputs inputs{{"state", state}};
   calcResidual(inputs, residual);
}

void AbstractSolver2::calcResidual(const MISOInputs &inputs,
                                   mfem::Vector &residual) const
{
   setInputs(*spatial_res, inputs);
   evaluate(*spatial_res, inputs, residual);
}

double AbstractSolver2::calcResidualNorm(const mfem::Vector &state) const
{
   MISOInputs inputs{{"state", state}};
   return calcResidualNorm(inputs);
}

double AbstractSolver2::calcResidualNorm(const MISOInputs &inputs) const
{
   work.SetSize(getSize(*spatial_res));
   calcResidual(inputs, work);
   return sqrt(InnerProduct(comm, work, work));
}

int AbstractSolver2::getStateSize() const
{
   if (spatial_res)
   {
      return getSize(*spatial_res);
   }
   else if (space_time_res)
   {
      return getSize(*space_time_res);
   }
   else
   {
      throw MISOException(
          "getStateSize(): residual not defined! State size unknown.\n");
   }
}

int AbstractSolver2::getFieldSize(const std::string &name) const
{
   if (name == "state" || name == "residual" || name == "adjoint")
   {
      return getStateSize();
   }
   // may be better way to return something not found without throwing error
   return 0;
}

void AbstractSolver2::createOutput(const std::string &output)
{
   nlohmann::json options;
   createOutput(output, options);
}

void AbstractSolver2::createOutput(const std::string &output,
                                   const nlohmann::json &options)
{
   if (outputs.count(output) == 0)
   {
      addOutput(output, options);
   }
   else
   {
      throw MISOException("Output with name " + output + " already created!\n");
   }
}

int AbstractSolver2::getOutputSize(const std::string &output)
{
   try
   {
      auto output_iter = outputs.find(output);
      if (output_iter == outputs.end())
      {
         throw MISOException("Did not find " + output + " in output map!\n");
      }
      return miso::getSize(output_iter->second);
   }
   catch (const std::out_of_range &exception)
   {
      std::cerr << exception.what() << std::endl;
   }
   return -1;
}

void AbstractSolver2::setOutputOptions(const std::string &output,
                                       const nlohmann::json &options)
{
   try
   {
      auto output_iter = outputs.find(output);
      if (output_iter == outputs.end())
      {
         throw MISOException("Did not find " + output + " in output map!\n");
      }
      miso::setOptions(output_iter->second, options);
   }
   catch (const std::out_of_range &exception)
   {
      std::cerr << exception.what() << std::endl;
   }
}

double AbstractSolver2::calcOutput(const std::string &output,
                                   const MISOInputs &inputs)
{
   try
   {
      auto output_iter = outputs.find(output);
      if (output_iter == outputs.end())
      {
         throw MISOException("Did not find " + output + " in output map!\n");
      }
      setInputs(output_iter->second, inputs);
      return miso::calcOutput(output_iter->second, inputs);
   }
   catch (const std::out_of_range &exception)
   {
      std::cerr << exception.what() << std::endl;
      return std::nan("");
   }
}

void AbstractSolver2::calcOutput(const std::string &output,
                                 const MISOInputs &inputs,
                                 mfem::Vector &out_vec)
{
   try
   {
      auto output_iter = outputs.find(output);
      if (output_iter == outputs.end())
      {
         throw MISOException("Did not find " + output + " in output map!\n");
      }
      setInputs(output_iter->second, inputs);
      if (out_vec.Size() == 1)
      {
         out_vec(0) = miso::calcOutput(output_iter->second, inputs);
      }
      else
      {
         miso::calcOutput(output_iter->second, inputs, out_vec);
      }
   }
   catch (const std::out_of_range &exception)
   {
      std::cerr << exception.what() << std::endl;
   }
}

void AbstractSolver2::calcOutputPartial(const std::string &of,
                                        const std::string &wrt,
                                        const MISOInputs &inputs,
                                        double &partial)
{
   try
   {
      auto output_iter = outputs.find(of);
      if (output_iter == outputs.end())
      {
         throw MISOException("Did not find " + of + " in output map!\n");
      }
      setInputs(output_iter->second, inputs);
      double part = miso::calcOutputPartial(output_iter->second, wrt, inputs);
      partial += part;
   }
   catch (const std::out_of_range &exception)
   {
      std::cerr << exception.what() << std::endl;
      partial = std::nan("");
   }
}

void AbstractSolver2::calcOutputPartial(const std::string &of,
                                        const std::string &wrt,
                                        const MISOInputs &inputs,
                                        mfem::Vector &partial)
{
   try
   {
      auto output_iter = outputs.find(of);
      if (output_iter == outputs.end())
      {
         throw MISOException("Did not find " + of + " in output map!\n");
      }
      setInputs(output_iter->second, inputs);
      miso::calcOutputPartial(output_iter->second, wrt, inputs, partial);
   }
   catch (const std::out_of_range &exception)
   {
      std::cerr << exception.what() << std::endl;
   }
}

void AbstractSolver2::outputJacobianVectorProduct(const std::string &of,
                                                  const MISOInputs &inputs,
                                                  const mfem::Vector &wrt_dot,
                                                  const std::string &wrt,
                                                  mfem::Vector &out_dot)
{
   try
   {
      auto output_iter = outputs.find(of);
      if (output_iter == outputs.end())
      {
         throw MISOException("Did not find " + of + " in output map!\n");
      }
      auto &output = output_iter->second;
      setInputs(output, inputs);
      if (out_dot.Size() == 1)
      {
         out_dot(0) += miso::jacobianVectorProduct(output, wrt_dot, wrt);
      }
      else
      {
         miso::jacobianVectorProduct(output, wrt_dot, wrt, out_dot);
      }
   }
   catch (const std::out_of_range &exception)
   {
      std::cerr << exception.what() << std::endl;
   }
}

void AbstractSolver2::outputVectorJacobianProduct(const std::string &of,
                                                  const MISOInputs &inputs,
                                                  const mfem::Vector &out_bar,
                                                  const std::string &wrt,
                                                  mfem::Vector &wrt_bar)
{
   try
   {
      auto output_iter = outputs.find(of);
      if (output_iter == outputs.end())
      {
         throw MISOException("Did not find " + of + " in output map!\n");
      }
      auto &output = output_iter->second;
      setInputs(output, inputs);
      if (wrt_bar.Size() == 1)
      {
         wrt_bar(0) += miso::vectorJacobianProduct(output, out_bar, wrt);
      }
      else
      {
         miso::vectorJacobianProduct(output, out_bar, wrt, wrt_bar);
      }
   }
   catch (const std::out_of_range &exception)
   {
      std::cerr << exception.what() << std::endl;
   }
}

void AbstractSolver2::linearize(const MISOInputs &inputs)
{
   /// if solving an unsteady problem
   if (ode)
   {
      throw MISOException(
          "AbstractSolver2::vectorJacobianProduct not implemented for unsteady "
          "problems!\n");
   }
   else  /// steady problem
   {
      miso::linearize(*spatial_res, inputs);
   }
}

double AbstractSolver2::jacobianVectorProduct(const mfem::Vector &wrt_dot,
                                              const std::string &wrt)
{
   /// if solving an unsteady problem
   if (ode)
   {
      throw MISOException(
          "AbstractSolver2::jacobianVectorProduct not implemented for unsteady "
          "problems!\n");
   }
   else  /// steady problem
   {
      return miso::jacobianVectorProduct(*spatial_res, wrt_dot, wrt);
   }
}

void AbstractSolver2::jacobianVectorProduct(const mfem::Vector &wrt_dot,
                                            const std::string &wrt,
                                            mfem::Vector &res_dot)
{
   /// if solving an unsteady problem
   if (ode)
   {
      throw MISOException(
          "AbstractSolver2::jacobianVectorProduct not implemented for unsteady "
          "problems!\n");
   }
   else  /// steady problem
   {
      miso::jacobianVectorProduct(*spatial_res, wrt_dot, wrt, res_dot);
   }
}
double AbstractSolver2::vectorJacobianProduct(const mfem::Vector &res_bar,
                                              const std::string &wrt)
{
   /// if solving an unsteady problem
   if (ode)
   {
      throw MISOException(
          "AbstractSolver2::vectorJacobianProduct not implemented for unsteady "
          "problems!\n");
   }
   else  /// steady problem
   {
      return miso::vectorJacobianProduct(*spatial_res, res_bar, wrt);
   }
}

void AbstractSolver2::vectorJacobianProduct(const mfem::Vector &res_bar,
                                            const std::string &wrt,
                                            mfem::Vector &wrt_bar)
{
   /// if solving an unsteady problem
   if (ode)
   {
      throw MISOException(
          "AbstractSolver2::vectorJacobianProduct not implemented for unsteady "
          "problems!\n");
   }
   else  /// steady problem
   {
      miso::vectorJacobianProduct(*spatial_res, res_bar, wrt, wrt_bar);
   }
}

void AbstractSolver2::initialHook(const mfem::Vector &state)
{
   for (auto &pair : loggers)
   {
      auto &logger = pair.first;
      auto &options = pair.second;
      if (options.initial_state)
      {
         logState(logger, state, "state", 0, 0.0, rank);
      }
   }
}

void AbstractSolver2::iterationHook(int iter,
                                    double t,
                                    double dt,
                                    const mfem::Vector &state)
{
   for (auto &pair : loggers)
   {
      auto &logger = pair.first;
      auto &options = pair.second;
      if (options.each_timestep)
      {
         logState(logger, state, "state", iter, t, rank);
      }
   }
}

double AbstractSolver2::calcStepSize(int iter,
                                     double t,
                                     double t_final,
                                     double dt_old,
                                     const mfem::Vector &state) const
{
   auto dt = options["time-dis"]["dt"].get<double>();
   if (options["time-dis"].value("exact-t-final", true))
   {
      dt = std::min(dt, t_final - t);
   }
   return dt;
}

bool AbstractSolver2::iterationExit(int iter,
                                    double t,
                                    double t_final,
                                    double dt,
                                    const mfem::Vector &state) const
{
   return t >= t_final - 1e-14 * dt;
}

void AbstractSolver2::terminalHook(int iter,
                                   double t_final,
                                   const mfem::Vector &state)
{
   for (auto &pair : loggers)
   {
      auto &logger = pair.first;
      auto &options = pair.second;
      if (options.final_state)
      {
         logState(logger, state, "state", iter, t_final, rank);
      }
   }
}

}  // namespace miso
