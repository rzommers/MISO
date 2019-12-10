#include <fstream>
#include <iostream>
#include "solver.hpp"
#include "default_options.hpp"
#include "sbp_fe.hpp"

using namespace std;
using namespace mfem;

namespace mach
{

// Static member function diff_stack needs to be defined
adept::Stack AbstractSolver::diff_stack;

AbstractSolver::AbstractSolver(const string &opt_file_name,
                               unique_ptr<Mesh> smesh)
{
// Set the options; the defaults are overwritten by the values in the file
// using the merge_patch method
#ifdef MFEM_USE_MPI
   comm = MPI_COMM_WORLD; // TODO: how to pass as an argument?
   MPI_Comm_rank(comm, &rank);
#else
   rank = 0; // serial case
#endif
   out = getOutStream(rank);
   options = default_options;
   nlohmann::json file_options;
   ifstream options_file(opt_file_name);
   options_file >> file_options;
   options.merge_patch(file_options);
   *out << setw(3) << options << endl;
   constructMesh(move(smesh));
   // does num_dim equal mesh->Dimension in all cases?
   num_dim = mesh->Dimension();

   *out << "problem space dimension = " << num_dim << endl;

   // Define the ODE solver used for time integration (possibly not used)
   ode_solver = NULL;
   *out << "ode-solver type = "
        << options["time-dis"]["ode-solver"].get<string>() << endl;
   if (options["time-dis"]["ode-solver"].get<string>() == "RK1")
   {
      ode_solver.reset(new ForwardEulerSolver);
   }
   if (options["time-dis"]["ode-solver"].get<string>() == "RK4")
   {
      ode_solver.reset(new RK4Solver);
   }
   if (options["time-dis"]["ode-solver"].get<string>() == "MIDPOINT")
   {
      ode_solver.reset(new ImplicitMidpointSolver);
   }
   else
   {
      throw MachException("Unknown ODE solver type " +
                          options["time-dis"]["ode-solver"].get<string>());
      // TODO: parallel exit
   }

   // Refine the mesh here, or have a separate member function?
   for (int l = 0; l < options["mesh"]["refine"].get<int>(); l++)
   {
      mesh->UniformRefinement();
   }

   // Define the SBP elements and finite-element space; eventually, we will want
   // to have a case or if statement here for both CSBP and DSBP, and (?) standard FEM.
   // and here it is for first two
   if (options["space-dis"]["basis-type"].get<string>() == "csbp")
   {
      fec.reset(new SBPCollection(options["space-dis"]["degree"].get<int>(),
                                  num_dim));
   }
   else if (options["space-dis"]["basis-type"].get<string>() == "dsbp")
   {
      fec.reset(new DSBPCollection(options["space-dis"]["degree"].get<int>(),
                                   num_dim));
   }
}

AbstractSolver::~AbstractSolver()
{
   *out << "Deleting Abstract Solver..." << endl;
}

void AbstractSolver::constructMesh(unique_ptr<Mesh> smesh)
{
#ifndef MFEM_USE_PUMI
   if (smesh == nullptr)
   { // read in the serial mesh
      smesh.reset(new Mesh(options["mesh"]["file"].get<string>().c_str(), 1, 1));
   }
#endif

#ifdef MFEM_USE_MPI
   comm = MPI_COMM_WORLD; // TODO: how to pass communicator as an argument?
   MPI_Comm_rank(comm, &rank);
#ifdef MFEM_USE_PUMI // if using pumi mesh
   if (smesh != nullptr)
   {
      throw MachException("AbstractSolver::constructMesh(smesh)\n"
                          "\tdo not provide smesh when using PUMI!");
   }
   // problem with using these in loadMdsMesh
   std::cout << options["model-file"].get<string>().c_str() << std::endl;
   const char *model_file = options["model-file"].get<string>().c_str();
   const char *mesh_file = options["mesh"]["file"].get<string>().c_str();
   PCU_Comm_Init();
#ifdef MFEM_USE_SIMMETRIX
   Sim_readLicenseFile(0);
   gmi_sim_start();
   gmi_register_sim();
#endif
   gmi_register_mesh();
   pumi_mesh = apf::loadMdsMesh(options["model-file"].get<string>().c_str(),
                                options["mesh"]["file"].get<string>().c_str());
   int dim = pumi_mesh->getDimension();
   int nEle = pumi_mesh->count(dim);
   int ref_levels = (int)floor(log(10000. / nEle) / log(2.) / dim);
   // Perform Uniform refinement
   // if (ref_levels > 1)
   // {
   //    ma::Input* uniInput = ma::configureUniformRefine(pumi_mesh, ref_levels);
   //    ma::adapt(uniInput);
   // }
   pumi_mesh->verify();
   mesh.reset(new MeshType(comm, pumi_mesh));
   PCU_Comm_Free();
#ifdef MFEM_USE_SIMMETRIX
   gmi_sim_stop();
   Sim_unregisterAllKeys();
#endif
#else
   mesh.reset(new MeshType(comm, *smesh));
#endif //MFEM_USE_PUMI
#else
   mesh.reset(new MeshType(*smesh));
#endif //MFEM_USE_MPI
}

void AbstractSolver::setInitialCondition(
    void (*u_init)(const Vector &, Vector &))
{
   // TODO: Need to verify that this is ok for scalar fields
   VectorFunctionCoefficient u0(num_state, u_init);
   u->ProjectCoefficient(u0);
   // DenseMatrix vals;
   // Vector uj;
   // for (int i = 0; i < fes->GetNE(); i++)
   // {
   //    const FiniteElement *fe = fes->GetFE(i);
   //    const IntegrationRule *ir = &(fe->GetNodes());
   //    ElementTransformation *T = fes->GetElementTransformation(i);
   //    u->GetVectorValues(*T, *ir, vals);
   //    for (int j = 0; j < ir->GetNPoints(); j++)
   //    {
   //       vals.GetColumnReference(j, uj);
   //       cout << "uj = " << uj(0) << ", " << uj(1) << ", " << uj(2) << ", " << uj(3) << endl;
   //    }
   // }
}

double AbstractSolver::calcL2Error(
    void (*u_exact)(const Vector &, Vector &), int entry)
{
   // TODO: need to generalize to parallel
   VectorFunctionCoefficient exsol(num_state, u_exact);
   //return u->ComputeL2Error(ue);

   double loc_norm = 0.0;
   const FiniteElement *fe;
   ElementTransformation *T;
   DenseMatrix vals, exact_vals;
   Vector loc_errs;

   if (entry < 0)
   {
      // sum up the L2 error over all states
      for (int i = 0; i < fes->GetNE(); i++)
      {
         fe = fes->GetFE(i);
         const IntegrationRule *ir = &(fe->GetNodes());
         T = fes->GetElementTransformation(i);
         u->GetVectorValues(*T, *ir, vals);
         exsol.Eval(exact_vals, *T, *ir);
         vals -= exact_vals;
         loc_errs.SetSize(vals.Width());
         vals.Norm2(loc_errs);
         for (int j = 0; j < ir->GetNPoints(); j++)
         {
            const IntegrationPoint &ip = ir->IntPoint(j);
            T->SetIntPoint(&ip);
            loc_norm += ip.weight * T->Weight() * (loc_errs(j) * loc_errs(j));
         }
      }
   }
   else
   {
      // calculate the L2 error for component index `entry`
      for (int i = 0; i < fes->GetNE(); i++)
      {
         fe = fes->GetFE(i);
         const IntegrationRule *ir = &(fe->GetNodes());
         T = fes->GetElementTransformation(i);
         u->GetVectorValues(*T, *ir, vals);
         exsol.Eval(exact_vals, *T, *ir);
         vals -= exact_vals;
         loc_errs.SetSize(vals.Width());
         vals.GetRow(entry, loc_errs);
         for (int j = 0; j < ir->GetNPoints(); j++)
         {
            const IntegrationPoint &ip = ir->IntPoint(j);
            T->SetIntPoint(&ip);
            loc_norm += ip.weight * T->Weight() * (loc_errs(j) * loc_errs(j));
         }
      }
   }
   double norm;
#ifdef MFEM_USE_MPI
   MPI_Allreduce(&loc_norm, &norm, 1, MPI_DOUBLE, MPI_SUM, comm);
#else
   norm = loc_norm;
#endif
   if (norm < 0.0) // This was copied from mfem...should not happen for us
   {
      return -sqrt(-norm);
   }
   return sqrt(norm);
}

double AbstractSolver::calcStepSize(double cfl) const
{
   throw MachException("AbstractSolver::calcStepSize(cfl)\n"
                       "\tis not implemented for this class!");
}

void AbstractSolver::printSolution(const std::string &file_name, int refine)
{
   // TODO: These mfem functions do not appear to be parallelized
   ofstream sol_ofs(file_name + ".vtk");
   sol_ofs.precision(14);
   if (refine == -1)
   {
      refine = options["space-dis"]["degree"].get<int>() + 1;
   }
   mesh->PrintVTK(sol_ofs, refine);
   u->SaveVTK(sol_ofs, "Solution", refine);
   sol_ofs.close();
}

void AbstractSolver::solveForState()
{
   if (options["steady"].get<bool>() == true)
   {
      solveSteady();
   }
   else
   {
      solveUnsteady();
   }
}

void AbstractSolver::solveSteady()
{
#ifdef MFEM_USE_PETSC
   // Get the PetscSolver option 
   double abstol = options["petscsolver"]["abstol"].get<double>();
   double reltol = options["petscsolver"]["reltol"].get<double>();
   int maxiter = options["petscsolver"]["maxiter"].get<int>();
   int ptl = options["petscsolver"]["printlevel"].get<int>();

   solver.reset(new mfem::PetscLinearSolver(fes->GetComm(), "solver_", 0));
   prec.reset(new mfem::PetscPreconditioner(fes->GetComm(), "prec_"));
   dynamic_cast<mfem::PetscLinearSolver *>(solver.get())->SetPreconditioner(*prec);

   dynamic_cast<mfem::PetscSolver *>(solver.get())->SetAbsTol(abstol);
   dynamic_cast<mfem::PetscSolver *>(solver.get())->SetRelTol(reltol);
   dynamic_cast<mfem::PetscSolver *>(solver.get())->SetMaxIter(maxiter);
   dynamic_cast<mfem::PetscSolver *>(solver.get())->SetPrintLevel(ptl);
   std::cout << "PetscLinearSolver is set.\n";
   //Get the newton solver options
   double nabstol = options["newtonsolver"]["abstol"].get<double>();
   double nreltol = options["newtonsolver"]["reltol"].get<double>();
   int nmaxiter = options["newtonsolver"]["maxiter"].get<int>();
   int nptl = options["newtonsolver"]["printlevel"].get<int>();
   newton_solver.reset(new mfem::NewtonSolver(fes->GetComm()));
   newton_solver->iterative_mode = true;
   newton_solver->SetSolver(*solver);
   newton_solver->SetOperator(*res);
   newton_solver->SetAbsTol(nabstol);
   newton_solver->SetRelTol(nreltol);
   newton_solver->SetMaxIter(nmaxiter);
   newton_solver->SetPrintLevel(nptl);
   // Solve the nonlinear problem with r.h.s at 0
   mfem::Vector b;
   newton_solver->Mult(b, *u);
   MFEM_VERIFY(newton_solver->GetConverged(), "Newton solver did not converge.");
#else
   // Hypre solver section
   //prec.reset( new HypreBoomerAMG() );
   //prec->SetPrintLevel(0);
   std::cout << "ILU preconditioner is not available in Hypre. Running HypreGMRES"
               << " without preconditioner.\n";
   
   double tol = options["hypresolver"]["tol"].get<double>();
   int maxiter = options["hypresolver"]["maxiter"].get<int>();
   int ptl = options["hypresolver"]["printlevel"].get<int>();
   solver.reset( new HypreGMRES(fes->GetComm()) );
   dynamic_cast<mfem::HypreGMRES*> (solver.get())->SetTol(tol);
   dynamic_cast<mfem::HypreGMRES*> (solver.get())->SetMaxIter(maxiter);
   dynamic_cast<mfem::HypreGMRES*> (solver.get())->SetPrintLevel(ptl);

   //solver->SetPreconditioner(*prec);
   double nabstol = options["newtonsolver"]["abstol"].get<double>();
   double nreltol = options["newtonsolver"]["reltol"].get<double>();
   int nmaxiter = options["newtonsolver"]["maxiter"].get<int>();
   int nptl = options["newtonsolver"]["printlevel"].get<int>();
   newton_solver.reset(new mfem::NewtonSolver(fes->GetComm()));
   newton_solver->iterative_mode = true;
   newton_solver->SetSolver(*solver);
   newton_solver->SetOperator(*res);
   newton_solver->SetPrintLevel(nptl);
   newton_solver->SetRelTol(nreltol);
   newton_solver->SetAbsTol(nabstol);
   newton_solver->SetMaxIter(nmaxiter);

   // Solve the nonlinear problem with r.h.s at 0
   mfem::Vector b;
   newton_solver->Mult(b,  *u);
   MFEM_VERIFY(newton_solver->GetConverged(), "Newton solver did not converge.");
#endif
}

void AbstractSolver::solveUnsteady()
{
   // TODO: This is not general enough.

   double t = 0.0;
   evolver->SetTime(t);
   ode_solver->Init(*evolver);

   // output the mesh and initial condition
   // TODO: need to swtich to vtk for SBP
   int precision = 8;
   {
      ofstream omesh("unsteady-vortex.mesh");
      omesh.precision(precision);
      mesh->Print(omesh);
      ofstream osol("unsteady-vortex-init.gf");
      osol.precision(precision);
      u->Save(osol);
   }

   printSolution("init");

   bool done = false;
   double t_final = options["time-dis"]["t-final"].get<double>();
   std::cout << "t_final is " << t_final << '\n';
   double dt = options["time-dis"]["dt"].get<double>();
   for (int ti = 0; !done;)
   {
      if (options["time-dis"]["const-cfl"].get<bool>())
      {
         dt = calcStepSize(options["time-dis"]["cfl"].get<double>());
         //std::cout << "dt is " << dt << '\n';
      }
      //std::cout << "t is " << t <<'\n';
      double dt_real = min(dt, t_final - t);
      if (ti % 100 == 0)
      {
         cout << "iter " << ti << ": time = " << t << ": dt = " << dt_real
              << " (" << round(100 * t / t_final) << "% complete)" << endl;
      }
#ifdef MFEM_USE_MPI
      HypreParVector *U = u->GetTrueDofs();
      ode_solver->Step(*U, t, dt_real);
      *u = *U;
#else
      ode_solver->Step(*u, t, dt_real);
#endif
      ti++;
      //std::cout << "t_final is " << t_final << '\n';
      done = (t >= t_final - 1e-8 * dt);
      //std::cout << "t_final is " << t_final << ", done is " << done << std::endl;
      /*       if (done || ti % vis_steps == 0)
      {
         cout << "time step: " << ti << ", time: " << t << endl;

         if (visualization)
         {
            sout << "solution\n" << mesh << u << flush;
         }

         if (visit)
         {
            dc->SetCycle(ti);
            dc->SetTime(t);
            dc->Save();
         }
      } */
   }

   // Save the final solution. This output can be viewed later using GLVis:
   // glvis -m unitGridTestMesh.msh -g adv-final.gf".
   {
      ofstream osol("unsteady-vortex-final.gf");
      osol.precision(precision);
      u->Save(osol);
   }
   // write the solution to vtk file
   if (options["space-dis"]["basis-type"].get<string>() == "csbp")
   {
      ofstream sol_ofs("steady_vortex_cg.vtk");
      sol_ofs.precision(14);
      mesh->PrintVTK(sol_ofs, options["space-dis"]["degree"].get<int>() + 1);
      u->SaveVTK(sol_ofs, "Solution", options["space-dis"]["degree"].get<int>() + 1);
      sol_ofs.close();
      printSolution("final");
   }
   else if (options["space-dis"]["basis-type"].get<string>() == "dsbp")
   {
      ofstream sol_ofs("steady_vortex_dg.vtk");
      sol_ofs.precision(14);
      mesh->PrintVTK(sol_ofs, options["space-dis"]["degree"].get<int>() + 1);
      u->SaveVTK(sol_ofs, "Solution", options["space-dis"]["degree"].get<int>() + 1);
      sol_ofs.close();
      printSolution("final");
   }
   // TODO: These mfem functions do not appear to be parallelized
}

double AbstractSolver::calcOutput(const std::string &fun)
{
   try
   {
      return output.at(fun).GetEnergy(*u);
   }
   catch (const std::out_of_range &exception)
   {
      std::cerr << exception.what() << endl;
   }
}
  
void AbstractSolver::jacobianCheck()
{
   // initialize the variables
   const double delta = 1e-5;
   std::unique_ptr<GridFunType> u_plus;
   std::unique_ptr<GridFunType> u_minus;
   std::unique_ptr<GridFunType> perturbation_vec;
   perturbation_vec.reset(new GridFunType(fes.get()));
   VectorFunctionCoefficient up(num_state, perturb_fun);
   perturbation_vec->ProjectCoefficient(up);
   u_plus.reset(new GridFunType(fes.get()));
   u_minus.reset(new GridFunType(fes.get()));

   // set uplus and uminus to the current state
   *u_plus = *u;
   *u_minus = *u;
   u_plus->Add(delta, *perturbation_vec);
   u_minus->Add(-delta, *perturbation_vec);

   std::unique_ptr<GridFunType> res_plus;
   std::unique_ptr<GridFunType> res_minus;
   res_plus.reset(new GridFunType(fes.get()));
   res_minus.reset(new GridFunType(fes.get()));

   res->Mult(*u_plus, *res_plus);
   res->Mult(*u_minus, *res_minus);

   res_plus->Add(-1.0, *res_minus);
   res_plus->Set(1 / (2 * delta), *res_plus);

   // result from GetGradient(x)
   std::unique_ptr<GridFunType> jac_v;
   jac_v.reset(new GridFunType(fes.get()));
   mfem::Operator &jac = res->GetGradient(*u);
   jac.Mult(*perturbation_vec, *jac_v);
   // check the difference norm
   jac_v->Add(-1.0, *res_plus);
   std::cout << "The difference norm is " << jac_v->Norml2() << '\n';
}

} // namespace mach
