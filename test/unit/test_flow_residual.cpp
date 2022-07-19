#include <random>

#include "catch.hpp"
#include "mfem.hpp"
#include "adept.h"

#include "flow_residual.hpp"
#include "sbp_fe.hpp"
#include "euler_fluxes.hpp"
#include "mach_input.hpp"

using namespace std;
using namespace mfem;
using namespace mach;

auto options = R"(
{
   "flow-param": {
      "viscous": false,
      "entropy-state": false,
      "mu": -1.0,
      "Re": 100,
      "Pr": 0.72,
      "mach": 0.5,
      "aoa": 0.0,
      "roll-axis": 0,
      "pitch-axis": 1
   },
   "space-dis": {
      "degree": 1,
      "lps-coeff": 1.0,
      "iface-coeff": 0.0,
      "basis-type": "csbp",
      "flux-fun": "Euler"
   },
   "time-dis": {
      "steady": false
   },
   "bcs": {
       "far-field": [2,3,4],
       "slip-wall": [1]
   }
      
})"_json;

TEST_CASE("FlowResidual construction and evaluation", "[FlowResidual]")
{
   const int dim = 2; // templating is hard here because mesh constructors
   int num_state = dim + 2;
   adept::Stack diff_stack;

   // generate a 8 element mesh and build the finite-element space
   int num_edge = 2;
   Mesh smesh(Mesh::MakeCartesian2D(num_edge, num_edge, Element::TRIANGLE,
                                    true /* gen. edges */, 1.0, 1.0, true));
   ParMesh mesh(MPI_COMM_WORLD, smesh);
   int p = options["space-dis"]["degree"].get<int>();
   SBPCollection fec(p, dim);
   ParFiniteElementSpace fespace(&mesh, &fec, num_state, Ordering::byVDIM);
   std::map<std::string, FiniteElementState> fields;
   // construct the residual
   FlowResidual<dim,false> res(options, fespace, fields, diff_stack);
   int num_var = getSize(res);
   REQUIRE(num_var == 132);

   // evaluate the residual using a constant state
   Vector q(num_var);
   double mach = options["flow-param"]["mach"].get<double>();
   double aoa = options["flow-param"]["aoa"].get<double>();
   for (int i = 0; i < num_var/num_state; ++i)
   {
      getFreeStreamQ<double, dim>(mach, aoa, 0, 1, q.GetData()+num_state*i);
   }
   auto inputs = MachInputs({{"state", q}});
   Vector res_vec(num_var);
   evaluate(res, inputs, res_vec);

   // the res_vec should be zero, since we are differentiating a constant flux
   REQUIRE( res_vec.Norml2() == Approx(0.0).margin(1e-14) );

   // check the entropy calculation; grabs the first 4 vars from q.GetData() to
   // compute the entropy, and then scales by domain size (which is 1 unit sqrd)
   auto total_ent = entropy<double, 2, false>(q.GetData());
   REQUIRE( calcEntropy(res, inputs) == Approx(total_ent) );
}

TEST_CASE("FlowResidual calcEntropyChange", "[FlowResidual]")
{
   const int dim = 2; // templating is hard here because mesh constructors
   int num_state = dim + 2;
   adept::Stack diff_stack;

   // generate a 18 element periodic mesh
   int num_edge = 3;
   Mesh smesh(Mesh::MakeCartesian2D(num_edge, num_edge, Element::TRIANGLE,
                                    true /* gen. edges */, 1.0, 1.0, true));
   Vector vx({1.0, 0.0});
   Vector vy({0.0, 1.0});
   std::vector<Vector> translations{ vx, vy }; 
   auto v2v = smesh.CreatePeriodicVertexMapping(translations, 1e-12);
   Mesh periodic_smesh = Mesh::MakePeriodic(smesh, v2v);

   ParMesh mesh(MPI_COMM_WORLD, periodic_smesh);
   int p = options["space-dis"]["degree"].get<int>();
   SBPCollection fec(p, dim);
   ParFiniteElementSpace fespace(&mesh, &fec, num_state, Ordering::byVDIM);

   std::map<std::string, FiniteElementState> fields;

   // construct the residual with no dissipation and using IR flux
   options["space-dis"]["lps-coeff"] = 0.0;
   options["space-dis"]["flux-fun"] = "IR"; 
   FlowResidual<dim,false> res(options, fespace, fields, diff_stack);
   int num_var = getSize(res);

   // create a randomly perturbed conservative variable state
   static std::default_random_engine gen;
   static std::uniform_real_distribution<double> uniform_rand(0.9,1.1);
   Vector q(num_var);
   double mach = options["flow-param"]["mach"].get<double>();
   double aoa = options["flow-param"]["aoa"].get<double>();
   for (int i = 0; i < num_var/num_state; ++i)
   {
      getFreeStreamQ<double, dim>(mach, aoa, 0, 1, q.GetData()+num_state*i);
      for (int j = 0; j < num_state; ++j)
      {
         q(num_state*i + j) *= uniform_rand(gen);
      }
   }

   // evaluate the entropy change based on q; by setting dqdt to be the 
   // residual evaluated at q, we ensure the entropy change should be zero for 
   // the periodic domain and lps coeff = 0.0
   auto inputs = MachInputs({{"state", q}});
   Vector dqdt(num_var);
   evaluate(res, inputs, dqdt);
   inputs = MachInputs({
      {"state", q}, {"state_dot", dqdt}, {"time", 0.0}, {"dt", 0.0}
   });
   REQUIRE( calcEntropyChange(res, inputs) == Approx(0.0).margin(1e-14) );
}

TEST_CASE("FlowResidual construction and evaluation 3D - 1 tet element", "[FlowResidual]")
{
   const int dim = 3; // templating is hard here because mesh constructors
   int num_state = dim + 2;
   adept::Stack diff_stack;
   options["bcs"]["slip-wall"] = {0};
   // //options["bcs"]["slip-wall"] = {1,2,3,4};
   options["bcs"]["far-field"] = {1,2,3,4};
   // //options["bcs"]["far-filed"] = {0};

   // // generate a 8 element mesh and build the finite-element space
   // int num_edge = 10;
   // Mesh smesh(Mesh::MakeCartesian3D(num_edge, num_edge, num_edge, Element::TETRAHEDRON,
   //                                  1.0, 1.0, 1.0, true));

   int nv = 4, ne = 1, nb = 4, sdim = 3, attrib = 1;
   Mesh smesh(dim, nv, ne, nb, sdim);
   smesh.AddVertex(Vertex(0.,0.,0.)());
   smesh.AddVertex(Vertex(1.,0.,0.)());
   smesh.AddVertex(Vertex(0.,1.,0.)());
   smesh.AddVertex(Vertex(0.,0.,1.)());

   int idx[4] = {0,1,2,3};
   smesh.AddElement(new Tetrahedron(idx,attrib));
   smesh.AddBdrElement(new Triangle(0,1,2,attrib));
   smesh.AddBdrElement(new Triangle(0,1,3,attrib));
   smesh.AddBdrElement(new Triangle(0,2,3,attrib));
   smesh.AddBdrElement(new Triangle(1,2,3,attrib));
   smesh.FinalizeTetMesh(1,1,true); 
   
   ParMesh mesh(MPI_COMM_WORLD, smesh);
   

   for(int p = 0; p <= 1; ++p)
   {  
      DYNAMIC_SECTION("... for degree p = " << p)
      {
         SBPCollection fec(p, dim);
         ParFiniteElementSpace fespace(&mesh, &fec, num_state, Ordering::byVDIM);
         int ndofs = fespace.GetNDofs();
         std::map<std::string, FiniteElementState> fields;
         // construct the residual
         FlowResidual<dim,false> res(options, fespace, fields, diff_stack);
         int num_var = getSize(res);
         REQUIRE(num_var == num_state*ndofs);

         // evaluate the residual using a constant state
         Vector q(num_var);
         double mach = options["flow-param"]["mach"].get<double>();
         double aoa = options["flow-param"]["aoa"].get<double>();
         for (int i = 0; i < ndofs; ++i)
         {  
            getFreeStreamQ<double, dim>(mach, aoa, 0, 1, q.GetData()+num_state*i);
         }
         auto inputs = MachInputs({{"state", q}});
         //Vector res_vec(num_var);
         ParGridFunction res_vec(&fespace);
         evaluate(res, inputs, res_vec);

         ofstream residual("residual_values.vtk");
         residual.precision(14);
         mesh.PrintVTK(residual, 0);
         res_vec.SaveVTK(residual, "residual", 0);
         residual.close();

         // the res_vec should be zero, since we are differentiating a constant flux
         REQUIRE( res_vec.Norml2() == Approx(0.0).margin(1e-14) );

         // check the entropy calculation; grabs the first 4 vars from q.GetData() to
         // compute the entropy, and then scales by domain size (which is 1 unit sqrd) 
         // The total_ent should be divided by 6 (since this is only 1 tet element)
         auto total_ent = entropy<double, dim, false>(q.GetData())/6.0;
         REQUIRE( calcEntropy(res, inputs) == Approx(total_ent) );      
      }
   }
}


TEST_CASE("FlowResidual construction and evaluation 3D", "[FlowResidual]")
{
   const int dim = 3; // templating is hard here because mesh constructors
   int num_state = dim + 2;
   adept::Stack diff_stack;
   options["bcs"]["slip-wall"] = {6};
   //options["bcs"]["slip-wall"] = {1,2,3,4};
   options["bcs"]["far-field"] = {1,2,3,4,5};
   //options["bcs"]["far-filed"] = {0};

   // generate a 8 element mesh and build the finite-element space
   int num_edge = 1;
   Mesh smesh(Mesh::MakeCartesian3D(num_edge, num_edge, num_edge, Element::TETRAHEDRON,
                                    1.0, 1.0, 1.0, Ordering::byVDIM));
   ParMesh mesh(MPI_COMM_WORLD, smesh);
   
   for(int p = 0; p <= 1; ++p)
   {  
      DYNAMIC_SECTION("... for degree p = " << p)
      {
         SBPCollection fec(p, dim);
         ParFiniteElementSpace fespace(&mesh, &fec, num_state, Ordering::byVDIM);
         int ndofs = fespace.GetNDofs();
         std::map<std::string, FiniteElementState> fields;
         // construct the residual
         FlowResidual<dim,false> res(options, fespace, fields, diff_stack);
         int num_var = getSize(res);
         REQUIRE(num_var == num_state*ndofs);

         // evaluate the residual using a constant state
         Vector q(num_var);
         double mach = options["flow-param"]["mach"].get<double>();
         double aoa = options["flow-param"]["aoa"].get<double>();
         for (int i = 0; i < ndofs; ++i)
         {  
            getFreeStreamQ<double, dim>(mach, aoa, 1, 0, q.GetData()+num_state*i);
         }
         auto inputs = MachInputs({{"state", q}});
         //Vector res_vec(num_var);
         ParGridFunction res_vec(&fespace);
         evaluate(res, inputs, res_vec);

         ofstream residual("residual_values.vtk");
         residual.precision(14);
         mesh.PrintVTK(residual, 0);
         res_vec.SaveVTK(residual, "residual", 0);
         residual.close();

         // the res_vec should be zero, since we are differentiating a constant flux
         REQUIRE( res_vec.Norml2() == Approx(0.0).margin(1e-14) );

         // check the entropy calculation; grabs the first 4 vars from q.GetData() to
         // compute the entropy, and then scales by domain size (which is 1 unit sqrd)
         // auto total_ent = entropy<double, dim, false>(q.GetData());
         // REQUIRE( calcEntropy(res, inputs) == Approx(total_ent) );      
      }
   }
}