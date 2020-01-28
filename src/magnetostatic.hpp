#ifndef MACH_MAGNETOSTATIC
#define MACH_MAGNETOSTATIC

#include "mfem.hpp"
#include "adept.h"

#include "solver.hpp"
#include "coefficient.hpp"
#include "electromag_integ.hpp"

/// Operator that combines residual nonlinear form with right hand side vector
/// to go from solving r(A) = J to solve R(A) = 0
/// \note Intentionally not in mach namespace to avoid potential collisions.
/// \note Since this is an incomplete type I must use a pointer. The size of 
///       the class must be known to be able to declare a unique_ptr to the
///       class, so the class description must be in this header 
class ReducedSystemOperator : public mfem::Operator
{
public:
	ReducedSystemOperator(const mach::NonlinearFormType *res,
								 const mach::GridFunType *rhs,
								 const mfem::Array<int> &ess_bdr);
	
	/// compute r = res(A) - rhs
	void Mult(const mfem::Vector &A, mfem::Vector &r) const;

	/// compute the gradient of the residual
	mfem::Operator& GetGradient(const mfem::Vector &A) const;

private:
	const mach::NonlinearFormType *res;
	const mach::GridFunType *rhs;
	mfem::Array<int> ess_tdof_list;
};

namespace mach
{

/// Solver for magnetostatic electromagnetic problems
/// dim - number of spatial dimensions (only 3 supported)
class MagnetostaticSolver : public AbstractSolver
{
public:
	/// Class constructor.
   /// \param[in] opt_file_name - file where options are stored
   /// \param[in] smesh - if provided, defines the mesh for the problem
   /// \param[in] dim - number of dimensions
   /// \todo Can we infer dim some other way without using a template param?
   MagnetostaticSolver(const std::string &opt_file_name,
                       std::unique_ptr<mfem::Mesh> smesh = nullptr);

   /// Solve nonlinear magnetostatics problem using an MFEM Newton solver
   virtual void solveSteady();

private:
   std::unique_ptr<ReducedSystemOperator> reduced_op;
   /// Nedelec finite element collection
   std::unique_ptr<mfem::FiniteElementCollection> h_curl_coll;
   /// Raviart-Thomas finite element collection
   std::unique_ptr<mfem::FiniteElementCollection> h_div_coll;
   /// H(Curl) finite element space
   std::unique_ptr<SpaceType> h_curl_space;
   /// H(Div) finite element space
   std::unique_ptr<SpaceType> h_div_space;

   /// Magnetic vector potential A grid function
   std::unique_ptr<GridFunType> A;
   /// Magnetic flux density B = curl(A) grid function
   std::unique_ptr<GridFunType> B;

   /// TODO: delete? defined in abstract solver
   /// the spatial residual (a semilinear form)
   std::unique_ptr<NonlinearFormType> res;

   /// current source vector
   std::unique_ptr<GridFunType> current_vec;

   // /// current source vector with applied BC
   // std::unique_ptr<GridFunType> current_vec_BC;

   std::unique_ptr<mfem::Coefficient> neg_one;

   /// mesh dependent reluctivity coefficient
   std::unique_ptr<MeshDependentCoefficient> nu;
   /// vector mesh dependent current density function coefficient
   std::unique_ptr<VectorMeshDependentCoefficient> current_coeff;
   /// vector mesh dependent magnetization coefficient
   std::unique_ptr<VectorMeshDependentCoefficient> mag_coeff;

   /// boundary condition marker array
   mfem::Array<int> ess_bdr;
   std::unique_ptr<mfem::VectorCoefficient> bc_coef;

   /// linear system solver used in Newton's method
   std::unique_ptr<mfem::HypreGMRES> solver;
   /// linear system preconditioner used in Newton's method
   std::unique_ptr<EMPrecType> prec;

   /// Newton solver
   mfem::NewtonSolver newton_solver;

   /// Material Library
   nlohmann::json materials;

   int dim;

   int getNumState() {return dim;};

   /// static member variables used inside static member functions
   /// magnetization_source and winding_current_source
   /// values set by options in setStaticMembers
   static int num_poles;
   static double remnant_flux;
   static double mag_mu_r;

   /// set the values of static member variables used based on options file
   void setStaticMembers();

   /// construct mesh dependent coefficient for reluctivity
   /// \param[in] alpha - used to move to lhs or rhs
   void constructReluctivity();

   /// construct mesh dependent coefficient for magnetization
   /// \param[in] alpha - used to move to lhs or rhs
   void constructMagnetization();

   /// construct vector mesh dependent coefficient for current source
   /// \param[in] alpha - used to move to lhs or rhs
   void constructCurrent();

   /// TODO: throw MachException if constructCurrent not called first
   ///       introdue some current constructed flag?
   /// assemble vector associated with current source
   /// \note - constructCurrent must be called before calling this
   void assembleCurrentSource();

   /// Function to compute seconday fields
   /// For magnetostatics, computes the magnetic flux density
   void computeSecondaryFields();

   /// function describing current density in windings
   /// \param[in] x - position x in space of evaluation
   /// \param[out] J - current density at position x 
   static void winding_current_source(const mfem::Vector &x,
                                      mfem::Vector &J);

   /// TODO - implement function
   /// function describing permanent magnet magnetization source
   /// \param[in] x - position x in space
   /// \param[out] M - magetic flux density at position x cause by permanent
   ///                 magnets
   static void magnetization_source(const mfem::Vector &x,
                                    mfem::Vector &M);

   static void a_bc_uniform(const mfem::Vector &x, mfem::Vector &A);

   static void sol_b_analytic(const mfem::Vector &x, mfem::Vector &B);
};

} // namespace mach

#endif