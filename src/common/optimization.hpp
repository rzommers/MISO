#ifndef MACH_OPTIMIZATION
#define MACH_OPTIMIZATION

#include "solver.hpp"
#include "galer_diff.hpp"
namespace mach
{
class Optimizer
{
public:
   // default constructor
   Optimizer() {};

   /// Given the design variable may take different form
   virtual double ComputeObject() = 0;
   virtual Operator *GetGradient() = 0;
   virtual ~Optimizer() = default;
};


class DGDOptimizer : public NonlinearForm
{
public:
   /// class constructor
   DGDOptimizer(mfem::FiniteElementSpace *fes,
	 				 mfem::DGDSpace *fes_dgd);
   
   virtual double GetEnergy(const Vector &x) const;

   /// compute the jacobian of the functional w.r.t the design variable
   virtual Operator &GetGradient(const mfem::Vector &x) const;

   /// class destructor
   ~DGDOptimizer();
protected:
   /// do I want to keep all the objectives here,
   ///  or simply use the EulerSolver?
   /// for now I would like to develope new class to have more flexibility
   /// some basic variables
   /// the design variables
   Vector design_var;
   /// number of design variable
   int numVar;
   /// number of basis
   int numBasis;

   std::unique_ptr<mfem::Mesh> mesh;
   std::unique_ptr<mfem::FiniteElementCollection> fec;
   std::unique_ptr<mfem::DGDSpace> fes_dgd;
	// the constraints
   std::unique_ptr<NonlinearFormType> res_dgd;
	std::unique_ptr<NonlinearFormType> res_full;

   // some working variables
   std::unique_ptr<mfem::CentGridFunction> u_dgd;
   std::unique_ptr<mfem::GridFunction> u_full;
};

} // end of namesapce
#endif