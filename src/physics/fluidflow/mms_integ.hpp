#ifndef MISO_MMS_INTEG
#define MISO_MMS_INTEG

#include "mfem.hpp"

#include "sbp_fe.hpp"
#include "utils.hpp"

namespace miso
{
/// Integrator for method-of-manufactured solution (MMS) sources
/// \tparam Derived - a class Derived from this one (needed for CRTP)
/// \note This probably does not need to be a nonlinear integrator, but this
/// makes it easier to incorporate directly into the nonlinear form.
template <typename Derived>
class MMSIntegrator : public mfem::NonlinearFormIntegrator
{
public:
   /// Construct an integrator for MMS sources
   /// \param[in] num_state_vars - the number of state variables
   /// \param[in] a - factor, usually used to move terms to rhs
   /// \note `num_state_vars` is not necessarily the same as the number of
   /// states used by, nor the number of fluxes returned by, `source`.
   /// For example, there may be 5 states for the 2D RANS equations, but
   /// `source` may use only the first 4.
   MMSIntegrator(int num_state_vars = 1, double a = 1.0)
    : num_states(num_state_vars), alpha(a)
   { }

   /// Get the contribution of this element to a functional
   /// \param[in] el - the finite element whose contribution we want
   /// \param[in] trans - defines the reference to physical element mapping
   /// \param[in] elfun - element local state function
   double GetElementEnergy(const mfem::FiniteElement &el,
                           mfem::ElementTransformation &trans,
                           const mfem::Vector &elfun) override
   {
      return 0.0;
   }

   /// Construct the element local residual
   /// \param[in] el - the finite element whose residual we want
   /// \param[in] trans - defines the reference to physical element mapping
   /// \param[in] elfun - element local state function
   /// \param[out] elvect - element local residual
   void AssembleElementVector(const mfem::FiniteElement &el,
                              mfem::ElementTransformation &trans,
                              const mfem::Vector &elfun,
                              mfem::Vector &elvect) override;

   /// Construct the element local Jacobian
   /// \param[in] el - the finite element whose Jacobian we want
   /// \param[in] trans - defines the reference to physical element mapping
   /// \param[in] elfun - element local state function
   /// \param[out] elmat - element local Jacobian
   void AssembleElementGrad(const mfem::FiniteElement &el,
                            mfem::ElementTransformation &trans,
                            const mfem::Vector &elfun,
                            mfem::DenseMatrix &elmat) override;

protected:
   /// number of states
   int num_states;
   /// scales the terms; can be used to move to rhs/lhs
   double alpha;
#ifndef MFEM_THREAD_SAFE
   /// the coordinates of node i
   mfem::Vector x_i;
   /// the source for node i
   mfem::Vector src_i;
#endif

   /// The MMS source function
   /// \param[in] x - spatial location at which to evaluate the source
   /// \param[out] src - source term evaluated at `x`
   /// \note This uses the CRTP, so it wraps a call to `calcSource` in Derived.
   void source(const mfem::Vector &x, mfem::Vector &src)
   {
      static_cast<Derived *>(this)->calcSource(x, src);
   }
};

}  // namespace miso

#include "mms_integ_def.hpp"

#endif
