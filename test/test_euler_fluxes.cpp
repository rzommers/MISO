#include "catch.hpp"
#include "mfem.hpp"
#include "euler.hpp"

//#include "euler_test_data.hpp"

TEST_CASE( "Log-average is correct", "[log-avg]")
{
   #include "euler_test_data.hpp"
   REQUIRE( mach::logavg(rho, rho) == Approx(rho) );
   REQUIRE( mach::logavg(rho, 2.0*rho) == Approx(1.422001977589051) );
}

TEMPLATE_TEST_CASE_SIG( "Euler flux functions, etc, produce correct values", "[euler]",
                        ((int dim), dim), 1, 2, 3 )
{
   #include "euler_test_data.hpp"
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

   SECTION( "Pressure function is correct" )
   {
      REQUIRE( mach::pressure<double,dim>(q.GetData()) == 
               Approx(press_check[dim-1]) );
   }

   SECTION( "Spectral radius of flux Jacobian is correct" )
   {
      REQUIRE( mach::calcSpectralRadius<double,dim>(nrm.GetData(), q.GetData()) ==
               Approx(spect_check[dim-1]) );
   }

   SECTION( "Euler flux is correct" )
   {
      // Load the data to test the Euler flux; the pointer arithmetic in the 
      // following constructor is to find the appropriate offset
      int offset = div((dim+1)*(dim+2),2).quot - 3;
      mfem::Vector flux_vec(flux_check + offset, dim + 2);
      mach::calcEulerFlux<double,dim>(nrm.GetData(), q.GetData(), flux.GetData());
      for (int i = 0; i < dim+2; ++i)
      {
         REQUIRE( flux(i) == Approx(flux_vec(i)) );
      }
   }

   SECTION(" Euler flux jacobian respect to state is correct")
   {
      //Create the perturbation vector
      mfem::Vector v_q(dim+2);
      for(int i=0; i<dim+2;i++)
      {
         v_q[i] = i*1e-6;
      }
      // Two ways to create the euler integrator
      adept::Stack stack;
      //mach::InviscidIntegrator<mach::EulerIntegrator<dim>> * eulerinteg = 
      // new mach::InviscidIntegrator<mach::EulerIntegrator<dim>> (stack);
      mach::EulerIntegrator<dim> eulerinteg(stack);

      // Create some intermediate variables
      mfem::Vector q_plus(q), q_minus(q);
      mfem::Vector flux1(dim+2), flux2(dim+2);
      mfem::Vector jac_v(dim+2);
      mfem::DenseMatrix flux_jac1(dim+2);

      eulerinteg.calcFluxJacState(nrm, q, flux_jac1);
      flux_jac1.Mult(v_q, jac_v);

      q_plus.Add(1.0, v_q);
      q_minus.Add(-1.0, v_q);
      eulerinteg.calcFlux(nrm, q_plus, flux1);
      eulerinteg.calcFlux(nrm, q_minus, flux2);

      mfem::Vector jac_v_fd(flux1);
      jac_v_fd -= flux2;
      jac_v_fd /= 2.0;
      mfem::Vector diff(jac_v);
      diff -= jac_v_fd;
      REQUIRE( diff.Norml2() == Approx(0.0).margin(abs_tol) ); 
   }

   SECTION(" Euler flux jacobian respect to direction is correct")
   {
      adept::Stack stack;
      mach::EulerIntegrator<dim> eulerinteg(stack);

      // Create the perturbation vector
      mfem::Vector v_q(dim);
      for(int i=0; i<dim;i++)
      {
         v_q[i] = i*1e-6;
      }

      // Create the intermediate variables
      mfem::Vector nrm_plus(nrm), nrm_minus(nrm);
      mfem::Vector flux1(dim+2), flux2(dim+2);
      mfem::Vector jac_v(dim+2);
      mfem::DenseMatrix flux_jac1(dim+2,dim);

      eulerinteg.calcFluxJacDir(nrm,q,flux_jac1);
      flux_jac1.Mult(v_q, jac_v);

      nrm_plus.Add(1.0,v_q);
      nrm_minus.Add(-1.0,v_q);
      eulerinteg.calcFlux(nrm_plus,q,flux1);
      eulerinteg.calcFlux(nrm_minus,q,flux2);
      mfem::Vector jac_v_fd(flux1);
      jac_v_fd -= flux2;
      jac_v_fd /= 2.0;

      REQUIRE( jac_v_fd.Norml2() == Approx(jac_v.Norml2())); 
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
         mach::calcIsmailRoeFlux<double,dim>(di, q.GetData(), qR.GetData(),
                                             flux.GetData());
         for (int i = 0; i < dim+2; ++i)
         {
            REQUIRE( flux(i) == Approx(fluxIR_check(i,di)) );
         }
      }
   }

   SECTION( "Entropy variables are correct" )
   {
      // Load the data to test the entropy variables; the pointer arithmetic in the 
      // following constructor is to find the appropriate offset
      int offset = div((dim+1)*(dim+2),2).quot - 3;
      mfem::Vector entvar_vec(entvar_check + offset, dim + 2);
      mach::calcEntropyVars<double,dim>(q.GetData(), qR.GetData());
      for (int i = 0; i < dim+2; ++i)
      {
         REQUIRE( qR(i) == Approx(entvar_vec(i)) );
      }
   }

   SECTION( "dQ/dW * vec product is correct" )
   {
      // Load the data to test the dQ/dW * vec product; the pointer arithmetic in the 
      // following constructor is to find the appropriate offset
      int offset = div((dim+1)*(dim+2),2).quot - 3;
      mfem::Vector dqdw_prod(dqdw_prod_check + offset, dim+2);
      mach::calcdQdWProduct<double,dim>(q.GetData(), qR.GetData(), flux.GetData());
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
      mach::calcBoundaryFlux<double,dim>(nrm.GetData(), q.GetData(),
                                         q.GetData(), work.GetData(),
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
      mach::projectStateOntoWall<double,dim>(q.GetData()+1, q.GetData(),
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
      // As above with projectStateOntoWall, the wall normal is set proportional to the momentum,
      // so the flux will be zero, except for the term flux[1:dim] = pressure*dir[0:dim-1]
      mfem::Vector x(dim);
      mach::calcSlipWallFlux<double,dim>(x.GetData(), q.GetData()+1,
                                         q.GetData(), flux.GetData());
      mach::projectStateOntoWall<double,dim>(q.GetData()+1, q.GetData(),
                                             work.GetData());
      double press = mach::pressure<double,dim>(work.GetData());
      REQUIRE( flux(0) == Approx(0.0).margin(abs_tol) );
      for (int i = 0; i < dim; ++i)
      {
         REQUIRE( flux(i+1) == Approx(press*q(i+1)) );
      }
      REQUIRE( flux(dim+1) == Approx(0.0).margin(abs_tol) );
   }

}

TEST_CASE( "calcBoundaryFlux is correct", "[bndry-flux]")
{
   #include "euler_test_data.hpp"
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
   mach::calcBoundaryFlux<double,2>(nrm.GetData(), qbnd.GetData(), q.GetData(),
                                    work.GetData(), flux.GetData());
   for (int i = 0; i < 4; ++i)
   {
      REQUIRE( flux(i) == Approx(flux_bnd_check[i]) );
   }
}

TEST_CASE( "calcIsentropicVortexFlux is correct", "[vortex-flux]")
{
   #include "euler_test_data.hpp"
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
   mach::calcIsentropicVortexState<double>(x.GetData(), q.GetData());
   mach::calcIsentropicVortexFlux<double>(x.GetData(), nrm.GetData(),
                                          q.GetData(), flux.GetData());
   mach::calcEulerFlux<double,2>(nrm.GetData(), q.GetData(), flux2.GetData());
   for (int i = 0; i < 4; ++i)
   {
      REQUIRE( flux(i) == Approx(flux2(i)) );
   }
}

