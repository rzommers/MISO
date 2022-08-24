#include <vector>

#include "mfem.hpp"

#include "mach_input.hpp"
#include "mach_integrator.hpp"
#include "mach_nonlinearform.hpp"
#include "utils.hpp"

namespace mach
{
int getSize(const MachNonlinearForm &form)
{
   return form.nf.ParFESpace()->GetTrueVSize();
}

void setInputs(MachNonlinearForm &form, const MachInputs &inputs)
{
   for (const auto &[name, input] : inputs)
   {
      if (std::holds_alternative<InputVector>(input))
      {
         auto it = form.nf_fields.find(name);
         if (it != form.nf_fields.end())
         {
            auto &field = it->second;
            mfem::Vector field_tv;
            setVectorFromInput(input, field_tv);

            field.distributeSharedDofs(field_tv);
         }
      }
   }
   setInputs(form.integs, inputs);
}

void setOptions(MachNonlinearForm &form, const nlohmann::json &options)
{
   setOptions(form.integs, options);

   if (options.contains("bcs"))
   {
      if (options["bcs"].contains("essential"))
      {
         auto &fes = *form.nf.ParFESpace();
         form.ess_bdr.SetSize(fes.GetParMesh()->bdr_attributes.Max());
         getMFEMBoundaryArray(options["bcs"]["essential"], form.ess_bdr);
         mfem::Array<int> ess_tdof_list;
         fes.GetEssentialTrueDofs(form.ess_bdr, ess_tdof_list);
         form.nf.SetEssentialTrueDofs(ess_tdof_list);
      }
   }
}

double calcFormOutput(MachNonlinearForm &form, const MachInputs &inputs)
{
   mfem::Vector state;
   setVectorFromInputs(inputs, "state", state, false, true);
   setInputs(form.integs, inputs);
   return form.nf.GetEnergy(state);
}

void evaluate(MachNonlinearForm &form,
              const MachInputs &inputs,
              mfem::Vector &res_vec)
{
   mfem::Vector state;
   setVectorFromInputs(inputs, "state", state, false, true);
   form.nf.Mult(state, res_vec);
}

void linearize(MachNonlinearForm &form, const MachInputs &inputs)
{
   setInputs(form, inputs);
   // getJacobianTranspose also gets the regular Jacobian
   getJacobianTranspose(form, inputs, "state");
}

mfem::Operator &getJacobian(MachNonlinearForm &form,
                            const MachInputs &inputs,
                            const std::string &wrt)
{
   mfem::Vector state;
   setVectorFromInputs(inputs, "state", state, false, true);

   mfem::Array<int> ess_tdof_list(form.nf.GetEssentialTrueDofs());
   mfem::Array<int> zeros;
   // Setting our essential true dofs to zero to full Jacobian is preserved
   form.nf.SetEssentialTrueDofs(zeros);

   // get our gradient with everything preserved
   auto *hypre_jac =
       dynamic_cast<mfem::HypreParMatrix *>(&form.nf.GetGradient(state));
   form.jac.Reset(hypre_jac, false);

   // Impose boundary conditions on pGrad
   form.jac_e.Clear();
   form.jac_e.EliminateRowsCols(form.jac, ess_tdof_list);
   form.jac_e.EliminateRows(ess_tdof_list);

   // reset our essential BCs to what they used to be
   form.nf.SetEssentialTrueDofs(ess_tdof_list);

   return *form.jac;
}

mfem::Operator &getJacobianTranspose(MachNonlinearForm &form,
                                     const MachInputs &inputs,
                                     const std::string &wrt)
{
   getJacobian(form, inputs, wrt);

   auto *hypre_jac = form.jac.As<mfem::HypreParMatrix>();
   if (hypre_jac == nullptr)
   {
      throw MachException(
          "getJacobianTranspose (MachNonlinearForm) only supports "
          "Jacobian matrices assembled to a HypreParMatrix!\n");
   }

   form.jac_trans = std::unique_ptr<mfem::Operator>(hypre_jac->Transpose());
   return *form.jac_trans;
}

void setUpAdjointSystem(MachNonlinearForm &form,
                        mfem::Solver &adj_solver,
                        const MachInputs &inputs,
                        mfem::Vector &state_bar,
                        mfem::Vector &adjoint)
{
   auto &jac_trans = getJacobianTranspose(form, inputs, "state");
   adj_solver.SetOperator(jac_trans);

   const auto &ess_tdof_list = form.nf.GetEssentialTrueDofs();
   if (ess_tdof_list.Size() == 0)
   {
      return;
   }
   adj_solver.Mult(state_bar, adjoint);

   auto *hypre_jac_e = form.jac_e.As<mfem::HypreParMatrix>();
   if (hypre_jac_e == nullptr)
   {
      throw MachException(
          "setUpAdjointSystem (MachNonlinearForm) only supports "
          "Jacobian matrices assembled to a HypreParMatrix!\n");
   }
   hypre_jac_e->MultTranspose(-1.0, adjoint, 1.0, state_bar);
}

double jacobianVectorProduct(MachNonlinearForm &form,
                             const mfem::Vector &wrt_dot,
                             const std::string &wrt)
{
   if (form.fwd_scalar_sens.count(wrt) != 0)
   {
      throw NotImplementedException(
          "not implemented for scalar sensitivities!\n");
   }
   return 0.0;
}

void jacobianVectorProduct(MachNonlinearForm &form,
                           const mfem::Vector &wrt_dot,
                           const std::string &wrt,
                           mfem::Vector &res_dot)
{
   if (wrt == "state")
   {
      auto *hypre_jac = form.jac.As<mfem::HypreParMatrix>();
      if (hypre_jac == nullptr)
      {
         throw MachException(
             "getJacobianTranspose (MachNonlinearForm) only supports "
             "Jacobian matrices assembled to a HypreParMatrix!\n");
      }
      hypre_jac->Mult(1.0, wrt_dot, 1.0, res_dot);

      auto *hypre_jac_e = form.jac_e.As<mfem::HypreParMatrix>();
      if (hypre_jac_e == nullptr)
      {
         throw MachException(
             "setUpAdjointSystem (MachNonlinearForm) only supports "
             "Jacobian matrices assembled to a HypreParMatrix!\n");
      }
      hypre_jac_e->Mult(1.0, wrt_dot, 1.0, res_dot);
   }
   else if (form.fwd_sens.count(wrt) != 0)
   {
      throw NotImplementedException(
          "not implemented for vector sensitivities (except for state)!\n");

      // /// Integrators added to fwd_sens will reference wrt_dot grid func so
      // /// we update it here
      // std::string wrt_dot_name = wrt + "_dot";
      // auto &wrt_dot_field = form.nf_fields.at(wrt_dot_name);
      // wrt_dot_field.distributeSharedDofs(wrt_dot);

      // /// Integrators added to fwd_sens will also reference the state grid
      // func
      // /// so that must have been distributed before calling this function
      // form.fwd_sens.at(wrt).Assemble();
      // form.scratch.SetSize(res_dot.Size());
      // form.fwd_sens.at(wrt).ParallelAssemble(form.scratch);

      // res_dot += form.scratch;
   }
}

double vectorJacobianProduct(MachNonlinearForm &form,
                             const mfem::Vector &res_bar,
                             const std::string &wrt)
{
   if (form.rev_scalar_sens.count(wrt) != 0)
   {
      /// Integrators added to rev_scalar_sens will reference the adjoint grid
      /// func so we update it here
      auto &adjoint = form.nf_fields.at("adjoint");
      adjoint.distributeSharedDofs(res_bar);

      /// The state must have previously been distributed before calling this
      /// function
      auto &state = form.nf_fields.at("state").gridFunc();
      return form.rev_scalar_sens.at(wrt).GetGridFunctionEnergy(state);
   }
   return 0.0;
}

void vectorJacobianProduct(MachNonlinearForm &form,
                           const mfem::Vector &res_bar,
                           const std::string &wrt,
                           mfem::Vector &wrt_bar)
{
   if (wrt == "state")
   {
      auto *hypre_jac = form.jac.As<mfem::HypreParMatrix>();
      if (hypre_jac == nullptr)
      {
         throw MachException(
             "getJacobianTranspose (MachNonlinearForm) only supports "
             "Jacobian matrices assembled to a HypreParMatrix!\n");
      }
      hypre_jac->MultTranspose(1.0, res_bar, 1.0, wrt_bar);

      auto *hypre_jac_e = form.jac_e.As<mfem::HypreParMatrix>();
      if (hypre_jac_e == nullptr)
      {
         throw MachException(
             "setUpAdjointSystem (MachNonlinearForm) only supports "
             "Jacobian matrices assembled to a HypreParMatrix!\n");
      }
      hypre_jac_e->MultTranspose(1.0, res_bar, 1.0, wrt_bar);
   }
   else if (form.rev_sens.count(wrt) != 0)
   {
      form.scratch.SetSize(res_bar.Size());
      form.scratch = res_bar;
      const auto &ess_tdof_list = form.getEssentialDofs();
      form.scratch.SetSubVector(ess_tdof_list, 0.0);

      /// Integrators added to rev_sens will reference the adjoint, grid func
      /// so we update it here
      auto &adjoint = form.nf_fields.at("adjoint");
      adjoint.distributeSharedDofs(form.scratch);

      /// Integrators added to rev_sens will also reference the state grid func,
      /// so that must have been distributed before calling this function
      auto &wrt_rev_sens = form.rev_sens.at(wrt);
      wrt_rev_sens.Assemble();
      form.scratch.SetSize(wrt_bar.Size());
      form.scratch = 0.0;
      wrt_rev_sens.ParallelAssemble(form.scratch);

      // mfem::Array<int> wrt_ess_tdof_list;
      // wrt_rev_sens.ParFESpace()->GetEssentialTrueDofs(form.ess_bdr,
      // wrt_ess_tdof_list); for (int i = 0; i < wrt_ess_tdof_list.Size(); ++i)
      // {
      //    form.scratch(wrt_ess_tdof_list[i]) = 0.0;
      // }
      wrt_bar += form.scratch;
   }
}

}  // namespace mach
