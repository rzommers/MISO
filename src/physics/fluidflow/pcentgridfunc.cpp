#include "pcentgridfunc.hpp"
using namespace mfem;
/// functions related to parcentgridfunction
ParCentGridFunction::ParCentGridFunction(ParFiniteElementSpace *pf)
{
   SetSize(pf->GetVDim() * pf->GetNE());
   fes = pf;
   fec = NULL;
   // sequence = pf->GetSequence();
   UseDevice(true);
}

void ParCentGridFunction::ProjectCoefficient(VectorCoefficient &coeff)
{
   using namespace std;
   cout << "gd projectcoefficient() is called " << endl;
   int vdim = fes->GetVDim();
   Array<int> vdofs(vdim);
   Vector vals;
   int geom = fes->GetMesh()->GetElement(0)->GetGeometryType();
   const IntegrationPoint &cent = Geometries.GetCenter(geom);
   const FiniteElement *fe;
   ElementTransformation *eltransf;
   for (int i = 0; i < fes->GetNE(); i++)
   {
      fe = fes->GetFE(i);
      // Get the indices of dofs
      for (int j = 0; j < vdim; j++)
      {
         vdofs[j] = i * vdim + j;
      }

      eltransf = fes->GetElementTransformation(i);
      eltransf->SetIntPoint(&cent);
      vals.SetSize(vdofs.Size());
      coeff.Eval(vals, *eltransf, cent);

      if (fe->GetMapType() == 1)
      {
         vals(i) *= eltransf->Weight();
      }
      SetSubVector(vdofs, vals);
   }
}

/// Returns the true dofs in a new HypreParVector
HypreParVector *ParCentGridFunction::GetTrueDofs() const
{
   HypreParVector *tv = fes->NewTrueDofVector();
   GetTrueDofs(*tv);
   return tv;
}

ParCentGridFunction &ParCentGridFunction::operator=(const Vector &v)
{
   MFEM_ASSERT(fes && v.Size() == fes->GetTrueVSize(), "");
   Vector::operator=(v);
   return *this;
}

ParCentGridFunction &ParCentGridFunction::operator=(double value)
{
   Vector::operator=(value);
   return *this;
}