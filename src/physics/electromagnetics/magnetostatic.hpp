#ifndef MISO_MAGNETOSTATIC
#define MISO_MAGNETOSTATIC

#include <memory>
#include <mpi.h>

#include "coefficient.hpp"
#include "mfem.hpp"
#include "nlohmann/json.hpp"

#include "miso_residual.hpp"
#include "magnetostatic_residual.hpp"
#include "mfem_extensions.hpp"
#include "pde_solver.hpp"
#include "reluctivity_coefficient.hpp"
#include "conductivity_coefficient.hpp"
#include "cal2_kh_coefficient.hpp"
#include "cal2_ke_coefficient.hpp"
#include "magnetic_source_functions.hpp"
#include "demag_flux_coefficient.hpp"

namespace miso
{
/// Solver for magnetostatic electromagnetic problems
class MagnetostaticSolver : public PDESolver
{
public:
   MagnetostaticSolver(MPI_Comm comm,
                       const nlohmann::json &solver_options,
                       std::unique_ptr<mfem::Mesh> smesh = nullptr);

private:
   /// Coefficient representing the potentially nonlinear magnetic reluctivity
   ReluctivityCoefficient nu;
   // /// Material dependent coefficient representing density
   // MeshDependentCoefficient rho;
   /// Coefficient representing the temperature-dependent electrical
   /// conductivity
   ConductivityCoefficient sigma;
   // /// Coefficient representing the magnetization in the permanent magnets
   // MagnetizationCoefficient mag_coeff;
   // /// Coefficient representing the knee point flux density
   // DemagFluxCoefficient B_knee;

   // /// For code that should be executed before the time stepping begins
   // /// \param[in] state - the current state
   // void derivedPDEInitialHook(const mfem::Vector &state) override;

   // /// Code that should be executed each time step, before `ode_solver->Step`
   // /// \param[in] iter - the current iteration
   // /// \param[in] t - the current time (before the step)
   // /// \param[in] dt - the step size that will be taken
   // /// \param[in] state - the current state
   // void derivedPDEIterationHook(int iter,
   //                                      double t,
   //                                      double dt,
   //                                      const mfem::Vector &state) override;

   /// Code that should be executed after time stepping ends
   /// \param[in] iter - the terminal iteration
   /// \param[in] t_final - the final time
   /// \param[in] state - the current state
   void derivedPDETerminalHook(int iter,
                               double t_final,
                               const mfem::Vector &state) override;

   // /// Find the step size based on the options; e.g. for constant CFL or PTC
   // /// \param[in] iter - the current iteration
   // /// \param[in] t - the current time (before the step)
   // /// \param[in] t_final - the final time
   // /// \param[in] dt_old - the step size that was just taken
   // /// \param[in] state - the current state
   // /// \returns dt - the step size appropriate to the problem
   // /// \note If "const-cfl" option is invoked, this uses the average spectral
   // /// radius to estimate the largest wave speed, and uses the minimum
   // distance
   // /// between nodes for the length in the CFL number.
   // /// \note If the "steady" option is true, the time step will increase
   // based
   // /// on the baseline value of "dt" and the residual norm.
   // double calcStepSize(int iter,
   //                     double t,
   //                     double t_final,
   //                     double dt_old,
   //                     const mfem::Vector &state) const override;

   // /// Determines when to exit the time stepping loop
   // /// \param[in] iter - the current iteration
   // /// \param[in] t - the current time (after the step)
   // /// \param[in] t_final - the final time
   // /// \param[in] dt - the step size that was just taken
   // /// \param[in] state - the current state
   // /// \note If a steady problem is being solved, the "steady-abstol" and
   // /// "steady-reltol" options from "time-dis" to determine convergence.
   // bool iterationExit(int iter,
   //                    double t,
   //                    double t_final,
   //                    double dt,
   //                    const mfem::Vector &state) const override;

   /// Add output @a fun based on @a options
   void addOutput(const std::string &fun,
                  const nlohmann::json &options) override;
};

//    /// Class constructor.
//    /// \param[in] opt_file_name - file where options are stored
//    /// \param[in] smesh - if provided, defines the mesh for the problem
//    /// \param[in] comm - MPI communicator for parallel operations
//    MagnetostaticSolver(const nlohmann::json &json_options,
//                        std::unique_ptr<mfem::Mesh> smesh,
//                        MPI_Comm comm);

//    /// Class constructor.
//    /// \param[in] options - pre-loaded JSON options object
//    /// \param[in] smesh - if provided, defines the mesh for the problem
//    MagnetostaticSolver(const nlohmann::json &options,
//                        std::unique_ptr<mfem::Mesh> smesh);

//    void calcCurl(const mfem::HypreParVector &A, mfem::HypreParVector &B);

//    /// Write the mesh and solution to a vtk file
//    /// \param[in] file_name - prefix file name **without** .vtk extension
//    /// \param[in] refine - if >=0, indicates the number of refinements to
//    make
//    /// \note the `refine` argument is useful for high-order meshes and
//    /// solutions; it divides the elements up so it is possible to visualize.
//    void printSolution(const std::string &file_name, int refine = -1)
//    override;

//    /// \brief Returns a vector of pointers to grid functions that define
//    fields
//    /// returns {A, B}
//    std::vector<GridFunType *> getFields() override;

//    /// TODO: have this accept a string input chosing the functional
//    /// Compute the sensitivity of the functional to the mesh volume
//    /// nodes, using appropriate mesh sensitivity integrators. This function
//    will
//    /// compute the adjoint.
//    // mfem::GridFunction *getMeshSensitivities() override;

//    /// perturb the whole mesh and finite difference
//    void verifyMeshSensitivities();

//    void Update() override;

//    void setFieldValue(
//        mfem::HypreParVector &field,
//        const std::function<void(const mfem::Vector &, mfem::Vector &)>
//        &u_init) override;

//    /// TODO: rename this and other related functions to set BoundaryCondition
//    /// Initializes the state vector to a given vector.
//    /// \param[in] state - the state vector to initialize
//    /// \param[in] u_init - const vector that defines the initial condition
//    void setInitialCondition(mfem::ParGridFunction &state,
//                             const mfem::Vector &u_init) override;

//    /// Initializes the state vector to a given function.
//    /// \param[in] state - the state vector to initialize
//    /// \param[in] u_init - function that defines the initial condition
//    /// \note The second argument in the function `u_init` is the initial
//    /// condition value.  This may be a vector of length 1 for scalar.
//    void setInitialCondition(
//        mfem::ParGridFunction &state,
//        const std::function<void(const mfem::Vector &, mfem::Vector &)>
//        &u_init) override;

//    double calcStepSize(int iter,
//                        double t,
//                        double t_final,
//                        double dt_old,
//                        const mfem::ParGridFunction &state) const override;

// private:
//    // // /// Nedelec finite element collection
//    // // std::unique_ptr<mfem::FiniteElementCollection> h_curl_coll;
//    // /// Raviart-Thomas finite element collection
//    // std::unique_ptr<mfem::FiniteElementCollection> h_div_coll;
//    // /// H1 finite element collection
//    // std::unique_ptr<mfem::FiniteElementCollection> h1_coll;
//    // ///L2 finite element collection
//    // std::unique_ptr<mfem::FiniteElementCollection> l2_coll;

//    // // /// H(Curl) finite element space
//    // // std::unique_ptr<SpaceType> h_curl_space;
//    // /// H(Div) finite element space
//    // std::unique_ptr<SpaceType> h_div_space;
//    // /// H1 finite element space
//    // std::unique_ptr<SpaceType> h1_space;
//    // /// L2 finite element space
//    // std::unique_ptr<SpaceType> l2_space;

//    // // /// Magnetic vector potential A grid function
//    // // std::unique_ptr<GridFunType> A;

//    /// alias to magnetic flux density grid function stored in res_fields
//    mfem::ParGridFunction *B;

//    // /// Magnetic flux density B = curl(A) grid function in H(curl) space
//    // std::unique_ptr<GridFunType> B_dual;
//    // /// Magnetization grid function
//    // std::unique_ptr<GridFunType> M;

//    std::unique_ptr<MagnetostaticLoad> magnetostatic_load;

//    /// current source vector
//    // std::unique_ptr<GridFunType> current_vec;
//    std::unique_ptr<GridFunType> div_free_current_vec;

//    /// mesh dependent reluctivity coefficient
//    std::unique_ptr<MeshDependentCoefficient> nu;
//    /// vector mesh dependent current density function coefficient
//    std::unique_ptr<VectorMeshDependentCoefficient> current_coeff;
//    /// vector mesh dependent magnetization coefficient
//    std::unique_ptr<VectorMeshDependentCoefficient> mag_coeff;
//    /// mesh dependent electrical conductivity coefficient
//    std::unique_ptr<MeshDependentCoefficient> sigma;

//    /// boundary condition marker array
//    // mfem::Array<int> ess_bdr;
//    std::unique_ptr<mfem::VectorCoefficient> bc_coef;

//    int dim;
//    void constructForms() override;

//    /// Construct various coefficients
//    void constructCoefficients() override;

//    void addMassIntegrators(double alpha) override;
//    void addResVolumeIntegrators(double alpha) override;
//    void assembleLoadVector(double alpha) override;
//    void addEntVolumeIntegrators() override;

//    /// mark which boundaries are essential
//    void setEssentialBoundaries() override;

//    int getNumState() override { return 1; }

//    double res_norm0 = -1.0;
//    void initialHook(const mfem::ParGridFunction &state) override;

//    // void iterationHook(int iter, double t, double dt,
//    //                    const mfem::ParGridFunction &state) override;

//    bool iterationExit(int iter,
//                       double t,
//                       double t_final,
//                       double dt,
//                       const mfem::ParGridFunction &state) const override;

//    void terminalHook(int iter,
//                      double t_final,
//                      const mfem::ParGridFunction &state) override;

//    void addOutput(const std::string &fun,
//                   const nlohmann::json &options) override;

//    /// Solve nonlinear magnetostatics problem using an MFEM Newton solver
//    void solveUnsteady(mfem::ParGridFunction &state) override;
//    void _solveUnsteady(mfem::ParGridFunction &state);

//    /// static member variables used inside static member functions
//    /// magnetization_source and winding_current_source
//    /// values set by options in setStaticMembers
//    static double remnant_flux;
//    static double mag_mu_r;
//    static double fill_factor;
//    static double current_density;

//    /// set the values of static member variables used based on options file
//    void setStaticMembers();

//    /// construct mesh dependent coefficient for reluctivity
//    /// \param[in] alpha - used to move to lhs or rhs
//    void constructReluctivity();

//    /// construct mesh dependent coefficient for magnetization
//    /// \param[in] alpha - used to move to lhs or rhs
//    void constructMagnetization();

//    /// construct vector mesh dependent coefficient for current source
//    /// \param[in] alpha - used to move to lhs or rhs
//    void constructCurrent();

//    void constructSigma();

// public:
//    /// TODO: throw MISOException if constructCurrent not called first
//    ///       introdue some current constructed flag?
//    /// assemble vector associated with current source
//    /// \note - constructCurrent must be called before calling this
//    void assembleCurrentSource();

//    /// Assemble mesh sensitivities of current source vector
//    /// \param[in] psi_a - Adjoint vector
//    /// \param[out] mesh_sens - mesh sensitivitites
//    /// \note this method will not initialize mesh_sens, but will add to it
//    void getCurrentSourceMeshSens(const mfem::GridFunction &psi_a,
//                                  mfem::Vector &mesh_sens);

//    std::unique_ptr<GridFunType> residual;
//    /// return the residual as a vector
//    mfem::Vector *getResidual();

//    // /// Get the derivative of the residual with respect to the current
//    density
//    // mfem::Vector *getResidualCurrentDensitySensitivity();

//    /// Get the total derivative of a functional with respect to the current
//    /// density
//    /// \param[in] fun - which functional to get sensitivity with respect to
//    // double getFunctionalCurrentDensitySensitivity(const std::string &fun);

// private:
//    /// TODO: throw MISOException if constructMagnetization or
//    ///       assembleCurrentSource not called first
//    /// \brief assemble magnetization source terms into rhs vector and add
//    them
//    ///        with the current source terms
//    /// \note - constructMagnetization must be called before calling this
//    void assembleMagnetizationSource();

//    /// Function to compute seconday fields
//    /// For magnetostatics, computes the magnetic flux density
//    void computeSecondaryFields(const mfem::ParGridFunction &state);

//    /// function describing current density in phase A windings
//    /// \param[in] x - position x in space of evaluation
//    /// \param[out] J - current density at position x
//    static void phaseACurrentSource(const mfem::Vector &x, mfem::Vector &J);

//    static void phaseACurrentSourceRevDiff(const mfem::Vector &x,
//                                           const mfem::Vector &V_bar,
//                                           mfem::Vector &x_bar);

//    /// function describing current density in phase B windings
//    /// \param[in] x - position x in space of evaluation
//    /// \param[out] J - current density at position x
//    static void phaseBCurrentSource(const mfem::Vector &x, mfem::Vector &J);

//    static void phaseBCurrentSourceRevDiff(const mfem::Vector &x,
//                                           const mfem::Vector &V_bar,
//                                           mfem::Vector &x_bar);

//    /// function describing current density in phase C windings
//    /// \param[in] x - position x in space of evaluation
//    /// \param[out] J - current density at position x
//    static void phaseCCurrentSource(const mfem::Vector &x, mfem::Vector &J);

//    static void phaseCCurrentSourceRevDiff(const mfem::Vector &x,
//                                           const mfem::Vector &V_bar,
//                                           mfem::Vector &x_bar);

//    /// function describing permanent magnet magnetization pointing outwards
//    /// \param[in] x - position x in space
//    /// \param[out] M - magetic flux density at position x cause by permanent
//    ///                 magnets
//    static void northMagnetizationSource(const mfem::Vector &x, mfem::Vector
//    &M);

//    /// \param[in] x - position x in space of evaluation
//    /// \param[in] V_bar -
//    /// \param[out] x_bar - V_bar^T Jacobian
//    static void northMagnetizationSourceRevDiff(const mfem::Vector &x,
//                                                const mfem::Vector &V_bar,
//                                                mfem::Vector &x_bar);

//    /// function describing permanent magnet magnetization pointing inwards
//    /// \param[in] x - position x in space
//    /// \param[out] M - magetic flux density at position x cause by permanent
//    ///                 magnets
//    static void southMagnetizationSource(const mfem::Vector &x, mfem::Vector
//    &M);

//    /// \param[in] x - position x in space of evaluation
//    /// \param[in] V_bar -
//    /// \param[out] x_bar - V_bar^T Jacobian
//    static void southMagnetizationSourceRevDiff(const mfem::Vector &x,
//                                                const mfem::Vector &V_bar,
//                                                mfem::Vector &x_bar);

//    /// function describing permanent magnet magnetization pointing inwards
//    /// \param[in] x - position x in space
//    /// \param[out] M - magetic flux density at position x cause by permanent
//    ///                 magnets
//    static void cwMagnetizationSource(const mfem::Vector &x, mfem::Vector &M);

//    /// \param[in] x - position x in space of evaluation
//    /// \param[in] V_bar -
//    /// \param[out] x_bar - V_bar^T Jacobian
//    static void cwMagnetizationSourceRevDiff(const mfem::Vector &x,
//                                             const mfem::Vector &V_bar,
//                                             mfem::Vector &x_bar);

//    /// function describing permanent magnet magnetization pointing inwards
//    /// \param[in] x - position x in space
//    /// \param[out] M - magetic flux density at position x cause by permanent
//    ///                 magnets
//    static void ccwMagnetizationSource(const mfem::Vector &x, mfem::Vector
//    &M);

//    /// \param[in] x - position x in space of evaluation
//    /// \param[in] V_bar -
//    /// \param[out] x_bar - V_bar^T Jacobian
//    static void ccwMagnetizationSourceRevDiff(const mfem::Vector &x,
//                                              const mfem::Vector &V_bar,
//                                              mfem::Vector &x_bar);

//    /// function defining current density aligned with the x axis
//    /// \param[in] x - position x in space of evaluation
//    /// \param[out] J - current density at position x
//    static void xAxisCurrentSource(const mfem::Vector &x, mfem::Vector &J);

//    static void xAxisCurrentSourceRevDiff(const mfem::Vector &x,
//                                          const mfem::Vector &V_bar,
//                                          mfem::Vector &x_bar);

//    /// function defining current density aligned with the x axis
//    /// \param[in] x - position x in space of evaluation
//    /// \param[out] J - current density at position x
//    static void yAxisCurrentSource(const mfem::Vector &x, mfem::Vector &J);

//    static void yAxisCurrentSourceRevDiff(const mfem::Vector &x,
//                                          const mfem::Vector &V_bar,
//                                          mfem::Vector &x_bar);

//    /// function defining current density aligned with the z axis
//    /// \param[in] x - position x in space of evaluation
//    /// \param[out] J - current density at position x
//    static void zAxisCurrentSource(const mfem::Vector &x, mfem::Vector &J);

//    static void zAxisCurrentSourceRevDiff(const mfem::Vector &x,
//                                          const mfem::Vector &V_bar,
//                                          mfem::Vector &x_bar);

//    /// function defining current density aligned with the -z axis
//    /// \param[in] x - position x in space of evaluation
//    /// \param[out] J - current density at position x
//    static void nzAxisCurrentSource(const mfem::Vector &x, mfem::Vector &J);

//    static void nzAxisCurrentSourceRevDiff(const mfem::Vector &x,
//                                           const mfem::Vector &V_bar,
//                                           mfem::Vector &x_bar);

//    /// function defining current density aligned in a ring around the z axis
//    /// \param[in] x - position x in space of evaluation
//    /// \param[out] J - current density at position x
//    static void ringCurrentSource(const mfem::Vector &x, mfem::Vector &J);

//    static void ringCurrentSourceRevDiff(const mfem::Vector &x,
//                                         const mfem::Vector &V_bar,
//                                         mfem::Vector &x_bar);

//    /// function defining current density for simple box problem
//    /// \param[in] x - position x in space of evaluation
//    /// \param[out] J - current density at position x
//    static void box1CurrentSource(const mfem::Vector &x, mfem::Vector &J);

//    static void box1CurrentSourceRevDiff(const mfem::Vector &x,
//                                         const mfem::Vector &V_bar,
//                                         mfem::Vector &x_bar);

//    /// function defining current density for simple box problem
//    /// \param[in] x - position x in space of evaluation
//    /// \param[out] J - current density at position x
//    static void box2CurrentSource(const mfem::Vector &x, mfem::Vector &J);

//    static void box2CurrentSourceRevDiff(const mfem::Vector &x,
//                                         const mfem::Vector &V_bar,
//                                         mfem::Vector &x_bar);

//    /// function defining current density for TEAM 13 problem
//    /// \param[in] x - position x in space of evaluation
//    /// \param[out] J - current density at position x
//    static void team13CurrentSource(const mfem::Vector &x, mfem::Vector &J);

//    static void team13CurrentSourceRevDiff(const mfem::Vector &x,
//                                           const mfem::Vector &V_bar,
//                                           mfem::Vector &x_bar);

//    /// function defining magnetization aligned with the x axis
//    /// \param[in] x - position x in space of evaluation
//    /// \param[out] J - current density at position x
//    static void xAxisMagnetizationSource(const mfem::Vector &x, mfem::Vector
//    &M);

//    /// function defining magnetization aligned with the x axis
//    /// \param[in] x - position x in space of evaluation
//    /// \param[in] V_bar -
//    /// \param[out] x_bar - V_bar^T Jacobian
//    static void xAxisMagnetizationSourceRevDiff(const mfem::Vector &x,
//                                                const mfem::Vector &V_bar,
//                                                mfem::Vector &x_bar);

//    /// function defining magnetization aligned with the y axis
//    /// \param[in] x - position x in space of evaluation
//    /// \param[out] J - current density at position x
//    static void yAxisMagnetizationSource(const mfem::Vector &x, mfem::Vector
//    &M);

//    /// function defining magnetization aligned with the x axis
//    /// \param[in] x - position x in space of evaluation
//    /// \param[in] V_bar -
//    /// \param[out] x_bar - V_bar^T Jacobian
//    static void yAxisMagnetizationSourceRevDiff(const mfem::Vector &x,
//                                                const mfem::Vector &V_bar,
//                                                mfem::Vector &x_bar);

//    /// function defining magnetization aligned with the z axis
//    /// \param[in] x - position x in space of evaluation
//    /// \param[out] J - current density at position x
//    static void zAxisMagnetizationSource(const mfem::Vector &x, mfem::Vector
//    &M);

//    /// function defining magnetization aligned with the x axis
//    /// \param[in] x - position x in space of evaluation
//    /// \param[in] V_bar -
//    /// \param[out] x_bar - V_bar^T Jacobian
//    static void zAxisMagnetizationSourceRevDiff(const mfem::Vector &x,
//                                                const mfem::Vector &V_bar,
//                                                mfem::Vector &x_bar);

//    static void a_exact(const mfem::Vector &x, mfem::Vector &A);

//    static void b_exact(const mfem::Vector &x, mfem::Vector &B);

//    friend SolverPtr createSolver<MagnetostaticSolver>(
//        const nlohmann::json &json_options,
//        std::unique_ptr<mfem::Mesh> smesh,
//        MPI_Comm comm);
// };

}  // namespace miso

#endif
