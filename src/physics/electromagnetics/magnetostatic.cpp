#include <fstream>
#include <random>

#include "adept.h"
#include "pfem_extras.hpp"

#include "magnetostatic.hpp"
#include "solver.hpp"
#include "electromag_integ.hpp"
#include "mesh_movement.hpp"
#include "res_integ.hpp"

using namespace std;
using namespace mfem;

using adept::adouble;

namespace
{
/// permeability of free space
const double mu_0 = 4e-7*M_PI;

std::unique_ptr<mfem::Coefficient>
constructReluctivityCoeff(nlohmann::json &component, nlohmann::json &materials)
{
   std::unique_ptr<mfem::Coefficient> temp_coeff;
   std::string material = component["material"].get<std::string>();
   if (!component["linear"].get<bool>())
   {
      auto b = materials[material]["B"].get<std::vector<double>>();
      auto h = materials[material]["H"].get<std::vector<double>>();
      temp_coeff.reset(new mach::ReluctivityCoefficient(b, h));
   }
   else
   {
      auto mu_r = materials[material]["mu_r"].get<double>();
      temp_coeff.reset(new mfem::ConstantCoefficient(1.0/(mu_r*mu_0)));
      // std::cout << "new coeff with mu_r: " << mu_r << "\n";
   }
   return temp_coeff;
}

// define the random-number generator; uniform between -1 and 1
static std::default_random_engine gen;
static std::uniform_real_distribution<double> uniform_rand(-1.0,1.0);

// double randState(const mfem::Vector &x)
// {
//    return 2.0 * uniform_rand(gen) - 1.0;
// }

void randState(const mfem::Vector &x, mfem::Vector &u)
{
   // std::cout << "u size: " << u.Size() << std::endl;
   for (int i = 0; i < u.Size(); ++i)
   {
      // std::cout << i << std::endl;
      u(i) = uniform_rand(gen);
   }
}

template <typename xdouble = double>
void phase_a_current(const xdouble &current_density,
                     const xdouble &fill_factor,
                     const xdouble *x,
                     xdouble *J)
{
   J[0] = 0.0;
   J[1] = 0.0;
   J[2] = 0.0;

   // example of needed geometric parameters, this should be all you need
   xdouble n_s = 12; // number of slots
   xdouble zb = 0.0; // bottom of stator
   xdouble zt = 0.25; // top of stator

   // compute theta from x and y
   xdouble tha = atan2(x[1], x[0]);
   xdouble thw = 2 * M_PI / n_s; //total angle of slot

   // check which winding we're in
   xdouble w = round(tha/thw); // current slot
   xdouble th = tha - w*thw;

   // check if we're in the stator body
   if (x[2] >= zb && x[2] <= zt)
   {
      // check if we're in left or right half
      if (th > 0)
      {
         J[2] = -1; // set to 1 for now, and direction depends on current direction
      }
      if (th < 0)
      {
         J[2] = 1;	
      }
   }
   else  // outside of the stator body, check if above or below
   {
      // 'subtract' z position to 0 depending on if above or below
      xdouble rx[] = {x[0], x[1], x[2]};
      if (x[2] > zt) 
      {
         rx[2] -= zt; 
      }
      if (x[2] < zb) 
      {
         rx[2] -= zb; 
      }

      // draw top rotation axis
      xdouble ax[] = {0.0, 0.0, 0.0};
      ax[0] = cos(w * thw);
      ax[1] = sin(w * thw);

      // take x cross ax, normalize
      J[0] = rx[1]*ax[2] - rx[2]*ax[1];
      J[1] = rx[2]*ax[0] - rx[0]*ax[2];
      J[2] = rx[0]*ax[1] - rx[1]*ax[0];
      xdouble norm_J = sqrt(J[0]*J[0] + J[1]*J[1] + J[2]*J[2]);
      J[0] /= norm_J;
      J[1] /= norm_J;
      J[2] /= norm_J;
   }
   J[0] *= current_density * fill_factor;
   J[1] *= current_density * fill_factor;
   J[2] *= current_density * fill_factor;
}

template <typename xdouble = double>
void phase_b_current(const xdouble &current_density,
                     const xdouble &fill_factor,
                     const xdouble *x,
                     xdouble *J)
{
   J[0] = 0.0;
   J[1] = 0.0;
   J[2] = 0.0;

   // example of needed geometric parameters, this should be all you need
   xdouble n_s = 12; // number of slots
   xdouble zb = 0.0; // bottom of stator
   xdouble zt = 0.25; // top of stator

   // compute theta from x and y
   xdouble tha = atan2(x[1], x[0]);
   xdouble thw = 2 * M_PI / n_s; // total angle of slot

   // check which winding we're in
   xdouble w = round(tha/thw); // current slot
   xdouble th = tha - w*thw;

   // check if we're in the stator body
   if (x[2] >= zb && x[2] <= zt)
   {
      // check if we're in left or right half
      if (th > 0)
      {
         J[2] = -1; // set to 1 for now, and direction depends on current direction
      }
      if (th < 0)
      {
         J[2] = 1;	
      }
   }
   else  // outside of the stator body, check if above or below
   {
      // 'subtract' z position to 0 depending on if above or below
      xdouble rx[] = {x[0], x[1], x[2]};
      if (x[2] > zt) 
      {
         rx[2] -= zt; 
      }
      if (x[2] < zb) 
      {
         rx[2] -= zb; 
      }

      // draw top rotation axis
      xdouble ax[] = {0.0, 0.0, 0.0};
      ax[0] = cos(w * thw);
      ax[1] = sin(w * thw);

      // take x cross ax, normalize
      J[0] = rx[1]*ax[2] - rx[2]*ax[1];
      J[1] = rx[2]*ax[0] - rx[0]*ax[2];
      J[2] = rx[0]*ax[1] - rx[1]*ax[0];
      xdouble norm_J = sqrt(J[0]*J[0] + J[1]*J[1] + J[2]*J[2]);
      J[0] /= norm_J;
      J[1] /= norm_J;
      J[2] /= norm_J;
   }
   J[0] *= -current_density * fill_factor;
   J[1] *= -current_density * fill_factor;
   J[2] *= -current_density * fill_factor;
}

template <typename xdouble = double>
void phase_c_current(const xdouble &current_density,
                     const xdouble &fill_factor,
                     const xdouble *x,
                     xdouble *J)
{
   J[0] = 0.0;
   J[1] = 0.0;
   J[2] = 0.0;
}

template <typename xdouble = double>
void north_magnetization(const xdouble& remnant_flux,
                         const xdouble *x,
                         xdouble *M)
{
   xdouble r[] = {0.0, 0.0, 0.0};
   r[0] = x[0];
   r[1] = x[1];
   xdouble norm_r = sqrt(r[0]*r[0] + r[1]*r[1]);
   M[0] = r[0] * remnant_flux / norm_r;
   M[1] = r[1] * remnant_flux / norm_r;
   M[2] = 0.0;
}

template <typename xdouble = double>
void south_magnetization(const xdouble& remnant_flux,
                         const xdouble *x,
                         xdouble *M)
{
   xdouble r[] = {0.0, 0.0, 0.0};
   r[0] = x[0];
   r[1] = x[1];
   xdouble norm_r = sqrt(r[0]*r[0] + r[1]*r[1]);
   M[0] = -r[0] * remnant_flux / norm_r;
   M[1] = -r[1] * remnant_flux / norm_r;
   M[2] = 0.0;
}

template <typename xdouble = double>
void x_axis_current(const xdouble &current_density,
                    const xdouble *x,
                    xdouble *J)
{
   J[0] = current_density;
   J[1] = 0.0;
   J[2] = 0.0;
}

template <typename xdouble = double>
void y_axis_current(const xdouble &current_density,
                    const xdouble *x,
                    xdouble *J)
{
   J[0] = 0.0;
   J[1] = current_density;
   J[2] = 0.0;
}

template <typename xdouble = double>
void z_axis_current(const xdouble &current_density,
                    const xdouble *x,
                    xdouble *J)
{
   J[0] = 0.0;
   J[1] = 0.0;
   J[2] = current_density;
}

template <typename xdouble = double>
void ring_current(const xdouble &current_density,
                  const xdouble *x,
                  xdouble *J)
{
   for (int i = 0; i < 3; ++i)
   {
      J[i] = 0.0;
   }
   xdouble r[] = {0.0, 0.0, 0.0};
   r[0] = x[0];
   r[1] = x[1];
   xdouble norm_r = sqrt(r[0]*r[0] + r[1]*r[1]);
   r[0] /= norm_r;
   r[1] /= norm_r;
   J[0] = -r[1] * current_density;
   J[1] = r[0] * current_density;
}

template <typename xdouble = double>
void x_axis_magnetization(const xdouble& remnant_flux,
                          const xdouble *x,
                          xdouble *M)
{
   M[0] = remnant_flux;
   M[1] = 0.0;
   M[2] = 0.0;
}

template <typename xdouble = double>
void y_axis_magnetization(const xdouble& remnant_flux,
                          const xdouble *x,
                          xdouble *M)
{
   M[0] = 0.0;
   M[1] = remnant_flux;
   M[2] = 0.0;
}

template <typename xdouble = double>
void z_axis_magnetization(const xdouble& remnant_flux,
                          const xdouble *x,
                          xdouble *M)
{
   M[0] = 0.0;
   M[1] = 0.0;
   M[2] = remnant_flux;
}

template <typename xdouble = double>
void box1_current(const xdouble &current_density,
                  const xdouble *x,
                  xdouble *J)
{
   for (int i = 0; i < 3; ++i)
   {
      J[i] = 0.0;
   }

	xdouble y = x[1] - .5;

   // J[2] = -current_density*6*y*(1/(M_PI*4e-7)); // for real scaled problem
   J[2] = -current_density*6*y;

}

template <typename xdouble = double>
void box2_current(const xdouble &current_density,
                  const xdouble *x,
                  xdouble *J)
{
   for (int i = 0; i < 3; ++i)
   {
      J[i] = 0.0;
   }

	xdouble y = x[1] - .5;

   // J[2] = current_density*6*y*(1/(M_PI*4e-7)); // for real scaled problem
   J[2] = current_density*6*y;
}

// void func(const mfem::Vector &x, mfem::Vector &y)
// {
//    y.SetSize(3);
//    y(0) = x(0)*x(0) - x(1);
//    y(1) = x(0) * exp(x(1));
//    y(2) = x(2)*x(0) - x(1);
// }

// void funcRevDiff(const mfem::Vector &x, const mfem::Vector &v_bar, mfem::Vector &x_bar)
// {
//    x_bar(0) = v_bar(0) * 2*x(0) + v_bar(1) * exp(x(1)) + v_bar(2)*x(2);
//    x_bar(1) = -v_bar(0) + v_bar(1) * x(0) * exp(x(1)) - v_bar(2); 
//    x_bar(2) = v_bar(2) * x(0); 
// }

} // anonymous namespace

namespace mach
{

MagnetostaticSolver::MagnetostaticSolver(
   const std::string &opt_file_name,
   std::unique_ptr<mfem::Mesh> smesh)
   : AbstractSolver(opt_file_name, move(smesh))
{
   dim = getMesh()->Dimension();
   int order = options["space-dis"]["degree"].get<int>();
   // num_state = dim;

   mesh->ReorientTetMesh();



   mesh->RemoveInternalBoundaries();

   /// Create the H(Div) finite element collection
   h_div_coll.reset(new RT_FECollection(order, dim));
   /// Create the H1 finite element collection
   h1_coll.reset(new H1_FECollection(order, dim));
   /// Create the L2 finite element collection
   l2_coll.reset(new L2_FECollection(order, dim));

   /// Create the H(Div) finite element space
   h_div_space.reset(new SpaceType(mesh.get(), h_div_coll.get()));
   /// Create the H1 finite element space
   h1_space.reset(new SpaceType(mesh.get(), h1_coll.get()));
   /// Create the L2 finite element space
   l2_space.reset(new SpaceType(mesh.get(), l2_coll.get()));

   /// Create magnetic flux grid function
   B.reset(new GridFunType(h_div_space.get()));
}

MagnetostaticSolver::MagnetostaticSolver(
   const nlohmann::json &_options,
   std::unique_ptr<mfem::Mesh> smesh)
   : AbstractSolver(_options, move(smesh))
{
   dim = getMesh()->Dimension();
   int order = options["space-dis"]["degree"].get<int>();
   // num_state = dim;

   mesh->ReorientTetMesh();
   mesh->RemoveInternalBoundaries();

   /// Create the H(Div) finite element collection
   h_div_coll.reset(new RT_FECollection(order, dim));
   /// Create the H1 finite element collection
   h1_coll.reset(new H1_FECollection(order, dim));
   /// Create the L2 finite element collection
   l2_coll.reset(new L2_FECollection(order, dim));

   /// Create the H(Div) finite element space
   h_div_space.reset(new SpaceType(mesh.get(), h_div_coll.get()));
   /// Create the H1 finite element space
   h1_space.reset(new SpaceType(mesh.get(), h1_coll.get()));
   /// Create the L2 finite element space
   l2_space.reset(new SpaceType(mesh.get(), l2_coll.get()));

   /// Create magnetic flux grid function
   B.reset(new GridFunType(h_div_space.get()));
}

MagnetostaticSolver::~MagnetostaticSolver() = default;

void MagnetostaticSolver::printSolution(const std::string &file_name,
                                       int refine)
{
   printFields(file_name,
               {u.get(), B.get()},
               {"MVP", "Magnetic_Flux_Density"},
               refine);
}

void MagnetostaticSolver::setEssentialBoundaries()
{
   /// apply zero tangential boundary condition everywhere
   ess_bdr.SetSize(mesh->bdr_attributes.Max());
   ess_bdr = 1;

   Array<int> ess_tdof_list;
   fes->GetEssentialTrueDofs(ess_bdr, ess_tdof_list);
   res->SetEssentialTrueDofs(ess_tdof_list);
   /// set current vector's ess_tdofs to zero
   current_vec->SetSubVector(ess_tdof_list, 0.0);

   /// set current vector to zero on any model surface
   // current_vec->SetSubVector(fes_surface_dofs, 0.0);
}

void MagnetostaticSolver::solveSteady()
{
   if (newton_solver == nullptr)
      constructNewtonSolver();

   setEssentialBoundaries();

   Vector Zero(3);
   Zero = 0.0;
   bool box_prob = options["problem-opts"].value("box", false);

   if (!box_prob)
      bc_coef.reset(new VectorConstantCoefficient(Zero)); // for motor 
   else
      bc_coef.reset(new VectorFunctionCoefficient(3, a_exact)); // for box problem

   *u = 0.0;
   u->ProjectBdrCoefficientTangent(*bc_coef, ess_bdr);

   HypreParVector *u_true = u->GetTrueDofs();
   HypreParVector *current_true = current_vec->GetTrueDofs();
   newton_solver->Mult(*current_true, *u_true);
   MFEM_VERIFY(newton_solver->GetConverged(), "Newton solver did not converge.");
   u->SetFromTrueDofs(*u_true);

   computeSecondaryFields();
}

void MagnetostaticSolver::addOutputs()
{
   auto &fun = options["outputs"];
   if (fun.find("energy") != fun.end())
   { 
      output.emplace("energy", fes.get());
      output.at("energy").AddDomainIntegrator(
         new MagneticEnergyIntegrator(nu.get()));
   }
   if (fun.find("co-energy") != fun.end())
   {
      output.emplace("co-energy", fes.get());
      output.at("co-energy").AddDomainIntegrator(
         new MagneticCoenergyIntegrator(*u, nu.get()));
   }
}

std::vector<GridFunType*> MagnetostaticSolver::getFields(void)
{
   return {u.get(), B.get()};
}

GridFunction* MagnetostaticSolver::getMeshSensitivities()
{	
   /// assign mesh node space to forms
   mesh->EnsureNodes();
   SpaceType *mesh_fes = static_cast<SpaceType*>(mesh->GetNodes()->FESpace());

   dLdX.reset(new GridFunType(mesh_fes));
   *dLdX = 0.0;

   /// Add mesh sensitivities of functional 
   LinearFormType dJdX(mesh_fes);
   dJdX.AddDomainIntegrator(
      new MagneticCoenergyIntegrator(*u, nu.get()));
   dJdX.Assemble();
   std::cout << "dJdX norm: " << dJdX.Norml2() << "\n";
   /// TODO I don't know if this works in parallel / when we need to use tdof vectors
   // *dLdX -= dJdX;

   res_mesh_sens_l.reset(new LinearFormType(mesh_fes));

   /// compute \psi_A
   solveForAdjoint("co-energy");

   /// add integrators R = CurlCurl(A) + Cm + Mj = 0
   /// \psi^T CurlCurl(A)
   res_mesh_sens_l->AddDomainIntegrator(
      new CurlCurlNLFIntegrator(nu.get(), u.get(), adj.get()));
   /// \psi^T C m 
   res_mesh_sens_l->AddDomainIntegrator(
      new VectorFECurldJdXIntegerator(nu.get(), M.get(), adj.get(),
                                      mag_coeff.get(), -1.0));

   /// Compute the derivatives and accumulate the result
   res_mesh_sens_l->Assemble();

   
   ParGridFunction j_mesh_sens(mesh_fes);
   j_mesh_sens = 0.0;
   auto *j_mesh_sens_true = j_mesh_sens.GetTrueDofs();
   getCurrentSourceMeshSens(*adj, *j_mesh_sens_true);
   std::cout << "residual dJdX norm: " << res_mesh_sens_l->Norml2() << "\n";
   std::cout << "current source dJdX norm: " << j_mesh_sens_true->Norml2() << "\n";
   /// dJdX = \partialJ / \partial X + \psi^T \partial R / \partial X
   dLdX->Add(1, *res_mesh_sens_l);
   dLdX->Add(-1, *j_mesh_sens_true);

   return dLdX.get();
}

void MagnetostaticSolver::verifyMeshSensitivities()
{
   std::cout << "Verifying Mesh Sensitivities..." << std::endl;
   int dim = mesh->SpaceDimension();
   double delta = 1e-7;
   double delta_cd = 1e-5;
   double dJdX_fd_v = -calcOutput("co-energy") / delta;
   double dJdX_cd_v = 0.0;

   VectorFunctionCoefficient v_rand(dim, randState);
   // GridFunction state(fes.get());
   // GridFunction adjoint(fes.get());
   // state.ProjectCoefficient(v_rand);
   // adjoint.ProjectCoefficient(v_rand);

   ess_bdr.SetSize(mesh->bdr_attributes.Max());
   ess_bdr = 0;

   Array<int> ess_tdof_list;
   fes->GetEssentialTrueDofs(ess_bdr, ess_tdof_list);
   res->SetEssentialTrueDofs(ess_tdof_list);


   Vector *dJdX_vect = getMeshSensitivities();

   // extract mesh nodes and get their finite-element space
   GridFunction *x_nodes = mesh->GetNodes();
   FiniteElementSpace *mesh_fes = x_nodes->FESpace();
   GridFunction dJdX(mesh_fes, dJdX_vect->GetData());
   GridFunction dJdX_fd(mesh_fes); GridFunction dJdX_cd(mesh_fes);
   GridFunction dJdX_fd_err(mesh_fes); GridFunction dJdX_cd_err(mesh_fes);
   // initialize the vector that we use to perturb the mesh nodes
   GridFunction v(mesh_fes);
   v.ProjectCoefficient(v_rand);

   /// only perturb the inside dofs not on model faces
   // for (int i = 0; i < mesh_fes_surface_dofs.Size(); ++i)
   // {
   //    v(mesh_fes_surface_dofs[i]) = 0.0;
   // }

   // contract dJ/dX with v
   double dJdX_v = (dJdX) * v;

   if (options["verify-full"].get<bool>())
   {
      for(int k = 0; k < x_nodes->Size(); k++)
      {
         GridFunction x_pert(*x_nodes);
         x_pert(k) += delta; mesh->SetNodes(x_pert);
         std::cout << "Solving Forward Step..." << std::endl;
         Update();
         solveForState();
         std::cout << "Solver Done" << std::endl;
         dJdX_fd(k) = calcOutput("co-energy")/delta + dJdX_fd_v;
         x_pert(k) -= delta; mesh->SetNodes(x_pert);

      }
      // central difference
      for(int k = 0; k < x_nodes->Size(); k++)
      {
         //forward
         GridFunction x_pert(*x_nodes);
         x_pert(k) += delta_cd; mesh->SetNodes(x_pert);
         std::cout << "Solving Forward Step..." << std::endl;
         Update();
         solveForState();
         std::cout << "Solver Done" << std::endl;
         dJdX_cd(k) = calcOutput("co-energy")/(2*delta_cd);

         //backward
         x_pert(k) -= 2*delta_cd; mesh->SetNodes(x_pert);
         std::cout << "Solving Backward Step..." << std::endl;
         Update();
         solveForState();
         std::cout << "Solver Done" << std::endl;
         dJdX_cd(k) -= calcOutput("co-energy")/(2*delta_cd);
         x_pert(k) += delta_cd; mesh->SetNodes(x_pert);
      }

      dJdX_fd_v = dJdX_fd*v;
      dJdX_cd_v = dJdX_cd*v;
      dJdX_fd_err += dJdX_fd; dJdX_fd_err -= dJdX;
      dJdX_cd_err += dJdX_cd; dJdX_cd_err -= dJdX;
      std::cout << "FD L2:  " << dJdX_fd_err.Norml2() << std::endl;
      std::cout << "CD L2:  " << dJdX_cd_err.Norml2() << std::endl;
      for(int k = 0; k < x_nodes->Size(); k++)
      {
         dJdX_fd_err(k) = dJdX_fd_err(k)/dJdX(k);
         dJdX_cd_err(k) = dJdX_cd_err(k)/dJdX(k);
      }
      stringstream fderrname;
      fderrname << "dJdX_fd_err.gf";
      ofstream fd(fderrname.str()); fd.precision(15);
      dJdX_fd_err.Save(fd);

      stringstream cderrname;
      cderrname << "dJdX_cd_err.gf";
      ofstream cd(cderrname.str()); cd.precision(15);
      dJdX_cd_err.Save(cd);

      stringstream analytic;
      analytic << "dJdX.gf";
      ofstream an(analytic.str()); an.precision(15);
      dJdX.Save(an);
   }
   else
   {
      // // compute finite difference approximation
      GridFunction x_pert(*x_nodes);
      // x_pert.Add(delta, v);
      // mesh->SetNodes(x_pert);
      // std::cout << "Solving Forward Step..." << std::endl;
      // Update();
      // residual.reset(new GridFunType(fes.get()));
      // *residual = 0.0;
      // res->Mult(state, *residual);
      // *residual -= *current_vec;
      // dJdX_fd_v += *residual * adjoint;
      // // solveForState();
      // std::cout << "Solver Done" << std::endl;
      // // dJdX_fd_v += calcOutput("co-energy")/delta;

      // central difference approximation
      std::cout << "Solving CD Backward Step..." << std::endl;
      x_pert = *x_nodes; x_pert.Add(-delta_cd, v);
      mesh->SetNodes(x_pert);
      Update();
      residual.reset(new GridFunType(fes.get()));
      *residual = 0.0;
      res->Mult(*u, *residual);
      *residual -= *current_vec;
      dJdX_cd_v -= (*residual * *adj )/(2*delta_cd);

      // solveForState();
      // std::cout << "Solver Done" << std::endl;
      // dJdX_cd_v = -calcOutput("co-energy")/(2*delta_cd);

      std::cout << "Solving CD Forward Step..." << std::endl;
      x_pert.Add(2*delta_cd, v);
      mesh->SetNodes(x_pert);
      Update();
      residual.reset(new GridFunType(fes.get()));
      *residual = 0.0;
      res->Mult(*u, *residual);
      *residual -= *current_vec;
      dJdX_cd_v += (*residual * *adj )/(2*delta_cd);

      // solveForState();
      // std::cout << "Solver Done" << std::endl;
      // dJdX_cd_v += calcOutput("co-energy")/(2*delta_cd);
   }

   std::cout << "Volume Mesh Sensititivies:  " << std::endl;
   // std::cout << "Finite Difference:          " << dJdX_fd_v << std::endl;
   std::cout << "Central Difference:         " << dJdX_cd_v << std::endl;
   std::cout << "Analytic:                   " << dJdX_v << std::endl;
   // std::cout << "FD Relative:                " << (dJdX_v-dJdX_fd_v)/dJdX_v << std::endl;
   // std::cout << "FD Absolute:                " << dJdX_v - dJdX_fd_v << std::endl;
   std::cout << "CD Relative:                " << (dJdX_v-dJdX_cd_v)/dJdX_v << std::endl;
   std::cout << "CD Absolute:                " << dJdX_v - dJdX_cd_v << std::endl;
}

void MagnetostaticSolver::Update()
{
   fes->Update();
   h_div_space->Update();
   h1_space->Update();

   u->Update();
   adj->Update();
   B->Update();
   M->Update();
   current_vec->Update();
   div_free_current_vec->Update();

   res->Update();
   assembleCurrentSource();
   assembleMagnetizationSource();

   constructLinearSolver(options["lin-solver"]);
   constructNewtonSolver();
}

void MagnetostaticSolver::constructCoefficients()
{
   current_vec.reset(new GridFunType(fes.get()));
   div_free_current_vec.reset(new GridFunType(fes.get()));
   
   /// read options file to set the proper values of static member variables
   setStaticMembers();
   /// Construct current source coefficient
   constructCurrent();
   /// Assemble current source vector
   assembleCurrentSource();
   /// Construct magnetization coefficient
   constructMagnetization();
   /// Assemble magnetization source vector and add it into current
   assembleMagnetizationSource();
   /// Construct reluctivity coefficient
   constructReluctivity();
}

void MagnetostaticSolver::addVolumeIntegrators(double alpha)
{
   /// TODO: Add a check in `CurlCurlNLFIntegrator` to check if |B| is close to
   ///       zero, and if so set the second term of the Jacobian to be zero.
   /// add curl curl integrator to residual
   res->AddDomainIntegrator(new CurlCurlNLFIntegrator(nu.get()));
}

void MagnetostaticSolver::setStaticMembers()
{
   if (options["components"].contains("magnets"))
   {
      auto &magnets = options["components"]["magnets"];
      std::string material = magnets["material"].get<std::string>();
      remnant_flux = materials[material]["B_r"].get<double>();
      mag_mu_r = materials[material]["mu_r"].get<double>();
   }
   fill_factor = options["problem-opts"].value("fill-factor", 1.0);
   current_density = options["problem-opts"].value("current-density", 1.0);
}

void MagnetostaticSolver::constructReluctivity()
{
   /// set up default reluctivity to be that of free space
   // const double mu_0 = 4e-7*M_PI;
   std::unique_ptr<Coefficient> nu_free_space(
      new ConstantCoefficient(1.0/mu_0));
   nu.reset(new MeshDependentCoefficient(move(nu_free_space)));

   /// loop over all components, construct either a linear or nonlinear
   ///    reluctivity coefficient for each
   for (auto& component : options["components"])
   {
      int attr = component.value("attr", -1);
      if (-1 != attr)
      {
         std::unique_ptr<mfem::Coefficient> temp_coeff;
         temp_coeff = constructReluctivityCoeff(component, materials);
         nu->addCoefficient(attr, move(temp_coeff));
      }
      else
      {
         auto attrs = component["attrs"].get<std::vector<int>>();
         for (auto& attribute : attrs)
         {
            std::unique_ptr<mfem::Coefficient> temp_coeff;
            temp_coeff = constructReluctivityCoeff(component, materials);
            nu->addCoefficient(attribute, move(temp_coeff));
         }
      }
   }
}

/// TODO - this approach cannot support general magnet topologies where the
///        magnetization cannot be described by a single vector function 
void MagnetostaticSolver::constructMagnetization()
{
   mag_coeff.reset(new VectorMeshDependentCoefficient(dim));

   if (options["problem-opts"].contains("magnets"))
   {
      auto &magnets = options["problem-opts"]["magnets"];
      if (magnets.contains("north"))
      {
         auto attrs = magnets["north"].get<std::vector<int>>();
         for (auto& attr : attrs)
         {
            std::unique_ptr<mfem::VectorCoefficient> temp_coeff(
                  new VectorFunctionCoefficient(dim,
                                                northMagnetizationSource,
                                                northMagnetizationSourceRevDiff));
            mag_coeff->addCoefficient(attr, move(temp_coeff));
         }
      }
      if (magnets.contains("south"))
      {
         auto attrs = magnets["south"].get<std::vector<int>>();
         
         for (auto& attr : attrs)
         {
            std::unique_ptr<mfem::VectorCoefficient> temp_coeff(
                  new VectorFunctionCoefficient(dim,
                                                southMagnetizationSource,
                                                southMagnetizationSourceRevDiff));
            mag_coeff->addCoefficient(attr, move(temp_coeff));
         }
      }
      if (magnets.contains("x"))
      {
         auto attrs = magnets["x"].get<std::vector<int>>();
         for (auto& attr : attrs)
         {
            std::unique_ptr<mfem::VectorCoefficient> temp_coeff(
                  new VectorFunctionCoefficient(dim,
                                                xAxisMagnetizationSource,
                                                xAxisMagnetizationSourceRevDiff));
            mag_coeff->addCoefficient(attr, move(temp_coeff));
         }
      }
      if (magnets.contains("y"))
      {
         auto attrs = magnets["y"].get<std::vector<int>>();
         for (auto& attr : attrs)
         {
            std::unique_ptr<mfem::VectorCoefficient> temp_coeff(
                  new VectorFunctionCoefficient(dim,
                                                yAxisMagnetizationSource,
                                                yAxisMagnetizationSourceRevDiff));
            mag_coeff->addCoefficient(attr, move(temp_coeff));
         }
      }
      if (magnets.contains("z"))
      {
         auto attrs = magnets["z"].get<std::vector<int>>();
         for (auto& attr : attrs)
         {
            std::unique_ptr<mfem::VectorCoefficient> temp_coeff(
                  new VectorFunctionCoefficient(dim,
                                                zAxisMagnetizationSource,
                                                zAxisMagnetizationSourceRevDiff));
            mag_coeff->addCoefficient(attr, move(temp_coeff));
         }
      }
   }
}

void MagnetostaticSolver::constructCurrent()
{
   current_coeff.reset(new VectorMeshDependentCoefficient());

   if (options["problem-opts"].contains("current"))
   {
      auto &current = options["problem-opts"]["current"];
      if (current.contains("Phase-A"))
      {
         auto attrs = current["Phase-A"].get<std::vector<int>>();
         for (auto& attr : attrs)
         {
            std::unique_ptr<mfem::VectorCoefficient> temp_coeff(
                  new VectorFunctionCoefficient(dim,
                                                phaseACurrentSource,
                                                phaseACurrentSourceRevDiff));
            current_coeff->addCoefficient(attr, move(temp_coeff));
         }
      }
      if (current.contains("Phase-B"))
      {
         auto attrs = current["Phase-B"].get<std::vector<int>>();
         for (auto& attr : attrs)
         {
            std::unique_ptr<mfem::VectorCoefficient> temp_coeff(
                  new VectorFunctionCoefficient(dim,
                                                phaseBCurrentSource,
                                                phaseBCurrentSourceRevDiff));
            current_coeff->addCoefficient(attr, move(temp_coeff));
         }
      }
      if (current.contains("Phase-C"))
      {
         auto attrs = current["Phase-C"].get<std::vector<int>>();
         for (auto& attr : attrs)
         {
            std::unique_ptr<mfem::VectorCoefficient> temp_coeff(
                  new VectorFunctionCoefficient(dim,
                                                phaseCCurrentSource,
                                                phaseCCurrentSourceRevDiff));
            current_coeff->addCoefficient(attr, move(temp_coeff));
         }
      }
      if (current.contains("x"))
      {
         auto attrs = current["x"].get<std::vector<int>>();
         for (auto& attr : attrs)
         {
            std::unique_ptr<mfem::VectorCoefficient> temp_coeff(
                  new VectorFunctionCoefficient(dim,
                                                xAxisCurrentSource,
                                                xAxisCurrentSourceRevDiff));
            current_coeff->addCoefficient(attr, move(temp_coeff));
         }
      }
      if (current.contains("y"))
      {
         auto attrs = current["y"].get<std::vector<int>>();
         for (auto& attr : attrs)
         {
            std::unique_ptr<mfem::VectorCoefficient> temp_coeff(
                  new VectorFunctionCoefficient(dim,
                                                yAxisCurrentSource,
                                                yAxisCurrentSourceRevDiff));
            current_coeff->addCoefficient(attr, move(temp_coeff));
         }
      }
      if (current.contains("z"))
      {
         auto attrs = current["z"].get<std::vector<int>>();
         for (auto& attr : attrs)
         {
            std::unique_ptr<mfem::VectorCoefficient> temp_coeff(
                  new VectorFunctionCoefficient(dim,
                                                zAxisCurrentSource,
                                                zAxisCurrentSourceRevDiff));
            current_coeff->addCoefficient(attr, move(temp_coeff));
         }
      }
      if (current.contains("ring"))
      {
         auto attrs = current["ring"].get<std::vector<int>>();
         for (auto& attr : attrs)
         {
            std::unique_ptr<mfem::VectorCoefficient> temp_coeff(
                  new VectorFunctionCoefficient(dim,
                                                ringCurrentSource,
                                                ringCurrentSourceRevDiff));
            current_coeff->addCoefficient(attr, move(temp_coeff));
         }
      }
      if (current.contains("box1"))
      {
         auto attrs = current["box1"].get<std::vector<int>>();
         for (auto& attr : attrs)
         {
            std::unique_ptr<mfem::VectorCoefficient> temp_coeff(
                     new VectorFunctionCoefficient(dim,
                                                   box1CurrentSource,
                                                   box1CurrentSourceRevDiff));
            current_coeff->addCoefficient(attr, move(temp_coeff));
         }
      }
      if (current.contains("box2"))
      {
         auto attrs = current["box2"].get<std::vector<int>>();
         for (auto& attr : attrs)
         {
            std::unique_ptr<mfem::VectorCoefficient> temp_coeff(
                     new VectorFunctionCoefficient(dim,
                                                   box2CurrentSource,
                                                   box2CurrentSourceRevDiff));
            current_coeff->addCoefficient(attr, move(temp_coeff));
         }
      }
   }
}

void MagnetostaticSolver::assembleCurrentSource()
{
   int fe_order = options["space-dis"]["degree"].get<int>();

   /// get int rule (approach followed my MFEM Tesla Miniapp)
   int irOrder = h1_space->GetElementTransformation(0)->OrderW()
               + 2 * fe_order;
   int geom = h1_space->GetFE(0)->GetGeomType();
   const IntegrationRule *ir = &IntRules.Get(geom, irOrder);

   /// Create a H(curl) mass matrix for integrating grid functions
   BilinearFormIntegrator *h_curl_mass_integ = new VectorFEMassIntegrator;
   h_curl_mass_integ->SetIntRule(ir);
   BilinearFormType h_curl_mass(fes.get());
   h_curl_mass.AddDomainIntegrator(h_curl_mass_integ);
   // assemble mass matrix
   h_curl_mass.Assemble();
   h_curl_mass.Finalize();

   LinearFormType J(fes.get());
   J.AddDomainIntegrator(new VectorFEDomainLFIntegrator(*current_coeff));
   J.Assemble();

   GridFunType j(fes.get());
   // j = 0.0;
   j.ProjectCoefficient(*current_coeff);
   {
      auto *M = new HypreParMatrix;
      Vector X, RHS;
      Array<int> ess_tdof_list;
      h_curl_mass.FormLinearSystem(ess_tdof_list, j, J, *M, X, RHS);
      std::cout << "solving for j in H(curl)\n";
      HypreBoomerAMG amg(*M);
      amg.SetPrintLevel(0);
      HypreGMRES gmres(*M);
      gmres.SetTol(1e-12);
      gmres.SetMaxIter(200);
      gmres.SetPrintLevel(2);
      gmres.SetPreconditioner(amg);
      gmres.Mult(RHS, X);

      h_curl_mass.RecoverFEMSolution(X, J, j);
   }
   h_curl_mass.Assemble();
   h_curl_mass.Finalize();

   /// compute the divergence free current source
   auto div_free_proj = mfem::common::DivergenceFreeProjector(*h1_space, *fes,
                                             irOrder, NULL, NULL, NULL);

   // Compute the discretely divergence-free portion of j
   *div_free_current_vec = 0.0;
   div_free_proj.Mult(j, *div_free_current_vec);

   *current_vec = 0.0;
   h_curl_mass.AddMult(*div_free_current_vec, *current_vec);
}

void MagnetostaticSolver::getCurrentSourceMeshSens(
   const mfem::GridFunction &psi_a,
   mfem::Vector &mesh_sens)
{
   Array<int> ess_bdr, ess_bdr_tdofs;
   ess_bdr.SetSize(h1_space->GetParMesh()->bdr_attributes.Max());
   ess_bdr = 1;
   h1_space->GetEssentialTrueDofs(ess_bdr, ess_bdr_tdofs);

   /// compute \psi_k
   /// D \psi_k = G^T M^T \psi_A (\psi_j_hat = -M^T \psi_A)
   ParBilinearForm h_curl_mass(fes.get());
   h_curl_mass.AddDomainIntegrator(new VectorFEMassIntegrator);
   // assemble mass matrix
   h_curl_mass.Assemble();
   h_curl_mass.Finalize();

   /// compute \psi_j_hat
   /// \psi_j_hat = -\psi_A
   ParGridFunction psi_j_hat(fes.get());
   psi_j_hat = 0.0;
   h_curl_mass.MultTranspose(psi_a, psi_j_hat);
   psi_j_hat *= -1.0; // (\psi_j_hat = -M^T \psi_A)

   mfem::common::ParDiscreteGradOperator grad(h1_space.get(), fes.get());
   grad.Assemble();
   grad.Finalize();

   ParGridFunction GTMTpsi_a(h1_space.get());
   GTMTpsi_a = 0.0;
   grad.MultTranspose(*adj, GTMTpsi_a);

   ParBilinearForm D(h1_space.get());
   D.AddDomainIntegrator(new DiffusionIntegrator);
   D.Assemble();
   D.Finalize();
   
   auto *Dmat = new HypreParMatrix;

   ParGridFunction psi_k(h1_space.get());
   psi_k = 0.0;
   {
      Vector PSIK;
      Vector RHS;
      D.FormLinearSystem(ess_bdr_tdofs, psi_k, GTMTpsi_a, *Dmat, PSIK, RHS);
      /// Diffusion matrix is symmetric, no need to transpose
      // auto *DmatT = Dmat->Transpose();
      HypreBoomerAMG amg(*Dmat);
      amg.SetPrintLevel(0);
      HypreGMRES gmres(*Dmat);
      gmres.SetTol(1e-14);
      gmres.SetMaxIter(200);
      gmres.SetPrintLevel(-1);
      gmres.SetPreconditioner(amg);
      gmres.Mult(RHS, PSIK);

      D.RecoverFEMSolution(PSIK, GTMTpsi_a, psi_k);
   }

   /// compute psi_j
   /// M^T \psi_j = W^T \psi_k - M \psi_a
   ParMixedBilinearForm weakDiv(fes.get(), h1_space.get());
   weakDiv.AddDomainIntegrator(new VectorFEWeakDivergenceIntegrator);
   weakDiv.Assemble();
   weakDiv.Finalize();

   ParGridFunction WTpsik(fes.get());
   WTpsik = 0.0;
   weakDiv.MultTranspose(psi_k, WTpsik);

   ParGridFunction Mpsia(fes.get());
   Mpsia = 0.0;
   h_curl_mass.Mult(*adj, Mpsia);
   WTpsik.Add(-1.0, Mpsia);

   ParGridFunction psi_j(fes.get());
   psi_j = 0.0;
   {
      Vector PSIJ;
      Vector RHS;
      auto *M = new HypreParMatrix;
      Array<int> ess_tdof_list;
      h_curl_mass.FormLinearSystem(ess_tdof_list, psi_j, WTpsik,
                                   *M, PSIJ, RHS);

      HypreBoomerAMG amg(*M);
      amg.SetPrintLevel(0);
      HypreGMRES gmres(*M);
      gmres.SetTol(1e-14);
      gmres.SetMaxIter(200);
      gmres.SetPrintLevel(-1);
      gmres.SetPreconditioner(amg);
      gmres.Mult(RHS, PSIJ);

      h_curl_mass.RecoverFEMSolution(PSIJ, WTpsik, psi_j);
   }

   /// compute j
   LinearFormType J(fes.get());
   J.AddDomainIntegrator(new VectorFEDomainLFIntegrator(*current_coeff));
   J.Assemble();

   GridFunType j(fes.get());
   j = 0.0;
   // j.ProjectCoefficient(*current_coeff);
   {
      auto *M = new HypreParMatrix;
      Vector X, RHS;
      Array<int> ess_tdof_list;
      h_curl_mass.FormLinearSystem(ess_tdof_list, j, J, *M, X, RHS);
      HypreBoomerAMG amg(*M);
      amg.SetPrintLevel(0);
      HypreGMRES gmres(*M);
      gmres.SetTol(1e-12);
      gmres.SetMaxIter(200);
      gmres.SetPrintLevel(2);
      gmres.SetPreconditioner(amg);
      gmres.Mult(RHS, X);

      h_curl_mass.RecoverFEMSolution(X, J, j);
   }

   /// compute k
   ParGridFunction Wj(h1_space.get());
   Wj = 0.0;
   weakDiv.Mult(j, Wj);

   ParGridFunction k(h1_space.get());
   k = 0.0;
   {
      Vector K;
      Vector RHS;
      D.FormLinearSystem(ess_bdr_tdofs, k, Wj, *Dmat, K, RHS);

      HypreBoomerAMG amg(*Dmat);
      amg.SetPrintLevel(0);
      HypreGMRES gmres(*Dmat);
      gmres.SetTol(1e-14);
      gmres.SetMaxIter(200);
      gmres.SetPrintLevel(-1);
      gmres.SetPreconditioner(amg);
      gmres.Mult(RHS, K);

      D.RecoverFEMSolution(K, Wj, k);
   }

   SpaceType *mesh_fes = static_cast<SpaceType*>(mesh->GetNodes()->FESpace());

   ParLinearForm Rk_mesh_sens(mesh_fes);
   /// add integrators R_k = Dk - Wj = 0
   /// \psi_k^T Dk
   ConstantCoefficient one(1.0);
   Rk_mesh_sens.AddDomainIntegrator(
      new DiffusionResIntegrator(one, &k, &psi_k));
   /// -\psi_k^T W j 
   Rk_mesh_sens.AddDomainIntegrator(
      new VectorFEWeakDivergencedJdXIntegrator(&j, &psi_k, nullptr, -1.0));
   Rk_mesh_sens.Assemble();

   /// Add integrators R_{\hat{j}} = \hat{j} - MGk - Mj = 0
   ParLinearForm Rjhat_mesh_sens(mesh_fes);
   ParGridFunction Gk(fes.get());
   Gk = 0.0;
   grad.Mult(k, Gk);

   /// NOTE: Not using -1.0 here even though there are - signs in the residual
   /// because we're using adj, not psi_j_hat, which would be -adj
   Rjhat_mesh_sens.AddDomainIntegrator(
      new VectorFEMassdJdXIntegerator(&Gk, &psi_a));
   Rjhat_mesh_sens.AddDomainIntegrator(
      new VectorFEMassdJdXIntegerator(&j, &psi_a));
   Rjhat_mesh_sens.Assemble();

   /// add integrators R_j = Mj - J = 0
   ParLinearForm Rj_mesh_sens(mesh_fes);

   Rj_mesh_sens.AddDomainIntegrator(
      new VectorFEMassdJdXIntegerator(&j, &psi_j));
   Rj_mesh_sens.AddDomainIntegrator(
      new VectorFEDomainLFMeshSensInteg(&psi_j, *current_coeff, -1.0));
   Rj_mesh_sens.Assemble();

   mesh_sens.Add(1.0, *Rk_mesh_sens.ParallelAssemble());
   mesh_sens.Add(1.0, *Rjhat_mesh_sens.ParallelAssemble());
   mesh_sens.Add(1.0, *Rj_mesh_sens.ParallelAssemble());   
}

Vector* MagnetostaticSolver::getResidual()
{
   residual.reset(new GridFunType(fes.get()));
   *residual = 0.0;
   /// state needs to be the same as the current density changes, zero is arbitrary
   *u = 0.0;
   res->Mult(*u, *residual);
   *residual -= *current_vec;
   return residual.get();
}

Vector* MagnetostaticSolver::getResidualCurrentDensitySensitivity()
{
   current_density = 1.0;
   *current_vec = 0.0;
   constructCurrent();
   assembleCurrentSource();
   *current_vec *= -1.0;

   Array<int> ess_bdr(mesh->bdr_attributes.Size());
   Array<int> ess_tdof_list;
   ess_bdr = 1;
   fes->GetEssentialTrueDofs(ess_bdr, ess_tdof_list);
   /// set current vector's ess_tdofs to zero
   current_vec->SetSubVector(ess_tdof_list, 0.0);

   return current_vec.get();
}

double MagnetostaticSolver::getFunctionalCurrentDensitySensitivity(const std::string &fun)
{
   Array<int> ess_bdr(mesh->bdr_attributes.Size());
   ess_bdr = 1;
   res->SetEssentialBC(ess_bdr);
   solveForAdjoint(fun);

   double derivative = *adj * *getResidualCurrentDensitySensitivity();

   setStaticMembers();
   constructCurrent();
   constructMagnetization();
   assembleCurrentSource();
   assembleMagnetizationSource();

   return derivative;
}

void MagnetostaticSolver::assembleMagnetizationSource(void)
{
   M.reset(new GridFunType(h_div_space.get()));

   auto weakCurlMuInv_ = new ParMixedBilinearForm(h_div_space.get(), fes.get());
   weakCurlMuInv_->AddDomainIntegrator(new VectorFECurlIntegrator(*nu));

   weakCurlMuInv_->Assemble();
   weakCurlMuInv_->Finalize();

   M->ProjectCoefficient(*mag_coeff);
   weakCurlMuInv_->AddMult(*M, *current_vec, 1.0);

   delete weakCurlMuInv_;
}

void MagnetostaticSolver::computeSecondaryFields()
{
   // std::cout << "before curl constructed\n";
   DiscreteCurlOperator curl(fes.get(), h_div_space.get());
   // std::cout << "curl constructed\n";
   curl.Assemble();
   curl.Finalize();
   curl.Mult(*u, *B);
   // std::cout << "secondary quantities computed\n";

	// VectorFunctionCoefficient B_exact(3, b_exact);
	// GridFunType B_ex(h_div_space.get());
	// B_ex.ProjectCoefficient(B_exact);

   // GridFunType b_err(B_ex);
   // b_err -= *B;

   // printFields("B_exact", {&B_ex}, {"B_exact"});
   // std::cout << "B field error " << b_err.Norml2() << "\n";
}

// /// TODO: Find a better way to handle solving the simple box problem
// void MagnetostaticSolver::phase_a_source(const Vector &x,
//                                        Vector &J)
// {
//    // example of needed geometric parameters, this should be all you need
//    int n_s = 12; //number of slots
//    double zb = 0.0; //bottom of stator
//    double zt = 0.25; //top of stator


//    // compute r and theta from x and y
//    // double r = sqrt(x(0)*x(0) + x(1)*x(1)); (r not needed)
//    double tha = atan2(x(1), x(0));
//    double th;

//    double thw = 2*M_PI/n_s; //total angle of slot
//    int w; //current slot
//    J = 0.0;

//    // check which winding we're in
//    th = remquo(tha, thw, &w);

//    // check if we're in the stator body
//    if(x(2) >= zb && x(2) <= zt)
//    {
//       // check if we're in left or right half
//       if(th > 0)
//       {
//          J(2) = -1; // set to 1 for now, and direction depends on current direction
//       }
//       if(th < 0)
//       {
//          J(2) = 1;	
//       }
//    }
//    else  // outside of the stator body, check if above or below
//    {
//       // 'subtract' z position to 0 depending on if above or below
//       mfem::Vector rx(x);
//       if(x(2) > zt) 
//       {
//          rx(2) -= zt; 
//       }
//       if(x(2) < zb) 
//       {
//          rx(2) -= zb; 
//       }

//       // draw top rotation axis
//       mfem::Vector ax(3);
//       mfem::Vector Jr(3);
//       ax = 0.0;
//       ax(0) = cos(w*thw);
//       ax(1) = sin(w*thw);

//       // take x cross ax, normalize
//       Jr(0) = rx(1)*ax(2) - rx(2)*ax(1);
//       Jr(1) = rx(2)*ax(0) - rx(0)*ax(2);
//       Jr(2) = rx(0)*ax(1) - rx(1)*ax(0);
//       Jr /= Jr.Norml2();
//       J = Jr;
//    }
//    J *= current_density * fill_factor;
// }

// void MagnetostaticSolver::phase_b_source(const Vector &x,
//                                        Vector &J)
// {
//    // example of needed geometric parameters, this should be all you need
//    int n_s = 12; //number of slots
//    double zb = 0.0; //bottom of stator
//    double zt = 0.25; //top of stator


//    // compute r and theta from x and y
//    // double r = sqrt(x(0)*x(0) + x(1)*x(1)); (r not needed)
//    double tha = atan2(x(1), x(0));
//    double th;

//    double thw = 2*M_PI/n_s; //total angle of slot
//    int w; //current slot
//    J = 0.0;

//    // check which winding we're in
//    th = remquo(tha, thw, &w);

//    // check if we're in the stator body
//    if(x(2) >= zb && x(2) <= zt)
//    {
//       // check if we're in left or right half
//       if(th > 0)
//       {
//          J(2) = -1; // set to 1 for now, and direction depends on current direction
//       }
//       if(th < 0)
//       {
//          J(2) = 1;	
//       }
//    }
//    else  // outside of the stator body, check if above or below
//    {
//       // 'subtract' z position to 0 depending on if above or below
//       mfem::Vector rx(x);
//       if(x(2) > zt) 
//       {
//          rx(2) -= zt; 
//       }
//       if(x(2) < zb) 
//       {
//          rx(2) -= zb; 
//       }

//       // draw top rotation axis
//       mfem::Vector ax(3);
//       mfem::Vector Jr(3);
//       ax = 0.0;
//       ax(0) = cos(w*thw);
//       ax(1) = sin(w*thw);

//       // take x cross ax, normalize
//       Jr(0) = rx(1)*ax(2) - rx(2)*ax(1);
//       Jr(1) = rx(2)*ax(0) - rx(0)*ax(2);
//       Jr(2) = rx(0)*ax(1) - rx(1)*ax(0);
//       Jr /= Jr.Norml2();
//       J = Jr;
//    }
//    J *= -current_density * fill_factor;
// }

// void MagnetostaticSolver::phase_c_source(const Vector &x,
//                                        Vector &J)
// {
//    J.SetSize(3);
//    J = 0.0;
//    // Vector r = x;
//    // r(2) = 0.0;
//    // r /= r.Norml2();
//    // J(0) = -r(1);
//    // J(1) = r(0);
//    // J *= current_density;
// }

// /// TODO: Find a better way to handle solving the simple box problem
// /// TODO: implement other kinds of sources
// void MagnetostaticSolver::magnetization_source_north(const Vector &x,
//                                                    Vector &M)
// {
//    Vector plane_vec = x;
//    plane_vec(2) = 0;
//    M = plane_vec;
//    M /= M.Norml2();
//    M *= remnant_flux;
// }

// void MagnetostaticSolver::magnetization_source_south(const Vector &x,
//                                                    Vector &M)
// {
//    Vector plane_vec = x;
//    plane_vec(2) = 0;
//    M = plane_vec;
//    M /= M.Norml2();
//    M *= -remnant_flux;

//    // M = 0.0;
//    // M(2) = remnant_flux;
// }

// void MagnetostaticSolver::x_axis_current_source(const Vector &x,
//                                                 Vector &J)
// {
//    J.SetSize(3);
//    J = 0.0;
//    J(0) = current_density;
// }

// void MagnetostaticSolver::y_axis_current_source(const Vector &x,
//                                                 Vector &J)
// {
//    J.SetSize(3);
//    J = 0.0;
//    J(1) = current_density;   
// }

// void MagnetostaticSolver::z_axis_current_source(const Vector &x,
//                                                 Vector &J)
// {
//    J.SetSize(3);
//    J = 0.0;
//    J(2) = current_density;
// }

// void MagnetostaticSolver::ring_current_source(const Vector &x,
//                                               Vector &J)
// {
//    J.SetSize(3);
//    J = 0.0;
//    Vector r = x;
//    r(2) = 0.0;
//    r /= r.Norml2();
//    J(0) = -r(1);
//    J(1) = r(0);
//    J *= current_density;
// }

// void MagnetostaticSolver::x_axis_magnetization_source(const Vector &x,
//                                                       Vector &M)
// {
//    M.SetSize(3);
//    M = 0.0;
//    M(0) = remnant_flux;
// }

// void MagnetostaticSolver::y_axis_magnetization_source(const Vector &x,
//                                                       Vector &M)
// {
//    M.SetSize(3);
//    M = 0.0;
//    M(1) = remnant_flux;
// }

// void MagnetostaticSolver::z_axis_magnetization_source(const Vector &x,
//                                                       Vector &M)
// {
//    M.SetSize(3);
//    M = 0.0;
//    M(2) = remnant_flux;
// }

void MagnetostaticSolver::phaseACurrentSource(const Vector &x,
                                             Vector &J)
{
   phase_a_current(current_density, fill_factor, x.GetData(), J.GetData());
}

void MagnetostaticSolver::phaseACurrentSourceRevDiff(const Vector &x,
                                                     const Vector &V_bar,
                                                     Vector &x_bar)
{
   DenseMatrix source_jac(3);
   // declare vectors of active input variables
   std::vector<adouble> x_a(x.Size());
   // copy data from mfem::Vector
   adept::set_values(x_a.data(), x.Size(), x.GetData());
   // start recording
   diff_stack.new_recording();
   // the depedent variable must be declared after the recording
   std::vector<adouble> J_a(x.Size());
   phase_a_current<adouble>(current_density, fill_factor, x_a.data(), J_a.data());
   // set the independent and dependent variable
   diff_stack.independent(x_a.data(), x.Size());
   diff_stack.dependent(J_a.data(), x.Size());
   // calculate the jacobian w.r.t position
   diff_stack.jacobian(source_jac.GetData());
   source_jac.MultTranspose(V_bar, x_bar);
}

void MagnetostaticSolver::phaseBCurrentSource(const Vector &x,
                                             Vector &J)
{
   phase_b_current(current_density, fill_factor, x.GetData(), J.GetData());
}

void MagnetostaticSolver::phaseBCurrentSourceRevDiff(const Vector &x,
                                                     const Vector &V_bar,
                                                     Vector &x_bar)
{
   DenseMatrix source_jac(3);
   // declare vectors of active input variables
   std::vector<adouble> x_a(x.Size());
   // copy data from mfem::Vector
   adept::set_values(x_a.data(), x.Size(), x.GetData());
   // start recording
   diff_stack.new_recording();
   // the depedent variable must be declared after the recording
   std::vector<adouble> J_a(x.Size());
   phase_b_current<adouble>(current_density, fill_factor, x_a.data(), J_a.data());
   // set the independent and dependent variable
   diff_stack.independent(x_a.data(), x.Size());
   diff_stack.dependent(J_a.data(), x.Size());
   // calculate the jacobian w.r.t position
   diff_stack.jacobian(source_jac.GetData());
   source_jac.MultTranspose(V_bar, x_bar);
}

void MagnetostaticSolver::phaseCCurrentSource(const Vector &x,
                                             Vector &J)
{
   phase_c_current(current_density, fill_factor, x.GetData(), J.GetData());
}

void MagnetostaticSolver::phaseCCurrentSourceRevDiff(const Vector &x,
                                                     const Vector &V_bar,
                                                     Vector &x_bar)
{
   DenseMatrix source_jac(3);
   // declare vectors of active input variables
   std::vector<adouble> x_a(x.Size());
   // copy data from mfem::Vector
   adept::set_values(x_a.data(), x.Size(), x.GetData());
   // start recording
   diff_stack.new_recording();
   // the depedent variable must be declared after the recording
   std::vector<adouble> J_a(x.Size());
   phase_c_current<adouble>(current_density, fill_factor, x_a.data(), J_a.data());
   // set the independent and dependent variable
   diff_stack.independent(x_a.data(), x.Size());
   diff_stack.dependent(J_a.data(), x.Size());
   // calculate the jacobian w.r.t position
   diff_stack.jacobian(source_jac.GetData());
   source_jac.MultTranspose(V_bar, x_bar);
}

void MagnetostaticSolver::northMagnetizationSource(const Vector &x,
                                                   Vector &M)
{
   north_magnetization(remnant_flux, x.GetData(), M.GetData());
}

void MagnetostaticSolver::northMagnetizationSourceRevDiff(const Vector &x,
                                                          const Vector &V_bar,
                                                          Vector &x_bar)
{
   DenseMatrix source_jac(3);
   // declare vectors of active input variables
   std::vector<adouble> x_a(x.Size());
   // copy data from mfem::Vector
   adept::set_values(x_a.data(), x.Size(), x.GetData());
   // start recording
   diff_stack.new_recording();
   // the depedent variable must be declared after the recording
   std::vector<adouble> M_a(x.Size());
   north_magnetization<adouble>(remnant_flux, x_a.data(), M_a.data());
   // set the independent and dependent variable
   diff_stack.independent(x_a.data(), x.Size());
   diff_stack.dependent(M_a.data(), x.Size());
   // calculate the jacobian w.r.t position
   diff_stack.jacobian(source_jac.GetData());
   source_jac.MultTranspose(V_bar, x_bar);
}

void MagnetostaticSolver::southMagnetizationSource(const Vector &x,
                                                   Vector &M)
{
   south_magnetization(remnant_flux, x.GetData(), M.GetData());
}

void MagnetostaticSolver::southMagnetizationSourceRevDiff(const Vector &x,
                                                          const Vector &V_bar,
                                                          Vector &x_bar)
{
   DenseMatrix source_jac(3);
   // declare vectors of active input variables
   std::vector<adouble> x_a(x.Size());
   // copy data from mfem::Vector
   adept::set_values(x_a.data(), x.Size(), x.GetData());
   // start recording
   diff_stack.new_recording();
   // the depedent variable must be declared after the recording
   std::vector<adouble> M_a(x.Size());
   south_magnetization<adouble>(remnant_flux, x_a.data(), M_a.data());
   // set the independent and dependent variable
   diff_stack.independent(x_a.data(), x.Size());
   diff_stack.dependent(M_a.data(), x.Size());
   // calculate the jacobian w.r.t position
   diff_stack.jacobian(source_jac.GetData());
   source_jac.MultTranspose(V_bar, x_bar);
}

void MagnetostaticSolver::xAxisCurrentSource(const Vector &x,
                                             Vector &J)
{
   x_axis_current(current_density, x.GetData(), J.GetData());
}

void MagnetostaticSolver::xAxisCurrentSourceRevDiff(const Vector &x,
                                                    const Vector &V_bar,
                                                    Vector &x_bar)
{
   DenseMatrix source_jac(3);
   // declare vectors of active input variables
   std::vector<adouble> x_a(x.Size());
   // copy data from mfem::Vector
   adept::set_values(x_a.data(), x.Size(), x.GetData());
   // start recording
   diff_stack.new_recording();
   // the depedent variable must be declared after the recording
   std::vector<adouble> J_a(x.Size());
   x_axis_current<adouble>(current_density, x_a.data(), J_a.data());
   // set the independent and dependent variable
   diff_stack.independent(x_a.data(), x.Size());
   diff_stack.dependent(J_a.data(), x.Size());
   // calculate the jacobian w.r.t state vaiables
   diff_stack.jacobian(source_jac.GetData());
   source_jac.MultTranspose(V_bar, x_bar);
}

void MagnetostaticSolver::yAxisCurrentSource(const Vector &x,
                                             Vector &J)
{
   y_axis_current(current_density, x.GetData(), J.GetData());
}

void MagnetostaticSolver::yAxisCurrentSourceRevDiff(const Vector &x,
                                                    const Vector &V_bar,
                                                    Vector &x_bar)
{
   DenseMatrix source_jac(3);
   // declare vectors of active input variables
   std::vector<adouble> x_a(x.Size());
   // copy data from mfem::Vector
   adept::set_values(x_a.data(), x.Size(), x.GetData());
   // start recording
   diff_stack.new_recording();
   // the depedent variable must be declared after the recording
   std::vector<adouble> J_a(x.Size());
   y_axis_current<adouble>(current_density, x_a.data(), J_a.data());
   // set the independent and dependent variable
   diff_stack.independent(x_a.data(), x.Size());
   diff_stack.dependent(J_a.data(), x.Size());
   // calculate the jacobian w.r.t state vaiables
   diff_stack.jacobian(source_jac.GetData());
   source_jac.MultTranspose(V_bar, x_bar);
}

void MagnetostaticSolver::zAxisCurrentSource(const Vector &x,
                                             Vector &J)
{
   z_axis_current(current_density, x.GetData(), J.GetData());
}

void MagnetostaticSolver::zAxisCurrentSourceRevDiff(const Vector &x,
                                                    const Vector &V_bar,
                                                    Vector &x_bar)
{
   DenseMatrix source_jac(3);
   // declare vectors of active input variables
   std::vector<adouble> x_a(x.Size());
   // copy data from mfem::Vector
   adept::set_values(x_a.data(), x.Size(), x.GetData());
   // start recording
   diff_stack.new_recording();
   // the depedent variable must be declared after the recording
   std::vector<adouble> J_a(x.Size());
   z_axis_current<adouble>(current_density, x_a.data(), J_a.data());
   // set the independent and dependent variable
   diff_stack.independent(x_a.data(), x.Size());
   diff_stack.dependent(J_a.data(), x.Size());
   // calculate the jacobian w.r.t state vaiables
   diff_stack.jacobian(source_jac.GetData());
   source_jac.MultTranspose(V_bar, x_bar);
}

void MagnetostaticSolver::ringCurrentSource(const Vector &x,
                                            Vector &J)
{
   ring_current(current_density, x.GetData(), J.GetData());
}

void MagnetostaticSolver::ringCurrentSourceRevDiff(const Vector &x,
                                                   const Vector &V_bar,
                                                   Vector &x_bar)
{
   DenseMatrix source_jac(3);
   // declare vectors of active input variables
   std::vector<adouble> x_a(x.Size());
   // copy data from mfem::Vector
   adept::set_values(x_a.data(), x.Size(), x.GetData());
   // start recording
   diff_stack.new_recording();
   // the depedent variable must be declared after the recording
   std::vector<adouble> J_a(x.Size());
   ring_current<adouble>(current_density, x_a.data(), J_a.data());
   // set the independent and dependent variable
   diff_stack.independent(x_a.data(), x.Size());
   diff_stack.dependent(J_a.data(), x.Size());
   // calculate the jacobian w.r.t state vaiables
   diff_stack.jacobian(source_jac.GetData());
   source_jac.MultTranspose(V_bar, x_bar);
}

void MagnetostaticSolver::xAxisMagnetizationSource(const Vector &x,
                                                   Vector &M)
{
   x_axis_magnetization(remnant_flux, x.GetData(), M.GetData());
}

void MagnetostaticSolver::xAxisMagnetizationSourceRevDiff(const Vector &x,
                                                          const Vector &V_bar,
                                                          Vector &x_bar)
{
   DenseMatrix source_jac(3);
   // declare vectors of active input variables
   std::vector<adouble> x_a(x.Size());
   // copy data from mfem::Vector
   adept::set_values(x_a.data(), x.Size(), x.GetData());
   // start recording
   diff_stack.new_recording();
   // the depedent variable must be declared after the recording
   std::vector<adouble> M_a(x.Size());
   x_axis_magnetization<adouble>(remnant_flux, x_a.data(), M_a.data());
   // set the independent and dependent variable
   diff_stack.independent(x_a.data(), x.Size());
   diff_stack.dependent(M_a.data(), x.Size());
   // calculate the jacobian w.r.t state vaiables
   diff_stack.jacobian(source_jac.GetData());
   source_jac.MultTranspose(V_bar, x_bar);
}

void MagnetostaticSolver::yAxisMagnetizationSource(const Vector &x,
                                                   Vector &M)
{
   y_axis_magnetization(remnant_flux, x.GetData(), M.GetData());
}

void MagnetostaticSolver::yAxisMagnetizationSourceRevDiff(const Vector &x,
                                                          const Vector &V_bar,
                                                          Vector &x_bar)
{
   DenseMatrix source_jac(3);
   // declare vectors of active input variables
   std::vector<adouble> x_a(x.Size());
   // copy data from mfem::Vector
   adept::set_values(x_a.data(), x.Size(), x.GetData());
   // start recording
   diff_stack.new_recording();
   // the depedent variable must be declared after the recording
   std::vector<adouble> M_a(x.Size());
   y_axis_magnetization<adouble>(remnant_flux, x_a.data(), M_a.data());
   // set the independent and dependent variable
   diff_stack.independent(x_a.data(), x.Size());
   diff_stack.dependent(M_a.data(), x.Size());
   // calculate the jacobian w.r.t state vaiables
   diff_stack.jacobian(source_jac.GetData());
   source_jac.MultTranspose(V_bar, x_bar);
}

void MagnetostaticSolver::zAxisMagnetizationSource(const Vector &x,
                                                   Vector &M)
{
   z_axis_magnetization(remnant_flux, x.GetData(), M.GetData());
}

void MagnetostaticSolver::zAxisMagnetizationSourceRevDiff(const Vector &x,
                                                          const Vector &V_bar,
                                                          Vector &x_bar)
{
   DenseMatrix source_jac(3);
   // declare vectors of active input variables
   std::vector<adouble> x_a(x.Size());
   // copy data from mfem::Vector
   adept::set_values(x_a.data(), x.Size(), x.GetData());
   // start recording
   diff_stack.new_recording();
   // the depedent variable must be declared after the recording
   std::vector<adouble> M_a(x.Size());
   z_axis_magnetization<adouble>(remnant_flux, x_a.data(), M_a.data());
   // set the independent and dependent variable
   diff_stack.independent(x_a.data(), x.Size());
   diff_stack.dependent(M_a.data(), x.Size());
   // calculate the jacobian w.r.t state vaiables
   diff_stack.jacobian(source_jac.GetData());
   source_jac.MultTranspose(V_bar, x_bar);
}

void MagnetostaticSolver::box1CurrentSource(const Vector &x,
                                            Vector &J)
{
   box1_current(current_density, x.GetData(), J.GetData());
}

void MagnetostaticSolver::box1CurrentSourceRevDiff(
   const Vector &x,
   const Vector &V_bar,
   Vector &x_bar)
{
   DenseMatrix source_jac(3);
   // declare vectors of active input variables
   std::vector<adouble> x_a(x.Size());
   // copy data from mfem::Vector
   adept::set_values(x_a.data(), x.Size(), x.GetData());
   // start recording
   diff_stack.new_recording();
   // the depedent variable must be declared after the recording
   std::vector<adouble> J_a(x.Size());
   box1_current<adouble>(current_density, x_a.data(), J_a.data());
   // set the independent and dependent variable
   diff_stack.independent(x_a.data(), x.Size());
   diff_stack.dependent(J_a.data(), x.Size());
   // calculate the jacobian w.r.t state vaiables
   diff_stack.jacobian(source_jac.GetData());
   source_jac.MultTranspose(V_bar, x_bar);
}

void MagnetostaticSolver::box2CurrentSource(const Vector &x,
                                      Vector &J)
{
   box2_current(current_density, x.GetData(), J.GetData());
}

void MagnetostaticSolver::box2CurrentSourceRevDiff(
   const Vector &x,
   const Vector &V_bar,
   Vector &x_bar)
{
   DenseMatrix source_jac(3);
   // declare vectors of active input variables
   std::vector<adouble> x_a(x.Size());
   // copy data from mfem::Vector
   adept::set_values(x_a.data(), x.Size(), x.GetData());
   // start recording
   diff_stack.new_recording();
   // the depedent variable must be declared after the recording
   std::vector<adouble> J_a(x.Size());
   box2_current<adouble>(current_density, x_a.data(), J_a.data());
   // set the independent and dependent variable
   diff_stack.independent(x_a.data(), x.Size());
   diff_stack.dependent(J_a.data(), x.Size());
   // calculate the jacobian w.r.t state vaiables
   diff_stack.jacobian(source_jac.GetData());
   source_jac.MultTranspose(V_bar, x_bar);
}

void MagnetostaticSolver::a_exact(const Vector &x, Vector &A)
{
   A.SetSize(3);
   A = 0.0;
   double y = x(1) - .5;
   if ( x(1) <= .5)
   {
      A(2) = y*y*y; 
      // A(2) = y*y; 
   }
   else 
   {
      A(2) = -y*y*y;
      // A(2) = -y*y;
   }
}

void MagnetostaticSolver::b_exact(const Vector &x, Vector &B)
{
   B.SetSize(3);
   B = 0.0;
   double y = x(1) - .5;
   if ( x(1) <= .5)
   {
      B(0) = 3*y*y; 
      // B(0) = 2*y; 
   }
   else 
   {
      B(0) = -3*y*y;
      // B(0) = -2*y;
   }	
}

double MagnetostaticSolver::remnant_flux = 0.0;
double MagnetostaticSolver::mag_mu_r = 0.0;
double MagnetostaticSolver::fill_factor = 0.0;
double MagnetostaticSolver::current_density = 0.0;

} // namespace mach
