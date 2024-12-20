#include <random>
#include <cmath>

#include "utils.hpp"

using namespace mfem;
using namespace std;

namespace miso
{
/// performs the Hadamard (elementwise) product: `v(i) = v1(i)*v2(i)`
void multiplyElementwise(const Vector &v1, const Vector &v2, Vector &v)
{
   MFEM_ASSERT(v1.Size() == v2.Size() && v1.Size() == v.Size(), "");
   for (int i = 0; i < v.Size(); ++i)
   {
      v(i) = v1(i) * v2(i);
   }
}

/// performs the Hadamard (elementwise) product: `a(i) *= b(i)`
void multiplyElementwise(const Vector &b, Vector &a)
{
   MFEM_ASSERT(a.Size() == b.Size(), "");
   for (int i = 0; i < a.Size(); ++i)
   {
      a(i) *= b(i);
   }
}

/// performs an elementwise division: `v(i) = v1(i)/v2(i)`
void divideElementwise(const Vector &v1, const Vector &v2, Vector &v)
{
   MFEM_ASSERT(v1.Size() == v2.Size() && v1.Size() == v.Size(), "");
   for (int i = 0; i < v.Size(); ++i)
   {
      v(i) = v1(i) / v2(i);
   }
}

/// performs elementwise inversion: `y(i) = 1/x(i)`
void invertElementwise(const Vector &x, Vector &y)
{
   MFEM_ASSERT(x.Size() == y.Size(), "");
   for (int i = 0; i < x.Size(); ++i)
   {
      y(i) = 1.0 / x(i);
   }
}

double squaredExponential(double len, const Vector &xc, const Vector &x)
{
   double prod2 = x * x - 2.0 * (x * xc) + xc * xc;  // avoid memory allocation
   return exp(-prod2 / pow(len, 2.0));
}

/// Handles print in parallel case
template <typename CharT, typename Traits>
class basic_oblackholestream : virtual public std::basic_ostream<CharT, Traits>
{
public:
   /// called when rank is not root, prints nothing
   explicit basic_oblackholestream()
    : std::basic_ostream<CharT, Traits>(nullptr)
   { }
};  // end class basic_oblackholestream

using oblackholestream = basic_oblackholestream<char, std::char_traits<char>>;
static oblackholestream obj;

std::ostream *getOutStream(int rank, bool silent)
{
   if (!silent)
   {
      /// print only on root
      if (0 == rank)
      {
         return &std::cout;
      }
      else
      {
         return &obj;
      }
   }
   else
   {
      return &obj;
   }
}

HypreParVector bufferToHypreParVector(double *buffer,
                                      const ParFiniteElementSpace &fes)
{
   return {
       fes.GetComm(), fes.GlobalTrueVSize(), buffer, fes.GetTrueDofOffsets()};
}

void getMFEMBoundaryArray(const nlohmann::json &boundary,
                          mfem::Array<int> &bdr_arr)
{
   bdr_arr = 0;
   if (boundary.is_string())
   {
      auto tmp = boundary.get<std::string>();
      if (tmp == "all")
      {
         bdr_arr = 1;
      }
      else if (tmp == "none")
      {
         bdr_arr = 0;
      }
      else
      {
         throw MISOException("Unrecognized string for boundary!");
      }
   }
   else if (boundary.is_array())
   {
      auto tmp = boundary.get<std::vector<int>>();
      attrVecToArray(tmp, bdr_arr);
   }
   else
   {
      throw MISOException("Unrecognized JSON value for boundary!");
   }
}

void attrVecToArray(const std::vector<int> &vec_attributes,
                    mfem::Array<int> &attributes)
{
   attributes = 0;
   for (const auto &attr : vec_attributes)
   {
      attributes[attr - 1] = 1;
   }
}

/// performs quadratic interpolation given x0, y0, dy0/dx0, x1, and y1.
double quadInterp(double x0, double y0, double dydx0, double x1, double y1)
{
   // Assume the fuction has the form y(x) = c0 + c1 * x + c2 * x^2
   // double c0 = (dydx0 * x0 * x0 * x1 + y1 * x0 * x0 - dydx0 * x0 * x1 * x1 -
   // 2 * y0 * x0 * x1 + y0 * x1 * x1) /
   //   (x0 * x0 - 2 * x1 * x0 + x1 * x1);
   double c1 = (2 * x0 * y0 - 2 * x0 * y1 - x0 * x0 * dydx0 + x1 * x1 * dydx0) /
               (x0 * x0 - 2 * x1 * x0 + x1 * x1);
   double c2 =
       -(y0 - y1 - x0 * dydx0 + x1 * dydx0) / (x0 * x0 - 2 * x1 * x0 + x1 * x1);
   return -c1 / (2 * c2);
}

// DiscreteGradOperator::DiscreteGradOperator(SpaceType *dfes,
//                                            SpaceType *rfes)
//    : ParDiscreteLinearOperator(dfes, rfes)
// {
//    this->AddDomainInterpolator(new GradientInterpolator);
// }

// DiscreteCurlOperator::DiscreteCurlOperator(SpaceType *dfes,
//                                            SpaceType *rfes)
//    : ParDiscreteLinearOperator(dfes, rfes)
// {
//    this->AddDomainInterpolator(new CurlInterpolator);
// }

// DiscreteDivOperator::DiscreteDivOperator(SpaceType *dfes,
//                                          SpaceType *rfes)
//    : ParDiscreteLinearOperator(dfes, rfes)
// {
//    this->AddDomainInterpolator(new DivergenceInterpolator);
// }

// IrrotationalProjector::IrrotationalProjector(ParFiniteElementSpace &h1_fes,
//                                              ParFiniteElementSpace &nd_fes,
//                                              const int &ir_order)
//    : h1_fes(h1_fes), nd_fes(nd_fes), s0(&h1_fes), weakDiv(&nd_fes, &h1_fes),
//    grad(&h1_fes, &nd_fes), psi(&h1_fes), xDiv(&h1_fes), pcg(h1_fes.GetComm())
// {
//    /// not sure if theres a better way to handle this
//    ess_bdr.SetSize(h1_fes.GetParMesh()->bdr_attributes.Max());
//    ess_bdr = 1;
//    h1_fes.GetEssentialTrueDofs(ess_bdr, ess_bdr_tdofs);

//    int geom = h1_fes.GetFE(0)->GetGeomType();
//    const IntegrationRule * ir = &IntRules.Get(geom, ir_order);

//    BilinearFormIntegrator *diffInteg = new DiffusionIntegrator;
//    diffInteg->SetIntRule(ir);
//    s0.AddDomainIntegrator(diffInteg);
//    s0.Assemble();
//    s0.Finalize();

//    BilinearFormIntegrator *wdivInteg = new VectorFEWeakDivergenceIntegrator;
//    wdivInteg->SetIntRule(ir);
//    weakDiv.AddDomainIntegrator(wdivInteg);
//    weakDiv.Assemble();
//    weakDiv.Finalize();
//    grad.Assemble();
//    grad.Finalize();
// }

// void IrrotationalProjector::Mult(const Vector &x, Vector &y) const
// {
//    // Compute the divergence of x
//    weakDiv.Mult(x, xDiv); xDiv *= -1.0;
//    std::cout << "weakdiv mult\n";

//    // Apply essential BC and form linear system
//    psi = 0.0;
//    s0.FormLinearSystem(ess_bdr_tdofs, psi, xDiv, S0, Psi, RHS);
//    std::cout << "form lin system\n";

//    amg.SetOperator(S0);
//    amg.SetPrintLevel(0);

//    pcg.SetOperator(S0);
//    pcg.SetTol(1e-14);
//    pcg.SetMaxIter(200);
//    pcg.SetPrintLevel(0);
//    pcg.SetPreconditioner(amg);

//    // Solve the linear system for Psi
//    pcg.Mult(RHS, Psi);
//    std::cout << "pcg mult\n";

//    // Compute the parallel grid function correspoinding to Psi
//    s0.RecoverFEMSolution(Psi, xDiv, psi);

//    // Compute the irrotational portion of x
//    grad.Mult(psi, y);
// }

// void
// IrrotationalProjector::Update()
// {
//    psi.Update();
//    xDiv.Update();

//    s0.Update();
//    s0.Assemble();
//    s0.Finalize();

//    weakDiv.Update();
//    weakDiv.Assemble();
//    weakDiv.Finalize();

//    grad.Update();
//    grad.Assemble();
//    grad.Finalize();

//    h1_fes.GetEssentialTrueDofs(ess_bdr, ess_bdr_tdofs);
// }

// DivergenceFreeProjector::DivergenceFreeProjector(ParFiniteElementSpace
// &h1_fes,
//                                                  ParFiniteElementSpace
//                                                  &nd_fes, const int
//                                                  &ir_order)
//    : IrrotationalProjector(h1_fes, nd_fes, ir_order)
// {}

// void DivergenceFreeProjector::Mult(const Vector &x, Vector &y) const
// {
//    std::cout << "above irrot proj mult\n";
//    this->IrrotationalProjector::Mult(x, y);
//    std::cout << "below irrot proj mult\n";
//    y  -= x;
//    y *= -1.0;
// }

// void DivergenceFreeProjector::vectorJacobianProduct(
//    const mfem::ParGridFunction &proj_bar,
//    std::string wrt,
//    mfem::ParGridFunction &wrt_bar)
// {
//    if (wrt == "wrt")
//    {

//    }
// }

// void DivergenceFreeProjector::Update()
// {
//    this->IrrotationalProjector::Update();
// }

double bisection(const std::function<double(double)> &func,
                 double xl,
                 double xr,
                 double ftol,
                 double xtol,
                 int maxiter)
{
   double fl = func(xl);
   double fr = func(xr);
   if (fl * fr > 0.0)
   {
      cerr << "bisection: func(xl)*func(xr) is positive." << endl;
      throw(-1);
   }
   double xm = 0.5 * (xl + xr);
   double fm = func(xm);
   int iter = 0;
   while ((abs(fm) > ftol) && (abs(xr - xl) > xtol) && (iter < maxiter))
   {
      iter++;
      // cout << "iter = " << iter << ": f(x) = " << fm << endl;
      if (fm * fl < 0.0)
      {
         xr = xm;
         fr = fm;
      }
      else if (fm * fr < 0.0)
      {
         xl = xm;
         fl = fm;
      }
      else
      {
         break;
      }
      xm = 0.5 * (xl + xr);
      fm = func(xm);
   }
   if (iter >= maxiter)
   {
      cerr << "bisection: failed to find a solution in maxiter." << endl;
      throw(-1);
   }
   return xm;
}

double secant(const std::function<double(double)> &func,
              double x1,
              double x2,
              double ftol,
              double xtol,
              int maxiter)
{
   double f1 = func(x1);
   double f2 = func(x2);
   double x = NAN;
   double f = NAN;
   if (fabs(f1) < fabs(f2))
   {
      // swap x1 and x2 if the latter gives a smaller value
      x = x2;
      f = f2;
      x2 = x1;
      f2 = f1;
      x1 = x;
      f1 = f;
   }
   x = x2;
   f = f2;
   int iter = 0;
   while ((fabs(f) > ftol) && (iter < maxiter))
   {
      ++iter;
      try
      {
         double dx = f2 * (x2 - x1) / (f2 - f1);
         x -= dx;
         f = func(x);
         if (fabs(dx) < xtol)
         {
            break;
         }
         x1 = x2;
         f1 = f2;
         x2 = x;
         f2 = f;
      }
      catch (std::exception &exception)
      {
         cerr << "secant: " << exception.what() << endl;
      }
   }
   if (iter > maxiter)
   {
      throw MISOException("secant: maximum number of iterations exceeded");
   }
   return x;
}

#ifndef MFEM_USE_LAPACK
void dgelss_(int *,
             int *,
             int *,
             double *,
             int *,
             double *,
             int *,
             double *,
             double *,
             int *,
             double *,
             int *,
             int *);
void dgels_(char *,
            int *,
            int *,
            int *,
            double *,
            int *,
            double *,
            int *,
            double *,
            int *,
            int *);
#else
extern "C" void dgelss_(int *,
                        int *,
                        int *,
                        double *,
                        int *,
                        double *,
                        int *,
                        double *,
                        double *,
                        int *,
                        double *,
                        int *,
                        int *);
extern "C" void dgels_(char *,
                       int *,
                       int *,
                       int *,
                       double *,
                       int *,
                       double *,
                       int *,
                       double *,
                       int *,
                       int *);
#endif
/// build the interpolation operator on element patch
/// this function will be moved later
#ifdef MFEM_USE_LAPACK
void buildInterpolation(int dim,
                        int degree,
                        const DenseMatrix &x_center,
                        const DenseMatrix &x_quad,
                        DenseMatrix &interp)
{
   // number of quadrature points
   int num_quad = x_quad.Width();
   // number of elements
   int num_el = x_center.Width();

   // number of row and colomn in r matrix
   int m = (degree + 1) * (degree + 2) /
           2;  // in 1 and 3D the size will be different
   int n = num_el;
   if (1 == dim)
   {
      m = degree + 1;
   }
   else if (2 == dim)
   {
      m = (degree + 1) * (degree + 2) / 2;
   }
   else
   {
      throw MISOException(
          "Other dimension interpolation has not been implemented yet.\n");
   }

   // Set the size of interpolation operator
   interp.SetSize(num_quad, num_el);
   // required by the lapack routine
   mfem::DenseMatrix rhs(n, 1);
   char TRANS = 'N';
   int nrhs = 1;
   int lwork = 2 * m * n;
   double work[lwork];

   // construct each row of R (also loop over each quadrature point)
   for (int i = 0; i < num_quad; i++)
   {
      // reset the rhs
      rhs = 0.0;
      rhs(0, 0) = 1.0;
      // construct the aux matrix to solve each row of R
      DenseMatrix r(m, n);
      r = 0.0;
      // loop over each column of r
      for (int j = 0; j < n; j++)
      {
         if (1 == dim)
         {
            double x_diff = x_center(0, j) - x_quad(0, i);
            r(0, j) = 1.0;
            for (int order = 1; order < m; order++)
            {
               r(order, j) = pow(x_diff, order);
            }
         }
         else if (2 == dim)
         {
            double x_diff = x_center(0, j) - x_quad(0, i);
            double y_diff = x_center(1, j) - x_quad(1, i);
            r(0, j) = 1.0;
            int index = 1;
            // loop over different orders
            for (int order = 1; order <= degree; order++)
            {
               for (int c = order; c >= 0; c--)
               {
                  r(index, j) = pow(x_diff, c) * pow(y_diff, order - c);
                  index++;
               }
            }
         }
         else
         {
            throw MISOException(
                "Other dimension interpolation has not been implemented "
                "yet.\n");
         }
      }
      // Solve each row of R and put them back to R
      int info, rank;
      dgels_(&TRANS,
             &m,
             &n,
             &nrhs,
             r.GetData(),
             &m,
             rhs.GetData(),
             &n,
             work,
             &lwork,
             &info);
      MFEM_ASSERT(info == 0, "Fail to solve the underdetermined system.\n");
      // put each row back to interp
      for (int k = 0; k < n; k++)
      {
         interp(i, k) = rhs(k, 0);
      }
   }
}  // end of constructing interp
#endif

#ifdef MFEM_USE_LAPACK
void buildLSInterpolation(int dim,
                          int degree,
                          const DenseMatrix &x_center,
                          const DenseMatrix &x_quad,
                          DenseMatrix &interp)
{
   // get the number of quadrature points and elements.
   int num_quad = x_quad.Width();
   int num_elem = x_center.Width();

   // number of total polynomial basis functions
   int num_basis = -1;
   if (1 == dim)
   {
      num_basis = degree + 1;
   }
   else if (2 == dim)
   {
      num_basis = (degree + 1) * (degree + 2) / 2;
   }
   else if (3 == dim)
   {
      num_basis = (degree + 1) * (degree + 2) * (degree + 3) / 6;
   }
   else
   {
      throw MISOException("buildLSInterpolation: dim must be 3 or less.\n");
   }

   // Construct the generalized Vandermonde matrix
   mfem::DenseMatrix V(num_elem, num_basis);
   if (1 == dim)
   {
      for (int i = 0; i < num_elem; ++i)
      {
         double dx = x_center(0, i) - x_center(0, 0);
         for (int p = 0; p <= degree; ++p)
         {
            V(i, p) = pow(dx, p);
         }
      }
   }
   else if (2 == dim)
   {
      for (int i = 0; i < num_elem; ++i)
      {
         double dx = x_center(0, i) - x_center(0, 0);
         double dy = x_center(1, i) - x_center(1, 0);
         int col = 0;
         for (int p = 0; p <= degree; ++p)
         {
            for (int q = 0; q <= p; ++q)
            {
               V(i, col) = pow(dx, p - q) * pow(dy, q);
               ++col;
            }
         }
      }
   }
   else if (3 == dim)
   {
      for (int i = 0; i < num_elem; ++i)
      {
         double dx = x_center(0, i) - x_center(0, 0);
         double dy = x_center(1, i) - x_center(1, 0);
         double dz = x_center(2, i) - x_center(2, 0);
         int col = 0;
         for (int p = 0; p <= degree; ++p)
         {
            for (int q = 0; q <= p; ++q)
            {
               for (int r = 0; r <= p - q; ++r)
               {
                  V(i, col) = pow(dx, p - q - r) * pow(dy, r) * pow(dz, q);
                  ++col;
               }
            }
         }
      }
   }

   // Set the RHS for the LS problem (it's the identity matrix)
   // This will store the solution, that is, the basis coefficients, hence
   // the name `coeff`
   mfem::DenseMatrix coeff(num_elem, num_elem);
   coeff = 0.0;
   for (int i = 0; i < num_elem; ++i)
   {
      coeff(i, i) = 1.0;
   }

   // Set-up and solve the least-squares problem using LAPACK's dgels
   char TRANS = 'N';
   int info;
   int lwork = 2 * num_elem * num_basis;
   double work[lwork];
   dgels_(&TRANS,
          &num_elem,
          &num_basis,
          &num_elem,
          V.GetData(),
          &num_elem,
          coeff.GetData(),
          &num_elem,
          work,
          &lwork,
          &info);
   MFEM_ASSERT(info == 0, "Fail to solve the underdetermined system.\n");

   // Perform matrix-matrix multiplication between basis functions evalauted at
   // quadrature nodes and basis function coefficients.
   interp.SetSize(num_quad, num_elem);
   interp = 0.0;
   if (1 == dim)
   {
      // loop over quadrature points
      for (int j = 0; j < num_quad; ++j)
      {
         double dx = x_quad(0, j) - x_center(0, 0);
         // loop over the element centers
         for (int i = 0; i < num_elem; ++i)
         {
            for (int p = 0; p <= degree; ++p)
            {
               interp(j, i) += pow(dx, p) * coeff(p, i);
            }
         }
      }
   }
   else if (2 == dim)
   {
      // loop over quadrature points
      for (int j = 0; j < num_quad; ++j)
      {
         double dx = x_quad(0, j) - x_center(0, 0);
         double dy = x_quad(1, j) - x_center(1, 0);
         // loop over the element centers
         for (int i = 0; i < num_elem; ++i)
         {
            int col = 0;
            for (int p = 0; p <= degree; ++p)
            {
               for (int q = 0; q <= p; ++q)
               {
                  interp(j, i) += pow(dx, p - q) * pow(dy, q) * coeff(col, i);
                  ++col;
               }
            }
         }
      }
   }
   else if (dim == 3)
   {
      // loop over quadrature points
      for (int j = 0; j < num_quad; ++j)
      {
         double dx = x_quad(0, j) - x_center(0, 0);
         double dy = x_quad(1, j) - x_center(1, 0);
         double dz = x_quad(2, j) - x_center(2, 0);
         // loop over the element centers
         for (int i = 0; i < num_elem; ++i)
         {
            int col = 0;
            for (int p = 0; p <= degree; ++p)
            {
               for (int q = 0; q <= p; ++q)
               {
                  for (int r = 0; r <= p - q; ++r)
                  {
                     interp(j, i) += pow(dx, p - q - r) * pow(dy, r) *
                                     pow(dz, q) * coeff(col, i);
                     ++col;
                  }
               }
            }
         }
      }
   }
}
#endif

#ifdef MFEM_USE_GSLIB
void transferSolution(MeshType &old_mesh,
                      MeshType &new_mesh,
                      const GridFunType &in,
                      GridFunType &out)
{
   const int dim = old_mesh.Dimension();

   old_mesh.EnsureNodes();
   new_mesh.EnsureNodes();

   Vector vxyz = *(new_mesh.GetNodes());
   const int nodes_cnt = vxyz.Size() / dim;
   FindPointsGSLIB finder(MPI_COMM_WORLD);
   const double rel_bbox_el = 0.05;
   const double newton_tol = 1.0e-12;
   const int npts_at_once = 256;

   // Get the values at the nodes of mesh 1.
   Array<unsigned int> el_id_out(nodes_cnt), code_out(nodes_cnt),
       task_id_out(nodes_cnt);
   Vector pos_r_out(nodes_cnt * dim), dist_p_out(nodes_cnt * dim),
       interp_vals(nodes_cnt);
   finder.Setup(old_mesh, rel_bbox_el, newton_tol, npts_at_once);
   finder.FindPoints(
       vxyz, code_out, task_id_out, el_id_out, pos_r_out, dist_p_out);
   finder.Interpolate(
       code_out, task_id_out, el_id_out, pos_r_out, in, interp_vals);
   for (int n = 0; n < nodes_cnt; n++)
   {
      out(n) = interp_vals(n);
   }
   finder.FreeData();
}
#else
void transferSolution(MeshType &old_mesh,
                      MeshType &new_mesh,
                      const GridFunType &in,
                      GridFunType &out)
{
   throw MISOException(
       "transferSolution requires GSLIB!"
       "\trecompile MFEM with GSLIB!");
}
#endif

unique_ptr<Mesh> buildQuarterAnnulusMesh(int degree,
                                         int num_rad,
                                         int num_ang,
                                         double pert)
{
   Mesh mesh = Mesh::MakeCartesian2D(num_rad,
                                     num_ang,
                                     Element::TRIANGLE,
                                     true /* gen. edges */,
                                     2.0,
                                     M_PI * 0.5,
                                     true);

   static constexpr double eps = std::numeric_limits<double>::epsilon();
   if (pert > eps)
   {
      // randomly perturb the interior vertices
      std::default_random_engine gen(std::random_device{}());
      std::uniform_real_distribution<double> uni_rand(-pert, pert);
      for (int i = 0; i < mesh.GetNV(); ++i)
      {
         double *vertex = mesh.GetVertex(i);
         // make sure vertex is interior
         if (vertex[0] > eps && vertex[0] < 2.0 - eps && vertex[1] > eps &&
             vertex[1] < M_PI * 0.5 - eps)
         {
            // perturb coordinates
            vertex[0] += uni_rand(gen) * 2.0 / num_rad;
            vertex[1] += uni_rand(gen) * M_PI * 0.5 / num_ang;
         }
      }
   }

   // strategy:
   // 1) generate a fes for Lagrange elements of desired degree
   // 2) create a Grid Function using a VectorFunctionCoefficient
   // 4) use mesh_ptr->NewNodes(nodes, true) to set the mesh nodes

   // fes does not own fec, which is generated in this function's scope, but
   // the grid function can own both the fec and fes
   auto *fec = new H1_FECollection(degree, 2 /* = dim */);
   auto *fes = new FiniteElementSpace(&mesh, fec, 2, Ordering::byVDIM);

   // This lambda function transforms from (r,\theta) space to (x,y) space
   auto xy_fun = [](const Vector &rt, Vector &xy)
   {
      xy(0) =
          (rt(0) + 1.0) * cos(rt(1));  // need + 1.0 to shift r away from origin
      xy(1) = (rt(0) + 1.0) * sin(rt(1));
   };
   VectorFunctionCoefficient xy_coeff(2, xy_fun);
   auto *xy = new GridFunction(fes);
   xy->MakeOwner(fec);
   xy->ProjectCoefficient(xy_coeff);

   mesh.NewNodes(*xy, true);
   return make_unique<Mesh>(mesh);
}

}  // namespace miso
