#ifndef MACH_EULER_DG_CUT_SENS
#define MACH_EULER_DG_CUT_SENS


#include "mfem.hpp"
#include "solver.hpp"
#include "euler_integ_dg.hpp"
#include "euler_integ_dg_cut.hpp"
#include "euler_integ_dg_cut_sens.hpp"
#include "diag_mass_integ.hpp"
#include "euler_integ.hpp"
#include "functional_output.hpp"
#include "sbp_fe.hpp"
#include "utils.hpp"
#include "euler_dg.hpp"
#include <chrono>

namespace mach
{
/// Solver for inviscid flow problems
/// dim - number of spatial dimensions (1, 2, or 3)
/// entvar - if true, the entropy variables are used in the integrators
template <int dim, bool entvar = false>
class CutEulerDGSensitivitySolver : public AbstractSolver
{
public:
   /// Find the global time step size
   /// \param[in] iter - the current iteration
   /// \param[in] t - the current time (before the step)
   /// \param[in] t_final - the final time
   /// \param[in] dt_old - the step size that was just taken
   /// \param[in] state - the current state
   /// \returns dt - appropriate step size
   /// \note If "const-cfl" option is invoked, this uses the average spectral
   /// radius to estimate the largest wave speed, and uses the minimum distance
   /// between nodes for the length in the CFL number.
   /// \note If "steady" option is involved, the time step will increase based
   /// on the baseline value of "dt" and the inverse residual norm.
   double calcStepSize(int iter,
                       double t,
                       double t_final,
                       double dt_old,
                       const mfem::ParGridFunction &state) const override;

   /// Find the global time step size
   /// \param[in] iter - the current iteration
   /// \param[in] t - the current time (before the step)
   /// \param[in] t_final - the final time
   /// \param[in] dt_old - the step size that was just taken
   /// \param[in] state - the current `gd` state
   /// \returns dt - appropriate step size
   /// \note If "const-cfl" option is invoked, this uses the average spectral
   /// radius to estimate the largest wave speed, and uses the minimum distance
   /// between nodes for the length in the CFL number.
   /// \note If "steady" option is involved, the time step will increase based
   /// on the baseline value of "dt" and the inverse residual norm.
   double calcStepSize(int iter,
                       double t,
                       double t_final,
                       double dt_old,
                       const mfem::ParCentGridFunction &state) const override;
   /// Sets `q_ref` to the free-stream conservative variables
   void getFreeStreamState(mfem::Vector &q_ref);

   /// Returns the L2 error between the discrete and exact conservative vars.
   /// \param[in] u_exact - function that defines the exact **state**
   /// \param[in] entry - if >= 0, the L2 error of state `entry` is returned
   /// \returns L2 error
   /// \note The solution given by `u_exact` is for the state, conservative or
   /// entropy variables.  **Do not give the exact solution for the conservative
   /// variables if using entropy variables**.   The conversion to conservative
   /// variables is done by this function.
   double calcConservativeVarsL2Error(void (*u_exact)(const mfem::Vector &,
                                                      mfem::Vector &),
                                      int entry = -1);

   /// convert conservative variables to entropy variables
   /// \param[in/out] state - the conservative/entropy variables
   virtual void convertToEntvar(mfem::Vector &state);

   /// Sets `u` to the difference between it and a given exact solution
   /// \param[in] u_exact - function that defines the exact solution
   /// \note The second argument in the function `u_exact` is the field value.
   /// \warning This overwrites the solution in `u`!!!
   void setSolutionError(void (*u_exact)(const mfem::Vector &, mfem::Vector &));

protected:
   /// free-stream Mach number
   double mach_fs;
   /// free-stream angle of attack
   double aoa_fs;
   /// index of dimension corresponding to nose to tail axis
   int iroll;
   /// index of "vertical" dimension in body frame
   int ipitch;
   /// used to record the entropy
   std::ofstream entropylog;
   /// used to store the initial residual norm for PTC and convergence checks
   double res_norm0 = -1.0;

   /// related to cut-cell integrators
   // int rule for cut elements
   std::map<int, IntegrationRule *> cutSquareIntRules;
   std::map<int, IntegrationRule *> cutSquareIntRules_p;
   std::map<int, IntegrationRule *> cutSquareIntRules_m;
   /// for vortex case  
   // int rule for cut elements by outer circle
   std::map<int, IntegrationRule *> cutSquareIntRules_outer;
   // int rule for embedded boundary
   std::map<int, IntegrationRule *> cutSegmentIntRules;
   std::map<int, IntegrationRule *> cutSegmentIntRules_p;
   std::map<int, IntegrationRule *> cutSegmentIntRules_m;
   /// for vortex case
   std::map<int, IntegrationRule *> cutSegmentIntRules_inner;
   std::map<int, IntegrationRule *> cutSegmentIntRules_outer;
   // interior face int rule that is cut by the embedded geometry
   std::map<int, IntegrationRule *> cutInteriorFaceIntRules;
   std::map<int, IntegrationRule *> cutInteriorFaceIntRules_p;
   std::map<int, IntegrationRule *> cutInteriorFaceIntRules_m;
   // interior face int rule sensitivities
   std::map<int, IntegrationRule *> cutInteriorFaceIntRules_sens;
   std::map<int, IntegrationRule *> cutInteriorFaceIntRules_sens_p;
   std::map<int, IntegrationRule *> cutInteriorFaceIntRules_sens_m;
   // interior face int rule that is cut by the embedded geometry
   std::map<int, IntegrationRule *> cutInteriorFaceIntRules_outer;
   // boundary face int rule that is cut by the embedded geometry
   std::map<int, IntegrationRule *> cutBdrFaceIntRules;
   // boundary face int rule that is cut by the embedded geometry
   std::map<int, IntegrationRule *> cutBdrFaceIntRules_outer;
   /// int rule senstivities
   // cut elements
   std::map<int, IntegrationRule *> cutSquareIntRules_sens;
   std::map<int, IntegrationRule *> cutSquareIntRules_sens_p;
   std::map<int, IntegrationRule *> cutSquareIntRules_sens_m;
   // embedded boundary
   std::map<int, IntegrationRule *> cutSegmentIntRules_sens;
   std::map<int, IntegrationRule *> cutSegmentIntRules_sens_p;
   std::map<int, IntegrationRule *> cutSegmentIntRules_sens_m;
   /// embedded elements boolean vector
   std::vector<bool> embeddedElements;
   std::vector<bool> embeddedElements_p;
   std::vector<bool> embeddedElements_m;
   /// cut elements boolean vector
   std::vector<bool> cutElements;
   std::vector<bool> cutElements_p;
   std::vector<bool> cutElements_m;
   // vector of cut interior faces
   std::vector<int> cutInteriorFaces;
   std::vector<int> cutInteriorFaces_p;
   std::vector<int> cutInteriorFaces_m;
   // vector of cut interior faces by outer circle
   std::vector<int> cutInteriorFaces_outer;
   // tells if face is immersed
   std::map<int, bool> immersedFaces;
   // tells if face is immersed
   std::map<int, bool> immersedFaces_p;
   std::map<int, bool> immersedFaces_m;
   // find the elements cut by geometry
   std::vector<int> cutelems;
   // find the elements cut by geometry
   std::vector<int> cutelems_p;
   // find the elements cut by geometry
   std::vector<int> cutelems_m;
    // find the elements cut by geometry
   std::vector<int> cutelems_outer;
   /// domain boundary faces cut by geometry  
   vector<int> cutFaces;
   /// domain boundary faces cut by geometry  
   vector<int> cutBdrFaces;
   /// domain boundary faces cut by geometry  
   vector<int> cutBdrFaces_outer;
   /// levelset to calculate normal vectors
   // Algoim::LevelSet<2> phi_e;
   LevelSetF<double, 2> phi;
   LevelSetF<double, 2> phi_outer;
   /// @brief  for sensitivity
   LevelSetF<double, 2> phi_p;
   LevelSetF<double, 2> phi_m;
   bool vortex = false;
   std::unique_ptr<GDSpaceType> fes_gd_p;
   std::unique_ptr<GDSpaceType> fes_gd_m;
   std::unique_ptr<NonlinearFormType> res_p;
   std::unique_ptr<NonlinearFormType> res_m;
   // Algoim::LevelSet<2> phi;
   // Algoim::LevelSet<2> phi_outer;
   /// Class constructor (protected to prevent misuse)
   /// \param[in] json_options - json object containing the options
   /// \param[in] smesh - if provided, defines the mesh for the problem
   CutEulerDGSensitivitySolver(const nlohmann::json &json_options,
                    std::unique_ptr<mfem::Mesh> smesh,
                    MPI_Comm comm);

   /// Initialize `res` and either `mass` or `nonlinear_mass`
   void constructForms() override;
   
   /// Add domain integrators to `mass`
   /// \param[in] alpha - scales the data; used to move terms to rhs or lhs
   void addMassIntegrators(double alpha) override;

   /// Add domain integrator to the nonlinear mass operator
   /// \param[in] alpha - scales the data; used to ove terems to rhs or lhs
   void addNonlinearMassIntegrators(double alpha) override;

   /// Add volume integrators to `res` based on `options`
   /// \param[in] alpha - scales the data; used to move terms to rhs or lhs
   void addResVolumeIntegrators(double alpha) override
   {} 
   
   /// Add volume integrators to `res` based on `options`
   /// \param[in] alpha - scales the data; used to move terms to rhs or lhs
   void addResVolumeIntegrators(double alpha, double &diff_coeff) override;
   /// Add boundary-face integrators to `res` based on `options`
   /// \param[in] alpha - scales the data; used to move terms to rhs or lhs
   void addResBoundaryIntegrators(double alpha) override;

   /// Add interior-face integrators to `res` based on `options`
   /// \param[in] alpha - scales the data; used to move terms to rhs or lhs
   void addResInterfaceIntegrators(double alpha) override;

   void addEntVolumeIntegrators() override;

   void addOutput(const std::string &fun,
                  const nlohmann::json &options) override;
   /// Return the number of state variables
   int getNumState() override { return dim + 2; }
   
   /// sets the GD fespace 
   /// \param[in] order - the order of discretization
   void setGDSpace(int order) override; 
   /// For code that should be executed before the time stepping begins
   /// \param[in] state - the current state
   void initialHook(const mfem::ParGridFunction &state) override;

   /// For code that should be executed before the time stepping begins
   /// \param[in] state - the current `gd` state
   void initialHook(const mfem::ParCentGridFunction &state) override;

   /// For code that should be executed before `ode_solver->Step`
   /// \param[in] iter - the current iteration
   /// \param[in] t - the current time (before the step)
   /// \param[in] dt - the step size that will be taken
   /// \param[in] state - the current state
   void iterationHook(int iter,
                      double t,
                      double dt,
                      const mfem::ParGridFunction &state) override;

   /// Determines when to exit the time stepping loop
   /// \param[in] iter - the current iteration
   /// \param[in] t - the current time (after the step)
   /// \param[in] t_final - the final time
   /// \param[in] dt - the step size that was just taken
   /// \param[in] state - the current state
   bool iterationExit(int iter,
                      double t,
                      double t_final,
                      double dt,
                      const mfem::ParGridFunction &state) const override;

   /// Determines when to exit the time stepping loop
   /// \param[in] iter - the current iteration
   /// \param[in] t - the current time (after the step)
   /// \param[in] t_final - the final time
   /// \param[in] dt - the step size that was just taken
   /// \param[in] state - the current `gd` state
   bool iterationExit(int iter,
                      double t,
                      double t_final,
                      double dt,
                      const mfem::ParCentGridFunction &state) const override;

   /// For code that should be executed after the time stepping ends
   /// \param[in] iter - the terminal iteration
   /// \param[in] t_final - the final time
   /// \param[in] state - the current state
   void terminalHook(int iter,
                     double t_final,
                     const mfem::ParGridFunction &state) override;

   friend SolverPtr createSolver<CutEulerDGSensitivitySolver<dim, entvar>>(
       const nlohmann::json &json_options,
       std::unique_ptr<mfem::Mesh> smesh,
       MPI_Comm comm);
};
void inviscidMMSExact(const mfem::Vector &x, mfem::Vector &q);
}  // namespace mach

#endif
