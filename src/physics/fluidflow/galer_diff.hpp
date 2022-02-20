#ifndef MFEM_DGDSpace
#define MFEM_DGDSpace

#include "mfem.hpp"
#include "mach_types.hpp"
namespace mfem
{

// need to think a new name for this class
class DGDSpace : public mfem::FiniteElementSpace
{
public:
   /// class constructor
   DGDSpace(mfem::Mesh *m, const mfem::FiniteElementCollection *fec,
            mfem::Array<Vector*> center, int degree, int extra, int vdim = 1,
            int ordering = mfem::Ordering::byVDIM);
   virtual ~DGDSpace();

   /// build the prolongation matrix
   void buildProlongation() const;

   /// build the dof coordinate matrix
   /// note: Assume the mesh only has one type of element
   /// \param[in] el_id - global element id
   /// \param[in/out dofs - matrix that hold the dofs' coordinates
   void buildDofMat(int el_id, const int num_dofs, 
                    const mfem::FiniteElement *fe,
                    mfem::Array<mfem::Vector *> &dofs) const;
   
   void buildDataMat(const int el_id, mfem::DenseMatrix &V,
                     mfem::DenseMatrix &Vn) const;

   /// Solve and store the local prolongation coefficient
   /// \param[in] el_id - the element id
   /// \param[in] numDofs - degrees of freedom in the element
   /// \param[in] dof_coord - dofs coordinat location
   void solveLocalProlongationMat(const int el_id, const mfem::DenseMatrix &V,
                                     const mfem::DenseMatrix &Vn,
                                     mfem::DenseMatrix &localMat) const;

   /// build the element-wise polynomial basis matrix
   void buildElementPolyBasisMat(const int el_id, const int numDofs,
                                 const mfem::Array<mfem::Vector *> &dofs_coord,
                                 mfem::DenseMatrix &V,
                                 mfem::DenseMatrix &Vn) const;

   /// Assemble the local prolongation to the global matrix
   void AssembleProlongationMatrix(const int el_id, const mfem::DenseMatrix &localMat) const;

   virtual int GetTrueVSize() const {return vdim * numBasis;}
   inline int GetNDofs() const {return numBasis;}
   SparseMatrix *GetCP() { return cP; }
   const Operator *GetProlongationMatrix() const
   { 
      if (!cP)
      {
         buildProlongation();
         return cP;
      }
      else
      {
         return cP; 
      }
   }
protected:
   /// mesh dimension
   int dim;
   /// number of radial basis function
   int numBasis;
   /// number of polynomial basis
   int numPolyBasis;
   /// polynomial order
   int polyOrder;
   /// number of required basis to constructe certain order polynomial
   int numLocalBasis;
   int extra;
   
   /// location of the basis centers
   mfem::Array<mfem::Vector *> basisCenter;
   /// selected basis for each element (currently it is fixed upon setup)
   mfem::Array<mfem::Array<int> *> selectedBasis;
   mfem::Array<mfem::Array<int> *> selectedElement;
   /// store the element centers
   mfem::Array<mfem::Vector *> elementCenter;
   /// array of map that holds the distance from element center to basisCenter
   // mfem::Array<std::map<int, double> *> elementBasisDist;
   mfem::Array<std::vector<double> *> elementBasisDist;
   // local element prolongation matrix coefficient
   mutable mfem::Array<mfem::DenseMatrix *> coef;
   /// Initialize the patches/stencil given poly order


   // some protected function
   void InitializeStencil();
   /// Initialize the shape parameters
   void InitializeShapeParameter();
   std::vector<std::size_t> sort_indexes(const std::vector<double> &v);
};

} // end of namespace mfem
#endif