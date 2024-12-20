#include "catch.hpp"
#include "mfem.hpp"
#include "euler_fluxes.hpp"
#include "euler_test_data.hpp"

using namespace miso;

TEST_CASE( "Log-average is correct", "[log-avg]")
{
   using namespace euler_data;
   REQUIRE( miso::logavg(rho, rho) == Approx(rho) );
   REQUIRE( miso::logavg(rho, 2.0*rho) == Approx(1.422001977589051) );
}

TEMPLATE_TEST_CASE_SIG("Euler flux functions, etc, produce correct values",
                       "[euler]", ((int dim), dim), 1, 2, 3)
{
   using namespace euler_data;
   // copy the data into mfem vectors for convenience 
   mfem::Vector q(dim+2);
   mfem::Vector qR(dim+2);
   mfem::Vector flux(dim+2);
   mfem::Vector nrm(dim);
   mfem::Vector work(dim+2);
   q(0) = rho;
   q(dim+1) = rhoe;
   qR(0) = rho2;
   qR(dim+1) = rhoe2;
   for (int di = 0; di < dim; ++di)
   {
      q(di+1) = rhou[di];
      qR(di+1) = rhou2[di];
      nrm(di) = dir[di];
   }

   SECTION("Pressure function is correct")
   {
      REQUIRE(miso::pressure<double, dim>(q.GetData()) ==
              Approx(press_check[dim - 1]));
   }

   SECTION("Entropy function is correct")
   {
      REQUIRE(miso::entropy<double, dim>(q.GetData()) ==
              Approx(entropy_check[dim - 1]));
   }

   SECTION("Entropy function is correct when using entropy variables")
   {
      mfem::Vector w(dim + 2);
      miso::calcEntropyVars<double, dim, false>(q.GetData(), w.GetData());
      REQUIRE(miso::entropy<double, dim, true>(w.GetData()) ==
              Approx(entropy_check[dim - 1]));
   }

   SECTION( "Spectral radius of flux Jacobian is correct" )
   {
      REQUIRE(miso::calcSpectralRadius<double, dim>(nrm.GetData(), q.GetData()) == Approx(spect_check[dim - 1]));
   }

   SECTION( "Euler flux is correct" )
   {
      // Load the data to test the Euler flux; the pointer arithmetic in the 
      // following constructor is to find the appropriate offset
      int offset = div((dim+1)*(dim+2),2).quot - 3;
      mfem::Vector flux_vec(flux_check + offset, dim + 2);
      miso::calcEulerFlux<double,dim>(nrm.GetData(), q.GetData(), flux.GetData());
      for (int i = 0; i < dim+2; ++i)
      {
         REQUIRE( flux(i) == Approx(flux_vec(i)) );
      }
   }  
   
   SECTION( "Roe flux is consistent" )
   {
      int offset = div((dim+1)*(dim+2),2).quot - 3;
      mfem::Vector flux_vec(flux_check + offset, dim + 2);
      miso::calcRoeFaceFlux<double,dim>(nrm.GetData(), q.GetData(),
                                        q.GetData(), flux.GetData());
      for (int i = 0; i < dim+2; ++i)
      {
         REQUIRE( flux(i) == Approx(flux_vec(i)) );
      }
   }

   // load the data to test the IR flux function into an mfem DenseMatrix
   // TODO: I could not find an elegant way to do this
   mfem::DenseMatrix fluxIR_check;
   if (dim == 1)
   {
      fluxIR_check.Reset(fluxIR_1D_check, dim+2, dim);
   }
   else if (dim == 2)
   {
      fluxIR_check.Reset(fluxIR_2D_check, dim+2, dim);
   }
   else 
   {
      fluxIR_check.Reset(fluxIR_3D_check, dim+2, dim);
   }
   for (int di = 0; di < dim; ++di)
   {
      DYNAMIC_SECTION( "Ismail-Roe flux is correct in direction " << di )
      {
         miso::calcIsmailRoeFlux<double,dim>(di, q.GetData(), qR.GetData(),
                                             flux.GetData());
         for (int i = 0; i < dim+2; ++i)
         {
            REQUIRE( flux(i) == Approx(fluxIR_check(i,di)) );
         }
      }
      DYNAMIC_SECTION( "Ismail-Roe flux (based on entropy vars) is correct in direction " << di )
      {
         mfem::Vector wL(dim + 2), wR(dim + 2);
         miso::calcEntropyVars<double, dim, false>(q.GetData(), wL.GetData());
         miso::calcEntropyVars<double, dim, false>(qR.GetData(), wR.GetData());
         miso::calcIsmailRoeFluxUsingEntVars<double, dim>(di, wL.GetData(),
                                                          wR.GetData(),
                                                          flux.GetData());
         for (int i = 0; i < dim + 2; ++i)
         {
            REQUIRE(flux(i) == Approx(fluxIR_check(i, di)));
         }
      }
   }

   SECTION( "Ismail-Roe face flux is correct in given direction" )
   {
      // get flux from function
      miso::calcIsmailRoeFaceFlux<double, dim>(nrm.GetData(), q.GetData(),
                                               qR.GetData(), flux.GetData());
      for (int i = 0; i < dim+2; ++i)
      {
         // get true flux by scaling fluxIR_check data
         double fluxIR = 0.0;
         for (int di = 0; di < dim; ++di)
         {
            fluxIR += fluxIR_check(i,di)*nrm(di);
         }
         REQUIRE( flux(i) == Approx(fluxIR) );
      }
   }

   SECTION( "Ismail-Roe face flux with dissipation is correct in given direction" )
   {
      // get flux from function
      double diss_coeff = 1.0;
      miso::calcIsmailRoeFaceFluxWithDiss<double, dim>(
          nrm.GetData(), diss_coeff, q.GetData(), qR.GetData(), flux.GetData());
      // evaluate the dissipation externally (since we cannot use fluxIR_check
      // data here directly)
      mfem::Vector wL(dim+2), wR(dim+2);
      miso::calcEntropyVars<double, dim, false>(q.GetData(), wL.GetData());
      miso::calcEntropyVars<double, dim, false>(qR.GetData(), wR.GetData());
      mfem::Vector q_avg(dim+2);
      add(0.5, q, 0.5, qR, q_avg);
      mfem::Vector dqdw(dim+2);
      wL -= wR;
      miso::calcdQdWProduct<double, dim>(q_avg.GetData(), wL.GetData(),
                                         dqdw.GetData());
      double spect = miso::calcSpectralRadius<double, dim>(nrm.GetData(),
                                                           q_avg.GetData());

      for (int i = 0; i < dim+2; ++i)
      {
         // get true flux by scaling fluxIR_check data
         double fluxIR = 0.0;
         for (int di = 0; di < dim; ++di)
         {
            fluxIR += fluxIR_check(i,di)*nrm(di);
         }
         REQUIRE( flux(i) - spect*dqdw(i) == Approx(fluxIR) );
      }
   }

   SECTION( "Ismail-Roe face flux (based on entropy vars) is correct in given direction" )
   {
      // get flux from function
      mfem::Vector wL(dim+2), wR(dim+2);
      miso::calcEntropyVars<double, dim, false>(q.GetData(), wL.GetData());
      miso::calcEntropyVars<double, dim, false>(qR.GetData(), wR.GetData());
      miso::calcIsmailRoeFaceFluxUsingEntVars<double, dim>(
         nrm.GetData(), wL.GetData(), wR.GetData(), flux.GetData());
      for (int i = 0; i < dim+2; ++i)
      {
         // get true flux by scaling fluxIR_check data
         double fluxIR = 0.0;
         for (int di = 0; di < dim; ++di)
         {
            fluxIR += fluxIR_check(i,di)*nrm(di);
         }
         REQUIRE( flux(i) == Approx(fluxIR) );
      }
   }

   SECTION( "Ismail-Roe face flux with dissipation (based on entropy vars) is correct in given direction" )
   {
      // get flux from function
      mfem::Vector wL(dim+2), wR(dim+2);
      miso::calcEntropyVars<double, dim, false>(q.GetData(), wL.GetData());
      miso::calcEntropyVars<double, dim, false>(qR.GetData(), wR.GetData());
      double diss_coeff = 1.0;
      miso::calcIsmailRoeFaceFluxWithDissUsingEntVars<double, dim>(
          nrm.GetData(), diss_coeff, wL.GetData(), wR.GetData(), flux.GetData());

      // evaluate the dissipation externally (since we cannot use fluxIR_check
      // data here directly)
      mfem::Vector q_avg(dim+2);
      add(0.5, q, 0.5, qR, q_avg);
      mfem::Vector dqdw(dim+2);
      wL -= wR;
      miso::calcdQdWProduct<double, dim>(q_avg.GetData(), wL.GetData(),
                                         dqdw.GetData());
      double spect = miso::calcSpectralRadius<double, dim>(nrm.GetData(),
                                                           q_avg.GetData());

      for (int i = 0; i < dim+2; ++i)
      {
         // get true flux by scaling fluxIR_check data
         double fluxIR = 0.0;
         for (int di = 0; di < dim; ++di)
         {
            fluxIR += fluxIR_check(i,di)*nrm(di);
         }
         REQUIRE( flux(i) - spect*dqdw(i) == Approx(fluxIR) );
      }
   }

   SECTION( "Entropy variable conversion is correct" )
   {
      // Load the data to test the entropy variables; the pointer arithmetic in
      // the following constructor is to find the appropriate offset
      int offset = div((dim+1)*(dim+2),2).quot - 3;
      mfem::Vector entvar_vec(entvar_check + offset, dim + 2);
      miso::calcEntropyVars<double,dim,false>(q.GetData(), qR.GetData());
      for (int i = 0; i < dim+2; ++i)
      {
         REQUIRE( qR(i) == Approx(entvar_vec(i)) );
      }
   }

   SECTION( "Conservative variable conversion is correct" )
   {
      // Load the data to test the entropy variables; the pointer arithmetic in
      // the following constructor is to find the appropriate offset
      int offset = div((dim+1)*(dim+2),2).quot - 3;
      mfem::Vector w(entvar_check + offset, dim + 2);
      miso::calcConservativeVars<double,dim,true>(w.GetData(), qR.GetData());
      for (int i = 0; i < dim+2; ++i)
      {
         REQUIRE( qR(i) == Approx(q(i)) );
      }
   }

   SECTION( "dQ/dW * vec product is correct" )
   {
      // Load the data to test the dQ/dW * vec product; the pointer arithmetic
      // in the following constructor is to find the appropriate offset
      int offset = div((dim+1)*(dim+2),2).quot - 3;
      mfem::Vector dqdw_prod(dqdw_prod_check + offset, dim+2);
      miso::calcdQdWProduct<double, dim>(q.GetData(), qR.GetData(),
                                         flux.GetData());
      for (int i = 0; i < dim+2; ++i)
      {
         REQUIRE( flux(i) == Approx(dqdw_prod(i)) );
      }
   }

   SECTION( "Boundary flux is consistent" )
   {
      // Load the data to test the boundary flux; this only tests that the
      // boundary flux agrees with the Euler flux when given the same states.
      // The pointer arithmetic in the following constructor is to find the
      // appropriate offset
      int offset = div((dim+1)*(dim+2),2).quot - 3;
      mfem::Vector flux_vec(flux_check + offset, dim + 2);
      miso::calcBoundaryFlux<double,dim>(nrm.GetData(), q.GetData(),
                                         q.GetData(), work.GetData(),
                                         flux.GetData());
      for (int i = 0; i < dim+2; ++i)
      {
         REQUIRE( flux(i) == Approx(flux_vec(i)) );
      }
   }

   SECTION( "Far-field flux is consistent" )
   {
      // Load the data to test the far-field flux; this only tests that the
      // flux agrees with the Euler flux when given the same states.
      // The pointer arithmetic in the following constructor is to find the
      // appropriate offset
      int offset = div((dim+1)*(dim+2),2).quot - 3;
      mfem::Vector flux_vec(flux_check + offset, dim + 2);
      miso::calcFarFieldFlux<double, dim>(nrm.GetData(), q.GetData(),
                                          q.GetData(), work.GetData(),
                                          flux.GetData());
      for (int i = 0; i < dim+2; ++i)
      {
         REQUIRE( flux(i) == Approx(flux_vec(i)) );
      }
   }

   SECTION( "Far-field flux is consistent when states are entropy vars" )
   {
      // Load the data to test the far-field flux; this only tests that the
      // flux agrees with the Euler flux when given the same states.
      // The pointer arithmetic in the following constructor is to find the
      // appropriate offset
      int offset = div((dim+1)*(dim+2),2).quot - 3;
      mfem::Vector flux_vec(flux_check + offset, dim + 2);
      mfem::Vector w(dim+2);
      miso::calcEntropyVars<double, dim, false>(q.GetData(), w.GetData());
      miso::calcFarFieldFlux<double, dim, true>(nrm.GetData(), q.GetData(),
                                                w.GetData(), work.GetData(),
                                                flux.GetData());
      for (int i = 0; i < dim+2; ++i)
      {
         REQUIRE( flux(i) == Approx(flux_vec(i)) );
      }
   }

   SECTION( "projectStateOntoWall removes normal component" )
   {
      // In this test case, the wall normal is set proportional to the momentum,
      // so the momentum should come back as zero
      miso::projectStateOntoWall<double,dim>(q.GetData()+1, q.GetData(),
                                             flux.GetData());
      REQUIRE( flux(0) == Approx(q(0)) );
      for (int i = 0; i < dim; ++i)
      {
         REQUIRE( flux(i+1) == Approx(0.0).margin(abs_tol) );
      }
      REQUIRE( flux(dim+1) == Approx(q(dim+1)) );      
   }

   SECTION( "calcSlipWallFlux is correct" )
   {
      // As above with projectStateOntoWall, the wall normal is set
      // proportional to the momentum, so the flux will be zero, except for the
      // term flux[1:dim] = pressure*dir[0:dim-1]
      mfem::Vector x(dim);
      miso::calcSlipWallFlux<double,dim>(x.GetData(), q.GetData()+1,
                                         q.GetData(), flux.GetData());
      //miso::projectStateOntoWall<double,dim>(q.GetData()+1, q.GetData(),
      //                                       work.GetData());
      //double press = miso::pressure<double,dim>(work.GetData());
      double press = miso::pressure<double,dim>(q.GetData());
      REQUIRE( flux(0) == Approx(0.0).margin(abs_tol) );
      for (int i = 0; i < dim; ++i)
      {
         REQUIRE( flux(i+1) == Approx(press*q(i+1)) );
      }
      REQUIRE( flux(dim+1) == Approx(0.0).margin(abs_tol) );
   }

   SECTION( "calcSlipWallFlux is correct when using entropy variables" )
   {
      // As above with projectStateOntoWall, the wall normal is set
      // proportional to the momentum, so the flux will be zero, except for the
      // term flux[1:dim] = pressure*dir[0:dim-1]
      mfem::Vector x(dim);
      mfem::Vector w(dim + 2);
      miso::calcEntropyVars<double, dim, false>(q.GetData(), w.GetData());
      miso::calcSlipWallFlux<double, dim, true>(x.GetData(), q.GetData() + 1,
                                                w.GetData(), flux.GetData());
      double press = miso::pressure<double,dim>(q.GetData());
      REQUIRE( flux(0) == Approx(0.0).margin(abs_tol) );
      for (int i = 0; i < dim; ++i)
      {
         REQUIRE( flux(i+1) == Approx(press*q(i+1)) );
      }
      REQUIRE( flux(dim+1) == Approx(0.0).margin(abs_tol) );
   }

   SECTION("calcControlFlux is correct")
   {
      mfem::Vector flux2(dim + 2);
      double control = -miso::dot<double, dim>(nrm.GetData(), q.GetData() + 1);
      double dA = sqrt(miso::dot<double, dim>(nrm.GetData(), nrm.GetData()));
      control /= dA * q(0);
      miso::calcControlFlux<double, dim>(
          nrm.GetData(), q.GetData(), control, flux.GetData());
      miso::calcEulerFlux<double, dim>(
          nrm.GetData(), q.GetData(), flux2.GetData());
      for (int i = 0; i < dim + 2; ++i)
      {
         REQUIRE(flux(i) == Approx(flux2(i)));
      }
      control = 1.352;
      miso::calcControlFlux<double, dim>(
          nrm.GetData(), q.GetData(), control, flux.GetData());
      mfem::Vector w(dim + 2);
      miso::calcEntropyVars<double, dim, false>(q.GetData(), w.GetData());
      double dS = miso::dot<double, dim + 2>(flux.GetData(), w.GetData());
      double rhou = miso::dot<double, dim>(nrm.GetData(), q.GetData() + 1);
      double S = miso::entropy<double, dim>(q.GetData());
      REQUIRE(dS == Approx(-S * control * dA + rhou));
   }
}

TEST_CASE( "calcBoundaryFlux is correct", "[bndry-flux]")
{
   using namespace euler_data;
   // copy the data into mfem vectors
   mfem::Vector q(4);
   mfem::Vector flux(4);
   mfem::Vector qbnd(4);
   mfem::Vector work(4);
   mfem::Vector nrm(2);
   q(0) = rho;
   q(3) = rhoe;
   qbnd(0) = rho2;
   qbnd(3) = rhoe2;
   for (int di = 0; di < 2; ++di)
   {
      q(di+1) = rhou[di];
      qbnd(di+1) = rhou2[di];
      nrm(di) = dir[di];
   }
   miso::calcBoundaryFlux<double,2>(nrm.GetData(), qbnd.GetData(), q.GetData(),
                                    work.GetData(), flux.GetData());
   for (int i = 0; i < 4; ++i)
   {
      REQUIRE( flux(i) == Approx(flux_bnd_check[i]) );
   }
}

TEST_CASE( "calcBoundaryFluxEC is correct", "[bndry-flux]")
{
   using namespace euler_data;
   // copy the data into mfem vectors
   mfem::Vector q(4);
   mfem::Vector flux(4);
   mfem::Vector qbnd(4);
   mfem::Vector nrm(2);
   mfem::Vector w(4);
   q(0) = rho;
   q(3) = rhoe;
   qbnd(0) = rho2;
   qbnd(3) = rhoe2;
   for (int di = 0; di < 2; ++di)
   {
      q(di+1) = rhou[di];
      qbnd(di+1) = rhou2[di];
      nrm(di) = dir[di];
   }
   double psi = miso::dot<double,2>(rhou, dir);
   double Un = miso::dot<double,2>(rhou2, dir)/rho2;
   double entflux = Un*miso::entropy<double,2>(qbnd.GetData());
   miso::calcBoundaryFluxEC<double,2>(nrm.GetData(), qbnd.GetData(),
                                      q.GetData(), entflux, 
                                      flux.GetData());
   miso::calcEntropyVars<double,2,false>(q.GetData(), w.GetData());
   REQUIRE( miso::dot<double,4>(w.GetData(), flux.GetData())
            == Approx(entflux + psi) );
}

TEST_CASE( "calcIsentropicVortexFlux is correct", "[vortex-flux]")
{
   using namespace euler_data;
   // copy the data into mfem vectors for convenience 
   mfem::Vector q(4);
   mfem::Vector flux(4);
   mfem::Vector flux2(4);
   mfem::Vector x(2);
   mfem::Vector nrm(2);
   // set location where we want to evaluate the flux
   x(0) = cos(M_PI*0.25);
   x(1) = sin(M_PI*0.25);
   for (int di = 0; di < 2; ++di)
   {
      nrm(di) = dir[di];
   }
   miso::calcIsentropicVortexState<double>(x.GetData(), q.GetData());
   miso::calcIsentropicVortexFlux<double>(x.GetData(), nrm.GetData(),
                                          q.GetData(), flux.GetData());
   miso::calcEulerFlux<double,2>(nrm.GetData(), q.GetData(), flux2.GetData());
   for (int i = 0; i < 4; ++i)
   {
      REQUIRE( flux(i) == Approx(flux2(i)) );
   }
}
