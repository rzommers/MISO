#ifndef MACH_EULER_INTEG_DG_CUT
#define MACH_EULER_INTEG_DG_CUT

#include "adept.h"
#include "mfem.hpp"

#include "inviscid_integ_dg_cut.hpp"
#include "euler_fluxes.hpp"
#include "mms_integ_dg_cut.hpp"
namespace mach
{
/// Integrator for the Euler flux over an element
/// \tparam dim - number of spatial dimensions (1, 2, or 3)
/// \note This derived class uses the CRTP
template <int dim>
class CutEulerDGIntegrator
 : public CutDGInviscidIntegrator<CutEulerDGIntegrator<dim>>
{
public:
   /// Construct an integrator for the Euler flux over elements
   /// \param[in] diff_stack - for algorithmic differentiation
   /// \param[in] cutSquareIntRules - integration rule for cut cells
   /// \param[in] embeddedElements - elements completely inside the geometry
   /// \param[in] a - factor, usually used to move terms to rhs
   CutEulerDGIntegrator(adept::Stack &diff_stack,
                        std::map<int, IntegrationRule *> cutSquareIntRules,
                        std::vector<bool> embeddedElements,
                        double a = 1.0)
    : CutDGInviscidIntegrator<CutEulerDGIntegrator<dim>>(diff_stack,
                                                         cutSquareIntRules,
                                                         embeddedElements,
                                                         dim + 2,
                                                         a)
   { }

   /// Not used by this integrator
   double calcVolFun(const mfem::Vector &x, const mfem::Vector &u)
   {
      return 0.0;
   }

   /// Euler flux function in a given (scaled) direction
   /// \param[in] dir - direction in which the flux is desired
   /// \param[in] q - conservative variables
   /// \param[out] flux - fluxes in the direction `dir`
   /// \note wrapper for the relevant function in `euler_fluxes.hpp`
   void calcFlux(const mfem::Vector &dir,
                 const mfem::Vector &q,
                 mfem::Vector &flux)
   {
      calcEulerFlux<double, dim>(dir.GetData(), q.GetData(), flux.GetData());
   }

   /// Compute the Jacobian of the Euler flux w.r.t. `q`
   /// \param[in] dir - desired direction (scaled) for the flux
   /// \param[in] q - state at which to evaluate the flux Jacobian
   /// \param[out] flux_jac - Jacobian of the flux function w.r.t. `q`
   void calcFluxJacState(const mfem::Vector &dir,
                         const mfem::Vector &q,
                         mfem::DenseMatrix &flux_jac);

   /// Compute the Jacobian of the flux function `flux` w.r.t. `dir`
   /// \parma[in] dir - desired direction for the flux
   /// \param[in] q - state at which to evaluate the flux Jacobian
   /// \param[out] flux_jac - Jacobian of the flux function w.r.t. `dir`
   /// \note This uses the CRTP, so it wraps a call to a func. in Derived.
   void calcFluxJacDir(const mfem::Vector &dir,
                       const mfem::Vector &q,
                       mfem::DenseMatrix &flux_jac);
};

/// Integrator for the steady isentropic-vortex boundary condition
/// \tparam dim - number of spatial dimensions (1, 2, or 3)
/// \tparam entvar - if true, states = ent. vars; otherwise, states = conserv.
/// \note This derived class uses the CRTP
template <int dim, bool entvar = false>
class CutDGIsentropicVortexBC
 : public CutDGInviscidBoundaryIntegrator<CutDGIsentropicVortexBC<dim, entvar>>
{
public:
   /// Constructs an integrator for isentropic vortex boundary flux
   /// \param[in] diff_stack - for algorithmic differentiation
   /// \param[in] fe_coll - used to determine the face elements
   /// \param[in] cutSegmentIntRules - integration rule for cut segments
   /// \param[in] phi - level-set function
   /// \param[in] a - used to move residual to lhs (1.0) or rhs(-1.0)
   CutDGIsentropicVortexBC(adept::Stack &diff_stack,
                           const mfem::FiniteElementCollection *fe_coll,
                           std::map<int, IntegrationRule *> cutSegmentIntRules,
                           /*Algoim::LevelSet<2>*/ circle<2> phi,
                           double a = 1.0)
    : CutDGInviscidBoundaryIntegrator<CutDGIsentropicVortexBC<dim, entvar>>(
          diff_stack,
          fe_coll,
          cutSegmentIntRules,
          phi,
          dim + 2,
          a)
   { }

   /// Contracts flux with the entropy variables
   /// \param[in] x - coordinate location at which flux is evaluated
   /// \param[in] dir - vector normal to the boundary at `x`
   /// \param[in] q - state variable at which to evaluate the flux
   double calcBndryFun(const mfem::Vector &x,
                       const mfem::Vector &dir,
                       const mfem::Vector &q);

   /// Compute a characteristic boundary flux for the isentropic vortex
   /// \param[in] x - coordinate location at which flux is evaluated
   /// \param[in] dir - vector normal to the boundary at `x`
   /// \param[in] q - state variable at which to evaluate the flux
   /// \param[out] flux_vec - value of the flux
   void calcFlux(const mfem::Vector &x,
                 const mfem::Vector &dir,
                 const mfem::Vector &q,
                 mfem::Vector &flux_vec);

   /// Compute the Jacobian of the isentropic vortex boundary flux w.r.t. `q`
   /// \param[in] x - coordinate location at which flux is evaluated
   /// \param[in] dir - vector normal to the boundary at `x`
   /// \param[in] q - state variable at which to evaluate the flux
   /// \param[out] flux_jac - Jacobian of `flux` w.r.t. `q`
   void calcFluxJacState(const mfem::Vector &x,
                         const mfem::Vector &dir,
                         const mfem::Vector &q,
                         mfem::DenseMatrix &flux_jac);

   /// Compute the Jacobian of the isentropic vortex boundary flux w.r.t. `dir`
   /// \param[in] x - coordinate location at which flux is evaluated
   /// \param[in] dir - vector normal to the boundary at `x`
   /// \param[in] q - state variable at which to evaluate the flux
   /// \param[out] flux_jac - Jacobian of `flux` w.r.t. `dir`
   void calcFluxJacDir(const mfem::Vector &x,
                       const mfem::Vector &dir,
                       const mfem::Vector &q,
                       mfem::DenseMatrix &flux_jac);
};

/// Integrator for inviscid slip-wall boundary condition
/// \tparam dim - number of spatial dimensions (1, 2, or 3)
/// \tparam entvar - if true, states = ent. vars; otherwise, states = conserv.
/// \note This derived class uses the CRTP
template <int dim, bool entvar = false>
class CutDGSlipWallBC
 : public CutDGInviscidBoundaryIntegrator<CutDGSlipWallBC<dim, entvar>>
{
public:
   /// Constructs an integrator for a slip-wall boundary flux
   /// \param[in] diff_stack - for algorithmic differentiation
   /// \param[in] fe_coll - used to determine the face elements
   /// \param[in] cutSegmentIntRules - integration rule for cut segments
   /// \param[in] phi - level-set function
   /// \param[in] a - used to move residual to lhs (1.0) or rhs(-1.0)
   CutDGSlipWallBC(adept::Stack &diff_stack,
                   const mfem::FiniteElementCollection *fe_coll,
                   std::map<int, IntegrationRule *> cutSegmentIntRules,
                   /*Algoim::LevelSet<2>*/ circle<2> phi,
                   double a = 1.0)
    : CutDGInviscidBoundaryIntegrator<CutDGSlipWallBC<dim, entvar>>(
          diff_stack,
          fe_coll,
          cutSegmentIntRules,
          phi,
          dim + 2,
          a)
   { }

   /// Contracts flux with the entropy variables
   /// \param[in] x - coordinate location at which flux is evaluated
   /// \param[in] dir - vector normal to the boundary at `x`
   /// \param[in] q - state variable at which to evaluate the flux
   double calcBndryFun(const mfem::Vector &x,
                       const mfem::Vector &dir,
                       const mfem::Vector &q);

   /// Compute an adjoint-consistent slip-wall boundary flux
   /// \param[in] x - coordinate location at which flux is evaluated (not used)
   /// \param[in] dir - vector normal to the boundary at `x`
   /// \param[in] q - conservative variables at which to evaluate the flux
   /// \param[out] flux_vec - value of the flux
   void calcFlux(const mfem::Vector &x,
                 const mfem::Vector &dir,
                 const mfem::Vector &q,
                 mfem::Vector &flux_vec);

   /// Compute the Jacobian of the slip-wall boundary flux w.r.t. `q`
   /// \param[in] x - coordinate location at which flux is evaluated (not used)
   /// \param[in] dir - vector normal to the boundary at `x`
   /// \param[in] q - conservative variables at which to evaluate the flux
   /// \param[out] flux_jac - Jacobian of `flux` w.r.t. `q`
   void calcFluxJacState(const mfem::Vector &x,
                         const mfem::Vector &dir,
                         const mfem::Vector &q,
                         mfem::DenseMatrix &flux_jac);

   /// Compute the Jacobian of the slip-wall boundary flux w.r.t. `dir`
   /// \param[in] x - coordinate location at which flux is evaluated (not used)
   /// \param[in] dir - vector normal to the boundary at `x`
   /// \param[in] q - conservative variables at which to evaluate the flux
   /// \param[out] flux_jac - Jacobian of `flux` w.r.t. `dir`
   void calcFluxJacDir(const mfem::Vector &x,
                       const mfem::Vector &dir,
                       const mfem::Vector &q,
                       mfem::DenseMatrix &flux_jac);

private:
};
/// Integrator for applying inviscid far-field boundary condition on slip-wall
/// boundary for testing purposes \tparam dim - number of spatial dimensions (1,
/// 2, or 3) \tparam entvar - if true, states = ent. vars; otherwise, states =
/// conserv. \note This derived class uses the CRTP
template <int dim, bool entvar = false>
class CutDGSlipFarFieldBC
 : public CutDGInviscidBoundaryIntegrator<CutDGSlipFarFieldBC<dim, entvar>>
{
public:
   /// Constructs an integrator for a far-field boundary flux
   /// \param[in] diff_stack - for algorithmic differentiation
   /// \param[in] fe_coll - used to determine the face elements
   /// \param[in] cutSegmentIntRules - integration rule for cut segments
   /// \param[in] phi - level-set function
   /// \param[in] q_far - state at the far-field
   /// \param[in] a - used to move residual to lhs (1.0) or rhs(-1.0)
   CutDGSlipFarFieldBC(adept::Stack &diff_stack,
                       const mfem::FiniteElementCollection *fe_coll,
                       std::map<int, IntegrationRule *> cutSegmentIntRules,
                       /*Algoim::LevelSet<2>*/ circle<2> phi,
                       const mfem::Vector &q_far,
                       double a = 1.0)
    : CutDGInviscidBoundaryIntegrator<CutDGSlipFarFieldBC<dim, entvar>>(
          diff_stack,
          fe_coll,
          cutSegmentIntRules,
          phi,
          dim + 2,
          a),
      qfs(q_far),
      work_vec(dim + 2)
   { }

   /// Contracts flux with the entropy variables
   /// \param[in] x - coordinate location at which flux is evaluated
   /// \param[in] dir - vector normal to the boundary at `x`
   /// \param[in] q - state variable at which to evaluate the flux
   double calcBndryFun(const mfem::Vector &x,
                       const mfem::Vector &dir,
                       const mfem::Vector &q);

   /// Compute an adjoint-consistent slip-wall boundary flux
   /// \param[in] x - coordinate location at which flux is evaluated (not used)
   /// \param[in] dir - vector normal to the boundary at `x`
   /// \param[in] q - conservative variables at which to evaluate the flux
   /// \param[out] flux_vec - value of the flux
   void calcFlux(const mfem::Vector &x,
                 const mfem::Vector &dir,
                 const mfem::Vector &q,
                 mfem::Vector &flux_vec);

   /// Compute the Jacobian of the slip-wall boundary flux w.r.t. `q`
   /// \param[in] x - coordinate location at which flux is evaluated (not used)
   /// \param[in] dir - vector normal to the boundary at `x`
   /// \param[in] q - conservative variables at which to evaluate the flux
   /// \param[out] flux_jac - Jacobian of `flux` w.r.t. `q`
   void calcFluxJacState(const mfem::Vector &x,
                         const mfem::Vector &dir,
                         const mfem::Vector &q,
                         mfem::DenseMatrix &flux_jac);

   /// Compute the Jacobian of the slip-wall boundary flux w.r.t. `dir`
   /// \param[in] x - coordinate location at which flux is evaluated (not used)
   /// \param[in] dir - vector normal to the boundary at `x`
   /// \param[in] q - conservative variables at which to evaluate the flux
   /// \param[out] flux_jac - Jacobian of `flux` w.r.t. `dir`
   void calcFluxJacDir(const mfem::Vector &x,
                       const mfem::Vector &dir,
                       const mfem::Vector &q,
                       mfem::DenseMatrix &flux_jac);

private:
   /// Stores the far-field state
   mfem::Vector qfs;
   /// Work vector for boundary flux computation
   mfem::Vector work_vec;
};

/// Integrator for inviscid far-field boundary condition
/// \tparam dim - number of spatial dimensions (1, 2, or 3)
/// \tparam entvar - if true, states = ent. vars; otherwise, states = conserv.
/// \note This derived class uses the CRTP
template <int dim, bool entvar = false>
class CutDGFarFieldBC
 : public DGInviscidBoundaryIntegrator<CutDGFarFieldBC<dim, entvar>>
{
public:
   /// Constructs an integrator for a far-field boundary flux
   /// \param[in] diff_stack - for algorithmic differentiation
   /// \param[in] fe_coll - used to determine the face elements
   /// \param[in] q_far - state at the far-field
   /// \param[in] a - used to move residual to lhs (1.0) or rhs(-1.0)
   CutDGFarFieldBC(adept::Stack &diff_stack,
                   const mfem::FiniteElementCollection *fe_coll,
                   const mfem::Vector &q_far,
                   double a = 1.0)
    : DGInviscidBoundaryIntegrator<CutDGFarFieldBC<dim, entvar>>(diff_stack,
                                                                 fe_coll,
                                                                 dim + 2,
                                                                 a),
      qfs(q_far),
      work_vec(dim + 2)
   { }

   /// Contracts flux with the entropy variables
   /// \param[in] x - coordinate location at which flux is evaluated
   /// \param[in] dir - vector normal to the boundary at `x`
   /// \param[in] q - state variable at which to evaluate the flux
   double calcBndryFun(const mfem::Vector &x,
                       const mfem::Vector &dir,
                       const mfem::Vector &q);

   /// Compute an adjoint-consistent slip-wall boundary flux
   /// \param[in] x - coordinate location at which flux is evaluated (not used)
   /// \param[in] dir - vector normal to the boundary at `x`
   /// \param[in] q - conservative variables at which to evaluate the flux
   /// \param[out] flux_vec - value of the flux
   void calcFlux(const mfem::Vector &x,
                 const mfem::Vector &dir,
                 const mfem::Vector &q,
                 mfem::Vector &flux_vec);

   /// Compute the Jacobian of the slip-wall boundary flux w.r.t. `q`
   /// \param[in] x - coordinate location at which flux is evaluated (not used)
   /// \param[in] dir - vector normal to the boundary at `x`
   /// \param[in] q - conservative variables at which to evaluate the flux
   /// \param[out] flux_jac - Jacobian of `flux` w.r.t. `q`
   void calcFluxJacState(const mfem::Vector &x,
                         const mfem::Vector &dir,
                         const mfem::Vector &q,
                         mfem::DenseMatrix &flux_jac);

   /// Compute the Jacobian of the slip-wall boundary flux w.r.t. `dir`
   /// \param[in] x - coordinate location at which flux is evaluated (not used)
   /// \param[in] dir - vector normal to the boundary at `x`
   /// \param[in] q - conservative variables at which to evaluate the flux
   /// \param[out] flux_jac - Jacobian of `flux` w.r.t. `dir`
   void calcFluxJacDir(const mfem::Vector &x,
                       const mfem::Vector &dir,
                       const mfem::Vector &q,
                       mfem::DenseMatrix &flux_jac);

private:
   /// Stores the far-field state
   mfem::Vector qfs;
   /// Work vector for boundary flux computation
   mfem::Vector work_vec;
};

/// Integrator for vortex boundary condition
/// \tparam dim - number of spatial dimensions (1, 2, or 3)
/// \tparam entvar - if true, states = ent. vars; otherwise, states = conserv.
/// \note This derived class uses the CRTP
template <int dim, bool entvar = false>
class CutDGVortexBC
 : public CutDGEulerBoundaryIntegrator<CutDGVortexBC<dim, entvar>>
{
public:
   /// Constructs an integrator for a vortex boundary flux
   /// \param[in] diff_stack - for algorithmic differentiation
   /// \param[in] fe_coll - used to determine the face elements
   /// \param[in] cutBdrFaceIntRules - integration rule for the outer cut bdr
   /// faces \param[in] embeddedElements - elements completely inside geometry
   /// \param[in] a - used to move residual to lhs (1.0) or rhs(-1.0)
   CutDGVortexBC(adept::Stack &diff_stack,
                 const mfem::FiniteElementCollection *fe_coll,
                 std::map<int, IntegrationRule *> cutBdrFaceIntRules,
                 std::vector<bool> embeddedElements,
                 double a = 1.0)
    : CutDGEulerBoundaryIntegrator<CutDGVortexBC<dim, entvar>>(
          diff_stack,
          fe_coll,
          cutBdrFaceIntRules,
          embeddedElements,
          dim + 2,
          a)
   { }

   /// Contracts flux with the entropy variables
   /// \param[in] x - coordinate location at which flux is evaluated
   /// \param[in] dir - vector normal to the boundary at `x`
   /// \param[in] q - state variable at which to evaluate the flux
   double calcBndryFun(const mfem::Vector &x,
                       const mfem::Vector &dir,
                       const mfem::Vector &q);

   /// Compute an adjoint-consistent slip-wall boundary flux
   /// \param[in] x - coordinate location at which flux is evaluated (not used)
   /// \param[in] dir - vector normal to the boundary at `x`
   /// \param[in] q - conservative variables at which to evaluate the flux
   /// \param[out] flux_vec - value of the flux
   void calcFlux(const mfem::Vector &x,
                 const mfem::Vector &dir,
                 const mfem::Vector &q,
                 mfem::Vector &flux_vec);

   /// Compute the Jacobian of the slip-wall boundary flux w.r.t. `q`
   /// \param[in] x - coordinate location at which flux is evaluated (not used)
   /// \param[in] dir - vector normal to the boundary at `x`
   /// \param[in] q - conservative variables at which to evaluate the flux
   /// \param[out] flux_jac - Jacobian of `flux` w.r.t. `q`
   void calcFluxJacState(const mfem::Vector &x,
                         const mfem::Vector &dir,
                         const mfem::Vector &q,
                         mfem::DenseMatrix &flux_jac);

   /// Compute the Jacobian of the slip-wall boundary flux w.r.t. `dir`
   /// \param[in] x - coordinate location at which flux is evaluated (not used)
   /// \param[in] dir - vector normal to the boundary at `x`
   /// \param[in] q - conservative variables at which to evaluate the flux
   /// \param[out] flux_jac - Jacobian of `flux` w.r.t. `dir`
   void calcFluxJacDir(const mfem::Vector &x,
                       const mfem::Vector &dir,
                       const mfem::Vector &q,
                       mfem::DenseMatrix &flux_jac);

private:
   /// Stores the far-field state
   mfem::Vector qfs;
   /// Work vector for boundary flux computation
   mfem::Vector work_vec;
};

/// Interface integrator for the DG method
/// \tparam dim - number of spatial dimension (1, 2 or 3)
/// \tparam entvar - if true, states = ent. vars; otherwise, states = conserv.
template <int dim, bool entvar = false>
class CutDGInterfaceIntegrator
 : public CutDGInviscidFaceIntegrator<CutDGInterfaceIntegrator<dim, entvar>>
{
public:
   /// Construct an integrator for the Euler flux over elements
   /// \param[in] diff_stack - for algorithmic differentiation
   /// \param[in] coeff - scales the dissipation (must be non-negative!)
   /// \param[in] fe_coll - pointer to a finite element collection
   /// \param[in] immersedFaces - interior faces completely inside the geometry
   /// \param[in] cutInteriorFaceIntRules - integration rule for the interior
   /// faces cut by the geometry \param[in] a - factor, usually used to move
   /// terms to rhs
   CutDGInterfaceIntegrator(
       adept::Stack &diff_stack,
       double coeff,
       const mfem::FiniteElementCollection *fe_coll,
       std::map<int, bool> immersedFaces,
       std::map<int, IntegrationRule *> cutInteriorFaceIntRules,
       double a = 1.0);

   /// Contracts flux with the entropy variables
   /// \param[in] dir - vector normal to the interface
   /// \param[in] qL - "left" state at which to evaluate the flux
   /// \param[in] qR - "right" state at which to evaluate the flux
   double calcIFaceFun(const mfem::Vector &dir,
                       const mfem::Vector &qL,
                       const mfem::Vector &qR);

   /// Compute the interface function at a given (scaled) direction
   /// \param[in] dir - vector normal to the interface
   /// \param[in] qL - "left" state at which to evaluate the flux
   /// \param[in] qR - "right" state at which to evaluate the flux
   /// \param[out] flux - value of the flux
   /// \note wrapper for the relevant function in `euler_fluxes.hpp`
   void calcFlux(const mfem::Vector &dir,
                 const mfem::Vector &qL,
                 const mfem::Vector &qR,
                 mfem::Vector &flux);

   /// Compute the Jacobian of the interface flux function w.r.t. states
   /// \param[in] dir - vector normal to the face
   /// \param[in] qL - "left" state at which to evaluate the flux
   /// \param[in] qL - "right" state at which to evaluate the flux
   /// \param[out] jacL - Jacobian of `flux` w.r.t. `qL`
   /// \param[out] jacR - Jacobian of `flux` w.r.t. `qR`
   /// \note This uses the CRTP, so it wraps a call a func. in Derived.
   void calcFluxJacState(const mfem::Vector &dir,
                         const mfem::Vector &qL,
                         const mfem::Vector &qR,
                         mfem::DenseMatrix &jacL,
                         mfem::DenseMatrix &jacR);

   /// Compute the Jacobian of the interface flux function w.r.t. `dir`
   /// \param[in] dir - vector normal to the boundary at `x`
   /// \param[in] qL - "left" state at which to evaluate the flux
   /// \param[in] qR - "right" state at which to evaluate the flux
   /// \param[out] jac_dir - Jacobian of `flux` w.r.t. `dir`
   /// \note This uses the CRTP, so it wraps a call to a func. in Derived.
   void calcFluxJacDir(const mfem::Vector &dir,
                       const mfem::Vector &qL,
                       const mfem::Vector &qR,
                       mfem::DenseMatrix &jac_dir);

protected:
   /// Scalar that controls the amount of dissipation
   double diss_coeff;
};

/// Integrator for mass matrix
class CutDGMassIntegrator : public mfem::BilinearFormIntegrator
{
public:
   /// Constructs a diagonal-mass matrix integrator.
   /// \param[in] _cutSquareIntRules - integration rule for the elements cut by
   /// the geometry \param[in] _embeddedElements - elements completely inside
   /// the geometry \param[in] nvar - number of state variables
   CutDGMassIntegrator(std::map<int, IntegrationRule *> _cutSquareIntRules,
                       std::vector<bool> _embeddedElements,
                       int nvar = 1)
    : num_state(nvar),
      cutSquareIntRules(_cutSquareIntRules),
      embeddedElements(_embeddedElements)
   { }

   /// Finds the mass matrix for the given element.
   /// \param[in] el - the element for which the mass matrix is desired
   /// \param[in,out] trans -  transformation
   /// \param[out] elmat - the element mass matrix
   void AssembleElementMatrix(const mfem::FiniteElement &el,
                              mfem::ElementTransformation &trans,
                              mfem::DenseMatrix &elmat)
   {
      using namespace mfem;
      int num_nodes = el.GetDof();
      double w;
#ifdef MFEM_THREAD_SAFE
      Vector shape;
#endif
      elmat.SetSize(num_nodes * num_state);
      shape.SetSize(num_nodes);
      DenseMatrix elmat1;
      elmat1.SetSize(num_nodes);
      if (embeddedElements.at(trans.ElementNo) == true)
      {
         for (int k = 0; k < elmat.Size(); ++k)
         {
            elmat(k, k) = 1.0;
         }
      }
      else
      {
         const IntegrationRule *ir;  // = IntRule;
         ir = cutSquareIntRules[trans.ElementNo];
         if (ir == NULL)
         {
            int order = 2 * el.GetOrder() + trans.OrderW();
            if (el.Space() == FunctionSpace::rQk)
            {
               ir = &RefinedIntRules.Get(el.GetGeomType(), order);
            }
            else
            {
               ir = &IntRules.Get(el.GetGeomType(), order);
            }
         }
         elmat = 0.0;
         for (int i = 0; i < ir->GetNPoints(); i++)
         {
            const IntegrationPoint &ip = ir->IntPoint(i);
            el.CalcShape(ip, shape);
            trans.SetIntPoint(&ip);
            w = trans.Weight() * ip.weight;
            AddMult_a_VVt(w, shape, elmat1);
            for (int k = 0; k < num_state; k++)
            {
               elmat.AddMatrix(elmat1, num_nodes * k, num_nodes * k);
            }
         }
      }
   }

protected:
   mfem::Vector shape;
   mfem::DenseMatrix elmat;
   int num_state;
   /// cut-cell int rule
   std::map<int, IntegrationRule *> cutSquareIntRules;
   /// embedded elements boolean vector
   std::vector<bool> embeddedElements;
};

/// Integrator for forces due to pressure
/// \tparam dim - number of spatial dimensions (1, 2, or 3)
/// \tparam entvar - if true, states = ent. vars; otherwise, states = conserv.
/// \note This derived class uses the CRTP
template <int dim, bool entvar = false>
class CutDGPressureForce
 : public CutDGInviscidBoundaryIntegrator<CutDGPressureForce<dim, entvar>>
{
public:
   /// Constructs an integrator that computes pressure contribution to force
   /// \param[in] diff_stack - for algorithmic differentiation
   /// \param[in] fe_coll - used to determine the face elements
   /// \param[in] force_dir - unit vector specifying the direction of the force
   /// \param[in] cutSegmentIntRules - integration rule for cut segments
   /// \param[in] phi - level-set function (required for normal vector
   /// calculations)
   CutDGPressureForce(adept::Stack &diff_stack,
                      const mfem::FiniteElementCollection *fe_coll,
                      const mfem::Vector &force_dir,
                      std::map<int, IntegrationRule *> cutSegmentIntRules,
                      /*Algoim::LevelSet<2>*/ circle<2> phi)
    : CutDGInviscidBoundaryIntegrator<CutDGPressureForce<dim, entvar>>(
          diff_stack,
          fe_coll,
          cutSegmentIntRules,
          phi,
          dim + 2,
          1.0),
      force_nrm(force_dir),
      work_vec(dim + 2)
   { }

   /// Return an adjoint-consistent slip-wall normal (pressure) stress term
   /// \param[in] x - coordinate location at which stress is evaluated (not
   /// used) \param[in] dir - vector normal to the boundary at `x` \param[in] q
   /// - conservative variables at which to evaluate the stress \returns
   /// conmponent of stress due to pressure in `force_nrm` direction
   double calcBndryFun(const mfem::Vector &x,
                       const mfem::Vector &dir,
                       const mfem::Vector &q);

   /// Returns the gradient of the stress with respect to `q`
   /// \param[in] x - coordinate location at which stress is evaluated (not
   /// used) \param[in] dir - vector normal to the boundary at `x` \param[in] q
   /// - conservative variables at which to evaluate the stress \param[out]
   /// flux_vec - derivative of stress with respect to `q`
   void calcFlux(const mfem::Vector &x,
                 const mfem::Vector &dir,
                 const mfem::Vector &q,
                 mfem::Vector &flux_vec);

   /// Not used
   void calcFluxJacState(const mfem::Vector &x,
                         const mfem::Vector &dir,
                         const mfem::Vector &q,
                         mfem::DenseMatrix &flux_jac)
   { }

   /// Not used
   void calcFluxJacDir(const mfem::Vector &x,
                       const mfem::Vector &dir,
                       const mfem::Vector &q,
                       mfem::DenseMatrix &flux_jac)
   { }

private:
   /// `dim` entry unit normal vector specifying the direction of the force
   mfem::Vector force_nrm;
   /// work vector used to stored the flux
   mfem::Vector work_vec;
};

/// Source-term integrator for a 2D Euler MMS problem
/// \note For details on the MMS problem, see the file viscous_mms.py
/// \tparam dim - number of spatial dimensions (1, 2, or 3)
/// \tparam entvar - if true, states = ent. vars; otherwise, states = conserv.
/// \note This derived class uses the CRTP
template <int dim, bool entvar = false>
class CutEulerMMSIntegrator
 : public CutMMSIntegrator<CutEulerMMSIntegrator<dim, entvar>>
{
public:
   /// Construct an integrator for a 2D Navier-Stokes MMS source
   /// \param[in] diff_stack - for algorithmic differentiation
   /// \param[in] cutSquareIntRules - integration rule for cut cells
   /// \param[in] embeddedElements - elements completely inside the geometry
   /// \param[in] a - used to move residual to lhs (1.0) or rhs(-1.0)
   CutEulerMMSIntegrator(adept::Stack &diff_stack,
                         std::map<int, IntegrationRule *> cutSquareIntRules,
                         std::vector<bool> embeddedElements,
                         double a = 1.0)
    : CutMMSIntegrator<CutEulerMMSIntegrator<dim, entvar>>(cutSquareIntRules,
                                                           embeddedElements,
                                                           dim + 2,
                                                           a)
   { }

   /// Computes the MMS source term at a give point
   /// \param[in] x - spatial location at which to evaluate the source
   /// \param[out] src - source term evaluated at `x`
   void calcSource(const mfem::Vector &x, mfem::Vector &src) const
   {
      calcInviscidMMS<double>(x.GetData(), src.GetData());
   }

private:
};
/// Integrator for exact, prescribed BCs (with zero normal derivative)
/// \tparam dim - number of spatial dimensions (1, 2, or 3)
/// \note This derived class uses the CRTP
template <int dim, bool entvar>
class InviscidExactBC
 : public DGInviscidBoundaryIntegrator<InviscidExactBC<dim, entvar>>
{
public:
   /// Constructs an integrator for a viscous exact BCs
   /// \param[in] diff_stack - for algorithmic differentiation
   /// \param[in] fe_coll - used to determine the face elements
   /// \param[in] Re_num - Reynolds number
   /// \param[in] Pr_num - Prandtl number
   /// \param[in] q_far - state at the far-field
   /// \param[in] vis - viscosity (if negative use Sutherland's law)
   /// \param[in] a - used to move residual to lhs (1.0) or rhs(-1.0)
   InviscidExactBC(adept::Stack &diff_stack,
                   const mfem::FiniteElementCollection *fe_coll,
                   void (*fun)(const mfem::Vector &, mfem::Vector &),
                   double a = 1.0)
    : DGInviscidBoundaryIntegrator<InviscidExactBC<dim, entvar>>(diff_stack,
                                                                 fe_coll,
                                                                 dim + 2,
                                                                 a),
      qexact(dim + 2),
      work_vec(dim + 2)
   {
      exactSolution = fun;
   }

   /// Contracts flux with the entropy variables
   /// \param[in] x - coordinate location at which function is evaluated
   /// \param[in] dir - vector normal to the boundary at `x`
   /// \param[in] u - state at which to evaluate the function
   /// \param[in] Dw - `Dw[:,di]` is the derivative of `w` in direction `di`
   /// \returns fun - w^T*flux
   double calcBndryFun(const mfem::Vector &x,
                       const mfem::Vector &dir,
                       const mfem::Vector &q);

   /// Compute flux corresponding to an exact solution
   /// \param[in] x - coordinate location at which flux is evaluated (not used)
   /// \param[in] dir - vector normal to the boundary at `x`
   /// \param[in] q - conservative variables at which to evaluate the flux
   /// \param[out] flux_vec - value of the flux
   void calcFlux(const mfem::Vector &x,
                 const mfem::Vector &dir,
                 const mfem::Vector &q,
                 mfem::Vector &flux_vec);

   /// Compute jacobian of flux corresponding to an exact solution
   /// w.r.t `states`
   /// \param[in] x - coordinate location at which flux is evaluated
   /// \param[in] dir - vector normal to the boundary at `x`
   /// \param[in] q - conservative variables at which to evaluate the flux
   /// \param[out] flux_jac - jacobian of the flux
   void calcFluxJacState(const mfem::Vector &x,
                         const mfem::Vector &dir,
                         const mfem::Vector &q,
                         mfem::DenseMatrix &flux_jac);

   /// Compute jacobian of flux corresponding to an exact solution
   /// w.r.t `dir`
   /// \param[in] x - coordinate location at which flux is evaluated
   /// \param[in] dir - vector normal to the boundary at `x`
   /// \param[in] q - conservative variables at which to evaluate the flux
   /// \param[out] flux_jac - jacobian of the flux
   void calcFluxJacDir(const mfem::Vector &x,
                       const mfem::Vector &dir,
                       const mfem::Vector &q,
                       mfem::DenseMatrix &flux_jac);

private:
   /// Function to evaluate the exact solution at a given x value
   void (*exactSolution)(const mfem::Vector &, mfem::Vector &);
   /// far-field boundary state
   mfem::Vector qexact;
   /// work space for flux computations
   mfem::Vector work_vec;
};

/// Integrator for exact, prescribed BCs (with zero normal derivative)
/// \tparam dim - number of spatial dimensions (1, 2, or 3)
/// \note This derived class uses the CRTP
template <int dim, bool entvar>
class CutDGInviscidExactBC
 : public CutDGInviscidBoundaryIntegrator<CutDGInviscidExactBC<dim, entvar>>
{
public:
   /// Constructs an integrator for a viscous exact BCs
   /// \param[in] diff_stack - for algorithmic differentiation
   /// \param[in] fe_coll - used to determine the face elements
   /// \param[in] q_far - state at the far-field
   /// \param[in] vis - viscosity (if negative use Sutherland's law)
   /// \param[in] a - used to move residual to lhs (1.0) or rhs(-1.0)
   CutDGInviscidExactBC(adept::Stack &diff_stack,
                        const mfem::FiniteElementCollection *fe_coll,
                        std::map<int, IntegrationRule *> cutSegmentIntRules,
                        /*Algoim::LevelSet<2>*/ circle<2> phi,
                        void (*fun)(const mfem::Vector &, mfem::Vector &),
                        double a = 1.0)
    : CutDGInviscidBoundaryIntegrator<CutDGInviscidExactBC<dim, entvar>>(
          diff_stack,
          fe_coll,
          cutSegmentIntRules,
          phi,
          dim + 2,
          a),
      qexact(dim + 2),
      work_vec(dim + 2)
   {
      exactSolution = fun;
   }

   /// Contracts flux with the entropy variables
   /// \param[in] x - coordinate location at which function is evaluated
   /// \param[in] dir - vector normal to the boundary at `x`
   /// \param[in] u - state at which to evaluate the function
   /// \param[in] Dw - `Dw[:,di]` is the derivative of `w` in direction `di`
   /// \returns fun - w^T*flux
   double calcBndryFun(const mfem::Vector &x,
                       const mfem::Vector &dir,
                       const mfem::Vector &q);

   /// Compute flux corresponding to an exact solution
   /// \param[in] x - coordinate location at which flux is evaluated (not used)
   /// \param[in] dir - vector normal to the boundary at `x`
   /// \param[in] q - conservative variables at which to evaluate the flux
   /// \param[out] flux_vec - value of the flux
   void calcFlux(const mfem::Vector &x,
                 const mfem::Vector &dir,
                 const mfem::Vector &q,
                 mfem::Vector &flux_vec);

   /// Compute jacobian of flux corresponding to an exact solution
   /// w.r.t `states`
   /// \param[in] x - coordinate location at which flux is evaluated
   /// \param[in] dir - vector normal to the boundary at `x`
   /// \param[in] q - conservative variables at which to evaluate the flux
   /// \param[out] flux_jac - jacobian of the flux
   void calcFluxJacState(const mfem::Vector &x,
                         const mfem::Vector &dir,
                         const mfem::Vector &q,
                         mfem::DenseMatrix &flux_jac);
   /// Compute jacobian of flux corresponding to an exact solution
   /// w.r.t `dir`
   /// \param[in] x - coordinate location at which flux is evaluated
   /// \param[in] dir - vector normal to the boundary at `x`
   /// \param[in] q - conservative variables at which to evaluate the flux
   /// \param[out] flux_jac - jacobian of the flux
   void calcFluxJacDir(const mfem::Vector &x,
                       const mfem::Vector &dir,
                       const mfem::Vector &q,
                       mfem::DenseMatrix &flux_jac);

private:
   /// Function to evaluate the exact solution at a given x value
   void (*exactSolution)(const mfem::Vector &, mfem::Vector &);
   /// far-field boundary state
   mfem::Vector qexact;
   /// work space for flux computations
   mfem::Vector work_vec;
};
/// Source-term integrator for a 2D Euler MMS problem
/// \note For details on the MMS problem, see the file viscous_mms.py
/// \tparam dim - number of spatial dimensions (1, 2, or 3)
/// \tparam entvar - if true, states = ent. vars; otherwise, states = conserv.
/// \note This derived class uses the CRTP
template <int dim, bool entvar = false>
class CutPotentialMMSIntegrator
 : public CutMMSIntegrator<CutPotentialMMSIntegrator<dim, entvar>>
{
public:
   /// Construct an integrator for a 2D Navier-Stokes MMS source
   /// \param[in] diff_stack - for algorithmic differentiation
   /// \param[in] cutSquareIntRules - integration rule for cut cells
   /// \param[in] embeddedElements - elements completely inside the geometry
   /// \param[in] a - used to move residual to lhs (1.0) or rhs(-1.0)
   CutPotentialMMSIntegrator(adept::Stack &diff_stack,
                             std::map<int, IntegrationRule *> cutSquareIntRules,
                             std::vector<bool> embeddedElements,
                             double a = 1.0)
    : CutMMSIntegrator<CutPotentialMMSIntegrator<dim, entvar>>(
          cutSquareIntRules,
          embeddedElements,
          dim + 2,
          a)
   { }

   /// Computes the MMS source term at a give point
   /// \param[in] x - spatial location at which to evaluate the source
   /// \param[out] src - source term evaluated at `x`
   void calcSource(const mfem::Vector &x, mfem::Vector &src) const
   {
      calcPotentialMMS<double>(x.GetData(), src.GetData());
   }

private:
};

}  // namespace mach

#include "euler_integ_def_dg_cut.hpp"

#endif