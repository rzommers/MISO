#include "catch.hpp"
#include "mfem.hpp"
#include "navier_stokes_fluxes.hpp"
#include "euler_test_data.hpp"
#include "euler_fluxes.hpp"
TEMPLATE_TEST_CASE_SIG("navierstokes flux functions, etc, produce correct values",
                       "[navierstokes]", ((int dim), dim), 1)
{
   using namespace euler_data;
   using namespace std;
   double gamma = 1.4;
   double gami = gamma - 1.0;
   // copy the data into mfem vectors for convenience
   mfem::Vector q(dim + 2);
   mfem::Vector tau_ij(dim);
   mfem::Vector cdel_wxj(dim + 2);
   mfem::Vector cdwij(dim + 2);
   // create the derivative vector
   mfem::Vector del_wxj(dim + 2);
   mfem::Vector del_qxj(dim + 2);
   mfem::DenseMatrix fv(dim + 2, dim);
   mfem::DenseMatrix del_w(dim + 2, dim);
   mfem::DenseMatrix del_vxj(dim + 2, dim);
   mfem::DenseMatrix fcdwij(dim + 2, dim);
   // mfem::DenseMatrix tau_ij(dim);
   double Re, mu, Pr;
   Re = 1;
   mu = 1;
   Pr = 1;
   // conservative variables
   q(0) = rho;
   q(dim + 1) = rhoe;
   
   for (int di = 0; di < dim; ++di)
   {
      q(di + 1) = rhou[di];
   }
   // spatial derivatives of entropy variables
   for (int j = 0; j < dim; ++j)
   {
      for (int i = 0; i < dim + 2; ++i)
      {
         del_w(i, j) = 0.02 + (i * 0.01);
         del_w(i, j) += j * 0.02;
      }
   }
   // entropy variables derivatives
   cout << "dwdxj" << endl;
   del_w.Print();
   // get spatial derivatives of conservative variables
   // and use them to get respective derivatives for primitive variables
   for (int j = 0; j < dim; ++j)
   {
      del_vxj(dim + 1, j) = 0;
      del_w.GetColumn(j, del_wxj);
      mach::calcdQdWProduct<double, dim>(q.GetData(), del_wxj.GetData(),
                                         del_qxj.GetData());
      del_vxj(0, j) = del_qxj(0);
      for (int i = 1; i < dim + 1; ++i)
      {
         del_vxj(i, j) = (del_qxj(i) - (q(i) * del_qxj(0) / q(0))) / q(0);
         del_vxj(dim + 1, j) += (q(i) * ((q(i) * del_qxj(0) / q(0)) - del_qxj(i))) / q(0) * q(0);
      }
      del_vxj(dim + 1, j) = (del_qxj(dim + 1) - (q(dim + 1) * del_qxj(0) / q(0))) / q(0);
      del_vxj(dim + 1, j) *= gami;
   }

   // get the fluxes
   for (int i = 0; i < dim; ++i)
   {
      for (int j = 0; j < dim; ++j)
      {
         tau_ij(j) = del_vxj(i + 1, j) + del_vxj(j + 1, i);
         if (i == j)
         {
            for (int k = 0; k < dim; ++k)
            {
               tau_ij(j) -= 2*del_vxj(k + 1, k)/3;
            }
         }
         tau_ij(j) /= Re;
         fv(0, j) = 0;
         fv(j + 1, i) = tau_ij(j);
        // cout << "stress" << tau_ij(j) <<endl;
      }
      for (int k = 0; k < dim; ++k)
      {
         fv(dim + 1, k) += tau_ij(k) * q(i + 1) / q(0);
      }
      fv(dim + 1, i) -= mu * gamma * del_vxj(dim + 1, i) / (Pr * gami);
   }
   // get flux using c_{hat} matrices
   for (int i = 0; i < dim; ++i)
   {
      for (int k = 0; k < dim + 2; ++k)
      {
         cdwij(k) = 0;
      }
      for (int j = 0; j < dim; ++j)
      {
         for (int di = 0; di < dim + 2; ++di)
         {
            cdel_wxj(di) = 0;
         }
         del_w.GetColumn(j, del_wxj);
         mach::applyCijMatrix<double, dim>(i, j, mu, Pr, q.GetData(), del_wxj.GetData(), cdel_wxj.GetData());
         for (int k = 0; k < dim + 2; ++k)
         {
            cdwij(k) += cdel_wxj(k);
         }
      }
      for (int s = 0; s < dim + 2; ++s)
      {
         fcdwij(s, i) = cdwij(s);
      }
   }
   cout << "computed fluxes" << endl;
   fcdwij.Print();
   std::cout << "Analytical fluxes" << endl;
   fv.Print();
}

// back up to calculate flux
// // get the stress tensor
// for (int i = 0; i < dim; ++i)
// {
//    for (int j = 0; j < dim; ++j)
//    {
//       tau_ij(i, j) = del_vxj(i + 1, j) + del_vxj(j + 1, i);
//       if (i == j)
//       {
//          for (int k = 0; k < dim; ++k)
//          {
//             tau_ij(i, j) -= del_vxj(k + 1, k);
//          }
//          tau_ij(i, j) *= 2 / 3;
//       }
//       tau_ij(i, j) /= Re;
//       fv(0, j) = 0;
//       fv(j + 1, i) = tau_ij(i , j);
//    }
//    for (int k = 0; k < dim; ++k)
//    {
//       fv(dim + 1, k) += tau_ij(i) * q(k + 1) / q(0);
//    }
// }

// for (int j = 0; j < dim; ++j)
// {
//    fv(dim + 1, j) = 0;
//    for (int k = 0; k < dim ; ++k)
//    {
//       fv(dim + 1, j) += tau_ij(k, j) * q(k+1) / q(0);
//    }
//    fv(dim + 1, j) -= mu * euler::gammma * del_vxj(dim + 1, j) / (Pr * euler::gami);
// }
