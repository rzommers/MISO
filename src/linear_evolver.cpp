#include "linear_evolver.hpp"

using namespace mfem;
using namespace std;

namespace mach
{

Linear_Evolver::Linear_Evolver(SparseMatrix &mass, SparseMatrix &stiff) //, const Vector &_b)
   : TimeDependentOperator(mass.Size()), M(mass), K(stiff), Minv(mass.Size()), z(mass.Size()) //b(_b), z(_M.Size())
{
    // Here we extract the diagonal from the mass matrix and invert it
    M.GetDiag(z);
    ElementInv(z, Minv);
}

void Linear_Evolver::Mult(const Vector &x, Vector &y) const
{
   // y = M^{-1} (K x)
   K.Mult(x, z);
   HadamardProd(Minv, z, y);
}

}