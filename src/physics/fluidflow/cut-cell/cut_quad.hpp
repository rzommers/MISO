#ifndef MACH_CUT_QUAD
#define MACH_CUT_QUAD
#include "mfem.hpp"
#include <fstream>
#include <iostream>
// #include "algoim_quad.hpp"
#include "algoim_levelset.hpp"
#include <list>
using namespace mfem;
using namespace std;
using namespace Algoim;
namespace mach
{
template <int N>
struct circle
{
   double xscale;
   double yscale;
   double min_x;
   double min_y;
   double radius;
   double xc = 0.5;
   double yc = 0.5;
   template <typename T>
   T operator()(const blitz::TinyVector<T, N> &x) const
   {
      // level-set function to work in physical space
      // return -1 * (((x[0] - 5) * (x[0] - 5)) +
      //               ((x[1]- 5) * (x[1] - 5)) - (0.5 * 0.5));
      // level-set function for reference elements
      return -1 *
             ((((x[0] * xscale) + min_x - xc) * ((x[0] * xscale) + min_x - xc)) +
              (((x[1] * yscale) + min_y - yc) * ((x[1] * yscale) + min_y - yc)) -
              (radius * radius));
   }
   template <typename T>
   blitz::TinyVector<T, N> grad(const blitz::TinyVector<T, N> &x) const
   {
      // return blitz::TinyVector<T, N>(-1 * (2.0 * (x(0) - 5)), -1 * (2.0 *
      // (x(1) - 5)));
      return blitz::TinyVector<T, N>(
          -1 * (2.0 * xscale * ((x(0) * xscale) + min_x - xc)),
          -1 * (2.0 * yscale * ((x(1) * yscale) + min_y - yc)));
   }
};
template <int N>
class CutCell
{
public:
   CutCell(mfem::Mesh *_mesh) : mesh(_mesh)
   {
      phi = constructLevelSet();
   }
   /// construct levelset using given geometry points
   Algoim::LevelSet<2> constructLevelSet()
   {
      std::vector<TinyVector<double, N>> Xc;
      std::vector<TinyVector<double, N>> nor;
      int nbnd = 256;
      cout << "nbnd " << nbnd << endl;
      /// parameters
      double rho = 10.0 * nbnd;
      double delta = 1e-10;
      /// radius
      double a = 0.5;
      for (int k = 0; k < nbnd; ++k)
      {
         double theta = k * 2.0 * M_PI / nbnd;
         TinyVector<double, N> x, nrm;
         x(0) = a * cos(theta) + 0.5;
         x(1) = a * sin(theta) + 0.5;
         nrm(0) = 2.0 * (x(0) - 0.5);
         nrm(1) = 2.0 * (x(1) - 0.5);
         double ds = mag(nrm);
         TinyVector<double, N> ni;
         ni = nrm / ds;
         Xc.push_back(x);
         nor.push_back(ni);
      }
      /// initialize levelset
      //phi.LevelSet(Xc, nor, rho, delta);
      phi.initializeLevelSet(Xc, nor, rho, delta);
      phi.xscale = 1.0;
      phi.yscale = 1.0;
      phi.min_x = 0.0;
      phi.min_y = 0.0;
      // phi.radius = 0.5;
      TinyVector<double, N> x;
      x(0) = 0.2;
      x(1) = 0.8;

      std::cout << std::setprecision(10) << std::endl;
      cout << "phi " << phi(x) << endl;
      cout << "grad phi "<< phi.grad(x) << endl;
      phi_e.xscale = 1.0;
      phi_e.yscale = 1.0;
      phi_e.min_x = 0.0;
      phi_e.min_y = 0.0;
      phi_e.radius = 0.5;
      cout << "exact phi " << phi_e(x) << endl;
      cout << "exact grad phi "<< phi_e.grad(x) << endl;
      cout << "norm vectors " << endl;
      blitz::TinyVector<double, 2> beta, beta_e;
      beta = phi.grad(x);
      beta_e = phi_e.grad(x);
      double xc = 0.5;
      double yc = 0.5;
      double nx = beta(0);
      double ny = beta(1);
      double nx_e = beta_e(0);
      double ny_e = beta_e(1);
      double ds = sqrt((nx * nx) + (ny * ny));
      double ds_e = sqrt((nx_e * nx_e) + (ny_e * ny_e));
      Vector nrm, nrm_e;
      nrm.SetSize(2);
      nrm_e.SetSize(2);
      nrm(0) = -nx / ds;
      nrm(1) = -ny / ds;
      nrm_e(0) = nx_e / ds_e;
      nrm_e(1) = ny_e / ds_e;;
      cout << "exact norm vector: " << endl;
      nrm_e.Print();
      cout << "norm vector using ls: " << endl;
      nrm.Print();
      return phi;
   }

   /// function that checks if an element is `cut` by `embedded geometry` or not
   bool cutByGeom(int &elemid) const
   {
      Element *el = mesh->GetElement(elemid);
      mfem::Array<int> v;
      el->GetVertices(v);
      int k, l, n;
      k = 0;
      l = 0;
      n = 0;
      for (int i = 0; i < v.Size(); ++i)
      {
         double *coord = mesh->GetVertex(v[i]);
         TinyVector<double, N> x;
         x(0) = coord[0];
         x(1) = coord[1];
         Vector lvsval(v.Size());
         lvsval(i) = -phi(x);
         // cout << "lsv is " << lsv << endl;
         if ((lvsval(i) < 0) && (abs(lvsval(i)) > 1e-16))
         {
            k = k + 1;
         }
         if ((lvsval(i) > 0))
         {
            l = l + 1;
         }
         if ((lvsval(i) == 0) || (abs(lvsval(i)) < 1e-16))
         {
            n = n + 1;
         }
      }
      if ((k == v.Size()) || (l == v.Size()))
      {
         return false;
      }
      if (((k == 3) || (l == 3)) && (n == 1))
      {
         return false;
      }
      else
      {
         return true;
      }
   }
   /// function that checks if an element is inside the `embedded geometry` or
   /// not
   bool insideBoundary(int &elemid) const
   {
      Element *el = mesh->GetElement(elemid);
      mfem::Array<int> v;
      el->GetVertices(v);
      int k;
      k = 0;
      for (int i = 0; i < v.Size(); ++i)
      {
         double *coord = mesh->GetVertex(v[i]);
         Vector lvsval(v.Size());
         TinyVector<double, N> x;
         x(0) = coord[0];
         x(1) = coord[1];
         lvsval(i) = -phi(x);
         if ((lvsval(i) < 0) || (lvsval(i) == 0))
         {
            k = k + 1;
         }
      }
      if (k == v.Size())
      {
         return true;
      }
      else
      {
         return false;
      }
   }

   /// function to get element center
   void GetElementCenter(int id, mfem::Vector &cent) const
   {
      cent.SetSize(mesh->Dimension());
      int geom = mesh->GetElement(id)->GetGeometryType();
      ElementTransformation *eltransf = mesh->GetElementTransformation(id);
      eltransf->Transform(Geometries.GetCenter(geom), cent);
   }

   /// find bounding box for a given cut element
   void findBoundingBox(int id,
                        blitz::TinyVector<double, N> &xmin,
                        blitz::TinyVector<double, N> &xmax) const
   {
      Element *el = mesh->GetElement(id);
      mfem::Array<int> v;
      Vector min, max;
      min.SetSize(N);
      max.SetSize(N);
      for (int d = 0; d < N; d++)
      {
         min(d) = infinity();
         max(d) = -infinity();
      }
      el->GetVertices(v);
      for (int iv = 0; iv < v.Size(); ++iv)
      {
         double *coord = mesh->GetVertex(v[iv]);
         for (int d = 0; d < N; d++)
         {
            if (coord[d] < min(d))
            {
               min(d) = coord[d];
            }
            if (coord[d] > max(d))
            {
               max(d) = coord[d];
            }
         }
      }
      xmin = {min[0], min[1]};
      xmax = {max[0], max[1]};
   }

   /// find worst cut element size
   void GetCutsize(vector<int> cutelems,
                   std::map<int, IntegrationRule *> &cutSquareIntRules,
                   double &cutsize) const
   {
      cutsize = 0.0;
   }
   /// get integration rule for cut elements
   void GetCutElementIntRule(
       vector<int> cutelems,
       int order,
       double radius,
       std::map<int, IntegrationRule *> &cutSquareIntRules) const
   {
      double tol = 1e-16;
      for (int k = 0; k < cutelems.size(); ++k)
      {
         IntegrationRule *ir;
         blitz::TinyVector<double, N> xmin;
         blitz::TinyVector<double, N> xmax;
         blitz::TinyVector<double, N> xupper;
         blitz::TinyVector<double, N> xlower;
         // standard reference element
         xlower = {0, 0};
         xupper = {1, 1};
         int dir = -1;
         int side = -1;
         int elemid = cutelems.at(k);
         findBoundingBox(elemid, xmin, xmax);
         phi.xscale = xmax[0] - xmin[0];
         phi.yscale = xmax[1] - xmin[1];
         phi.min_x = xmin[0];
         phi.min_y = xmin[1];;
         auto q =
             Algoim::quadGen<N>(phi,
                                Algoim::BoundingBox<double, N>(xlower, xupper),
                                dir,
                                side,
                                order);
         int i = 0;
         ir = new IntegrationRule(q.nodes.size());
         for (const auto &pt : q.nodes)
         {
            IntegrationPoint &ip = ir->IntPoint(i);
            ip.x = pt.x[0];
            ip.y = pt.x[1];
            ip.weight = pt.w;
            i = i + 1;
            MFEM_ASSERT(ip.weight > 0,
                        "integration point weight is negative in domain "
                        "integration from Saye's method");
            MFEM_ASSERT(
                (phi(pt.x) < tol),
                " phi = "
                    << phi(pt.x) << " : "
                    << " levelset function positive at the quadrature point "
                       "domain integration (Saye's method)");
         }
         cutSquareIntRules[elemid] = ir;
      }
   }

   /// get integration rule for cut segments
   void GetCutSegmentIntRule(
       vector<int> cutelems,
       vector<int> cutinteriorFaces,
       int order,
       double radius,
       std::map<int, IntegrationRule *> &cutSegmentIntRules,
       std::map<int, IntegrationRule *> &cutInteriorFaceIntRules) const
   {
      for (int k = 0; k < cutelems.size(); ++k)
      {
         IntegrationRule *ir;
         blitz::TinyVector<double, N> xmin;
         blitz::TinyVector<double, N> xmax;
         blitz::TinyVector<double, N> xupper;
         blitz::TinyVector<double, N> xlower;
         int side;
         int dir;
         double tol = 1e-16;
         // standard reference element
         xlower = {0, 0};
         xupper = {1, 1};
         int elemid = cutelems.at(k);
         findBoundingBox(elemid, xmin, xmax);
         // xlower = {xmin[0], xmin[1]};
         // xupper = {xmax[0], xmax[1]};
         phi.xscale = xmax[0] - xmin[0];
         phi.yscale = xmax[1] - xmin[1];
         phi.min_x = xmin[0];
         phi.min_y = xmin[1];
         // phi.radius = radius;
         dir = N;
         side = -1;
         auto q =
             Algoim::quadGen<N>(phi,
                                Algoim::BoundingBox<double, N>(xlower, xupper),
                                dir,
                                side,
                                order);
         int i = 0;
         ir = new IntegrationRule(q.nodes.size());
         for (const auto &pt : q.nodes)
         {
            IntegrationPoint &ip = ir->IntPoint(i);
            ip.x = pt.x[0];
            ip.y = pt.x[1];
            ip.weight = pt.w;
            i = i + 1;
            // cout << "elem " << elemid << " , " << ip.weight << endl;
            double xqp = (pt.x[0] * phi.xscale) + phi.min_x;
            double yqp = (pt.x[1] * phi.yscale) + phi.min_y;
            MFEM_ASSERT(
                ip.weight > 0,
                "integration point weight is negative in curved surface "
                "int rule from Saye's method");
         }
         cutSegmentIntRules[elemid] = ir;
         mfem::Array<int> orient;
         mfem::Array<int> fids;
         mesh->GetElementEdges(elemid, fids, orient);
         int fid;
         for (int c = 0; c < fids.Size(); ++c)
         {
            fid = fids[c];
            if (find(cutinteriorFaces.begin(), cutinteriorFaces.end(), fid) !=
                cutinteriorFaces.end())
            {
               if (cutInteriorFaceIntRules[fid] == NULL)
               {
                  mfem::Array<int> v;
                  mesh->GetEdgeVertices(fid, v);
                  double *v1coord, *v2coord;
                  v1coord = mesh->GetVertex(v[0]);
                  v2coord = mesh->GetVertex(v[1]);
                  if (v1coord[0] == v2coord[0])
                  {
                     dir = 0;
                     if (v1coord[0] < xmax[0])
                     {
                        side = 0;
                     }
                     else
                     {
                        side = 1;
                     }
                  }
                  else
                  {
                     dir = 1;
                     if (v1coord[1] < xmax[1])
                     {
                        side = 0;
                     }
                     else
                     {
                        side = 1;
                     }
                  }
                  auto q = Algoim::quadGen<N>(
                      phi,
                      Algoim::BoundingBox<double, N>(xlower, xupper),
                      dir,
                      side,
                      order);
                  int i = 0;
                  ir = new IntegrationRule(q.nodes.size());
                  for (const auto &pt : q.nodes)
                  {
                     IntegrationPoint &ip = ir->IntPoint(i);
                     ip.y = 0.0;
                     if (dir == 0)
                     {
                        if (-1 == orient[c])
                        {
                           ip.x = 1 - pt.x[1];
                        }
                        else
                        {
                           ip.x = pt.x[1];
                        }
                     }
                     else if (dir == 1)
                     {
                        if (-1 == orient[c])
                        {
                           ip.x = 1 - pt.x[0];
                        }
                        else
                        {
                           ip.x = pt.x[0];
                        }
                     }
                     ip.weight = pt.w;
                     i = i + 1;
                     // scaled to original element space
                     double xq = (pt.x[0] * phi.xscale) + phi.min_x;
                     double yq = (pt.x[1] * phi.yscale) + phi.min_y;
                     MFEM_ASSERT(ip.weight > 0,
                                 "integration point weight is negative from "
                                 "Saye's method");
                     MFEM_ASSERT(
                         (phi(pt.x) < tol),
                         " phi = " << phi(pt.x) << " : "
                                   << "levelset function positive at the "
                                      "quadrature point (Saye's method)");
                     MFEM_ASSERT(
                         (xq <= (max(v1coord[0], v2coord[0]))) &&
                             (xq >= (min(v1coord[0], v2coord[0]))),
                         "integration point (xcoord) not on element face "
                         "(Saye's rule)");
                     MFEM_ASSERT(
                         (yq <= (max(v1coord[1], v2coord[1]))) &&
                             (yq >= (min(v1coord[1], v2coord[1]))),
                         "integration point (ycoord) not on element face "
                         "(Saye's rule)");
                  }
                  cutInteriorFaceIntRules[fid] = ir;
               }
            }
         }
      }
   }

protected:
   mfem::Mesh *mesh;
   mutable circle<N> phi_e;
   mutable Algoim::LevelSet<2> phi;
};
}  // namespace mach

#endif