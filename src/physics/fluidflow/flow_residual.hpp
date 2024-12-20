#ifndef MISO_FLOW_RESIDUAL
#define MISO_FLOW_RESIDUAL

#include "mfem.hpp"
#include "nlohmann/json.hpp"
#include "adept.h"

#include "miso_input.hpp"
#include "miso_nonlinearform.hpp"
#include "miso_output.hpp"
#include "functional_output.hpp"

namespace miso
{
/// Class for flow equations that follows the MISOResidual API
/// \tparam dim - number of spatial dimensions (1, 2, or 3)
/// \tparam entvar - if true, the entropy variables are used in the integrators
/// \note We do not use friend functions with this class because it is
/// templated, which would require a large number of forward declaration.
/// Instead we define member functions needed by the MISOResidual interface,
/// and then use these in non-friend functions.
template <int dim, bool entvar = false>
class FlowResidual final
{
public:
   /// Constructor for flow equations
   FlowResidual(const nlohmann::json &options,
                mfem::ParFiniteElementSpace &fespace,
                std::map<std::string, FiniteElementState> &fields,
                adept::Stack &diff_stack,
                std::ostream &outstream = std::cout);

   /// Returns the MPI communicator associated with the underlying space
   friend MPI_Comm getMPIComm(const FlowResidual &residual)
   {
      return residual.fes.GetComm();
   }

   /// Returns the number of equations/unknowns in the flow system
   int getSize_() const;

   /// Set inputs in the given flow residual object
   /// \param[in] inputs - defines values and fields being set
   void setInputs_(const MISOInputs &inputs);

   /// Set options in the given flow residual object
   /// \param[in] options - object containing the options
   void setOptions_(const nlohmann::json &options);

   /// Evaluate the fully-discrete flow residual equations
   /// \param[in] inputs - defines values and fields needed for the evaluation
   /// \param[out] res_vec - where the resulting residual is stored
   /// \note The behavior of evaluate can be changed depending on whether the
   /// residual is used in an explicit or implicit time integration.  This
   /// behavior is controlled by setting `options["implicit"]` to true and
   /// passing this to `setOptions`.
   void evaluate_(const MISOInputs &inputs, mfem::Vector &res_vec);

   /// Returns the Jacobian of the fully-discrete flow residual equations
   /// \param[in] inputs - defines values and fields needed for the Jacobian
   /// \param[out] wrt - variable that we want to differentiate with respect to
   /// \returns a reference to an mfem Operator that defines the Jacobian
   /// \note The behavior of getJacobian can be changed depending on whether
   /// the residual is used in an explicit or implicit time integration.  This
   /// behavior is controlled by setting `options["implicit"]` to true and
   /// passing this to `setOptions`.
   mfem::Operator &getJacobian_(const MISOInputs &inputs,
                                const std::string &wrt);

   /// Returns the total integrated entropy over the domain
   /// \param[in] inputs - defines values and fields needed for the entropy
   /// \returns the total entropy over the domain
   /// \note The entropy depends only on the state, but the residual helps
   /// distinguish if conservative or entropy-variables are used for the state.
   double calcEntropy_(const MISOInputs &inputs);

   /// Evaluate the residual weighted by the entropy variables
   /// \param[in] inputs - the variables needed to evaluate the entropy
   /// \return the product `w^T res`
   /// \note `w` and `res` are evaluated at `state + dt*state_dot` and time
   /// `t+dt`
   /// \note optional, but must be implemented for relaxation RK
   double calcEntropyChange_(const MISOInputs &inputs);

   /// Return mass matrix for the flow equations
   /// \param[in] options - options (not presently used)
   /// \return pointer to mass matrix for the flow equations
   /// \note Constructs the mass form and matrix, and the returned pointer is
   /// owned by the residual
   mfem::Operator *getMassMatrix_(const nlohmann::json &options);

   /// Return a preconditioner for the flow residual's state Jacobian
   /// \return pointer to preconditioner for the state Jacobian
   /// \note the returned pointer is owned by the residual
   mfem::Solver *getPreconditioner_();

   /// Returns the minimum time step for a given state and CFL number
   /// \param[in] cfl - the target maximum allowable CFL number
   /// \param[in] state - the state which defines the velocity field
   double minCFLTimeStep(double cfl, const mfem::ParGridFunction &state);

   /// Returns the L2 error between the discrete and exact conservative vars.
   /// \param[in] u_exact - function that defines the exact **state**
   /// \param[in] entry - if >= 0, the L2 error of state `entry` is returned
   /// \returns L2 error
   /// \note The solution given by `u_exact` is for the state, conservative or
   /// entropy variables.  **Do not give the exact solution for the conservative
   /// variables if using entropy variables**.   The conversion to conservative
   /// variables is done by this function.
   double calcConservativeVarsL2Error(const mfem::ParGridFunction &state,
                                      void (*u_exact)(const mfem::Vector &,
                                                      mfem::Vector &),
                                      int entry);

   /// Returns an output capable of evaluting `fun`
   /// \param[in] fun - the name of the output to be constructed
   /// \param[in] options - used to define the output
   /// \returns a `MISOOutput`, which is **not** owned by residual
   MISOOutput constructOutput(const std::string &fun,
                              const nlohmann::json &options);

   /// Set the given vector to the free-stream *conservative* variables
   /// \param[out] qfar - used to hold the free-stream state upon return
   /// \note Helper function that wraps getFreeStreamQ and provides relevant
   /// constants
   void getFreeStreamState(mfem::Vector &qfar);

   bool isViscous() const { return viscous; }
   double getViscosity() const { return mu; }
   double getMISO() const { return mach_fs; }
   double getAoA() const { return aoa_fs; }
   double getReynolds() const { return re_fs; }
   double getPrandtl() const { return pr_fs; }
   int getIRoll() const { return iroll; }
   int getIPitch() const { return ipitch; }

private:
   /// reference to solver options
   const nlohmann::json &options;
   /// print object
   std::ostream &out;
   /// if true, viscous terms are included in the residual
   bool viscous;
   /// nondimensional viscosity (if mu is negative, we use Sutherland's)
   double mu;
   /// free-stream MISO number
   double mach_fs;
   /// free-stream angle of attack
   double aoa_fs;
   /// free-stream Reynolds number
   double re_fs;
   /// free-stream Prandtl number
   double pr_fs;
   /// index of dimension corresponding to nose to tail axis
   int iroll;
   /// index of "vertical" dimension in body frame
   int ipitch;
   /// if true, the states passed in are assumed to be the entropy variables
   bool state_is_entvar = false;
   /// Determines if the residual is for explicit or implicit time-marching
   bool is_implicit;
   /// Finite-element space associated with inputs to the residual
   mfem::ParFiniteElementSpace &fes;
   /// stack used for algorithmic differentiation
   adept::Stack &stack;
   /// Map of all state vectors used by the residual
   std::map<std::string, FiniteElementState> &fields;
   /// Defines the nonlinear form used to compute the residual and its Jacobian
   miso::MISONonlinearForm res;
   /// Bilinear form for the mass-matrix operator (make a MISONonlinearForm?)
   mfem::ParBilinearForm mass;
   /// Mass matrix as HypreParMatrix
   std::unique_ptr<mfem::Operator> mass_mat;
   /// Preconditioner for the spatial Jacobian
   std::unique_ptr<mfem::Solver> prec;
   /// Defines the output used to evaluate the entropy
   miso::FunctionalOutput ent;
   /// Work vector
   mfem::Vector work;

   void addFlowDomainIntegrators(const nlohmann::json &flow,
                                 const nlohmann::json &space_dis);

   void addFlowInterfaceIntegrators(const nlohmann::json &flow,
                                    const nlohmann::json &space_dis);

   void addFlowBoundaryIntegrators(const nlohmann::json &flow,
                                   const nlohmann::json &space_dis,
                                   const nlohmann::json &bcs);

   void addInviscidBoundaryIntegrators(const nlohmann::json &flow,
                                       const nlohmann::json &space_dis,
                                       const nlohmann::json &bcs);

   void addViscousBoundaryIntegrators(const nlohmann::json &flow,
                                      const nlohmann::json &space_dis,
                                      const nlohmann::json &bcs);

   void addEntropyIntegrators();
};

/// Returns the number of equations/unknowns in the flow system
/// \param[in] residual - flow residual being queried
/// \tparam dim - number of spatial dimensions (1, 2, or 3)
/// \tparam entvar - if true, the entropy variables are used in the integrators
template <int dim, bool entvar>
int getSize(const FlowResidual<dim, entvar> &residual)
{
   return residual.getSize_();
}

/// Set inputs in the given flow residual
/// \param[inout] residual - flow residual whose inputs are set
/// \param[in] inputs - defines values and fields being set
/// \tparam dim - number of spatial dimensions (1, 2, or 3)
/// \tparam entvar - if true, the entropy variables are used in the integrators
template <int dim, bool entvar>
void setInputs(FlowResidual<dim, entvar> &residual, const MISOInputs &inputs)
{
   residual.setInputs_(inputs);
}

/// Set options in the given flow residual
/// \param[inout] residual - flow residual whose options are being set
/// \param[in] options - object containing the options
/// \tparam dim - number of spatial dimensions (1, 2, or 3)
/// \tparam entvar - if true, the entropy variables are used in the integrators
template <int dim, bool entvar>
void setOptions(FlowResidual<dim, entvar> &residual,
                const nlohmann::json &options)
{
   residual.setOptions_(options);
}

/// Evaluate the fully-discrete flow residual equations
/// \param[inout] residual - defines the flow residual being evaluated
/// \param[in] inputs - defines values and fields needed for the evaluation
/// \param[out] res_vec - where the resulting residual is stored
/// \tparam dim - number of spatial dimensions (1, 2, or 3)
/// \tparam entvar - if true, the entropy variables are used in the integrators
/// \note The behavior of evaluate can be changed depending on whether the
/// residual is used in an explicit or implicit time integration.  This
/// behavior is controlled by setting `options["implicit"]` to true and
/// passing this to `setOptions`.
template <int dim, bool entvar>
void evaluate(FlowResidual<dim, entvar> &residual,
              const MISOInputs &inputs,
              mfem::Vector &res_vec)
{
   residual.evaluate_(inputs, res_vec);
}

/// Returns the Jacobian of the fully-discrete flow residual equations
/// \param[inout] residual - the flow residual whose Jacobian is sought
/// \param[in] inputs - defines values and fields needed for the Jacobian
/// \param[out] wrt - variable that we want to differentiate with respect to
/// \returns a reference to an mfem Operator that defines the Jacobian
/// \tparam dim - number of spatial dimensions (1, 2, or 3)
/// \tparam entvar - if true, the entropy variables are used in the integrators
/// \note The behavior of getJacobian can be changed depending on whether
/// the residual is used in an explicit or implicit time integration.  This
/// behavior is controlled by setting `options["implicit"]` to true and
/// passing this to `setOptions`.
template <int dim, bool entvar>
mfem::Operator &getJacobian(FlowResidual<dim, entvar> &residual,
                            const MISOInputs &inputs,
                            const std::string &wrt)
{
   return residual.getJacobian_(inputs, wrt);
}

/// Returns the total integrated entropy over the domain
/// \param[inout] residual - the flow residual, which helps compute entropy
/// \param[in] inputs - defines values and fields needed for the entropy
/// \returns the total entropy over the domain
/// \tparam dim - number of spatial dimensions (1, 2, or 3)
/// \tparam entvar - if true, the entropy variables are used in the integrators
/// \note The entropy depends only on the state, but the residual helps
/// distinguish if conservative or entropy-variables are used for the state.
template <int dim, bool entvar>
double calcEntropy(FlowResidual<dim, entvar> &residual,
                   const MISOInputs &inputs)
{
   return residual.calcEntropy_(inputs);
}

/// Evaluate the residual weighted by the entropy variables
/// \param[inout] residual - function with an associated entropy
/// \param[in] inputs - the variables needed to evaluate the entropy
/// \tparam dim - number of spatial dimensions (1, 2, or 3)
/// \tparam entvar - if true, the entropy variables are used in the integrators
/// \return the product `w^T res`
/// \note `w` and `res` are evaluated at `state` and time `t+dt`
/// \note `res` is equal to `-state_dot`, but may be recomputed if necessary
/// \note optional, but must be implemented for relaxation RK
template <int dim, bool entvar>
double calcEntropyChange(FlowResidual<dim, entvar> &residual,
                         const MISOInputs &inputs)
{
   return residual.calcEntropyChange_(inputs);
}

/// Return mass matrix for the flow equations
/// \param[in] mass_options - options (not presently used)
/// \return pointer to mass matrix for the flow equations
/// \note Constructs the mass form and matrix, and the returned pointer is
/// owned by the residual
template <int dim, bool entvar>
mfem::Operator *getMassMatrix(FlowResidual<dim, entvar> &residual,
                              const nlohmann::json &mass_options)
{
   return residual.getMassMatrix_(mass_options);
}

/// Return a preconditioner for the flow residual's state Jacobian
/// \param[inout] residual - residual whose preconditioner is desired
/// \return pointer to preconditioner for the state Jacobian
/// \note Constructs the preconditioner if necessary, and the returned pointer
/// is owned by the `residual`
template <int dim, bool entvar>
mfem::Solver *getPreconditioner(FlowResidual<dim, entvar> &residual)
{
   return residual.getPreconditioner_();
}

}  // namespace miso

#endif  // FLOW_RESIDUAL