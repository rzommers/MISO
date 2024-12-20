#include "catch.hpp"
#include "mfem.hpp"
#include "nlohmann/json.hpp"

#include "magnetostatic.hpp"

using namespace miso;

// Provide the options explicitly for regression tests
auto em_options = R"(
{
   "mesh": {
      "file": "data/torque_ring.smb",
      "model-file": "data/torque_ring.egads",
      "refine": 0
   },
   "space-dis": {
      "basis-type": "nedelec",
      "degree": 1
   },
   "time-dis": {
      "steady": true,
      "steady-abstol": 1e-9,
      "steady-reltol": 1e-6,
      "ode-solver": "PTC",
      "t-final": 100,
      "dt": 1e12,
      "max-iter": 10
   },
   "lin-solver": {
      "type": "gmres",
      "printlevel": 0,
      "maxiter": 200,
      "kdim": 200,
      "abstol": 1e-14,
      "reltol": 1e-14
   },
   "lin-prec": {
      "type": "hypreams",
      "printlevel": 0
   },
   "nonlin-solver": {
      "type": "newton",
      "printlevel": 3,
      "maxiter": 1,
      "reltol": 1e-10,
      "abstol": 1e-12
   },
   "components": {
      "ring": {
         "material": "copperwire",
         "attr": 3,
         "linear": true
      }
   },
   "bcs": {
      "essential": [1, 3]
   },
   "current": {
      "test": {
         "ring": [3]
      }
   }
})"_json;

/// exact force is 0.078 N
TEST_CASE("Torque Loop Regression Test")
{
   MagnetostaticSolver em_solver(MPI_COMM_WORLD, em_options, nullptr);
   mfem::Vector em_state(em_solver.getStateSize());
   em_state = 0.0; // initialize zero field

   em_solver.setState([](const mfem::Vector &x, mfem::Vector &A)
   {
      A(0) = 0.5*x(2);
      A(1) = 0.0;
      A(2) = -0.5*x(0);
   }, em_state);

   // The volume of wire in the mesh is 1.24266e-07, but should be 1.97392e-7.
   // This results in a lower effective current running through the wire, so 
   // we increase current density to get to 10 A.
   // The volume is lower because of bad mesh resolution
   constexpr auto current_density = M_1_PI * 1e7 * 1.5884635222882089;

   MISOInputs inputs {
      {"current_density:test", current_density},
      {"state", em_state}
   };
   em_solver.solveForState(inputs, em_state);

   nlohmann::json torque_options = {
      {"attributes", {3}},
      {"axis", {1.0, 0.0, 0.0}},
      {"about", {0.0, 0.0, 0.0}}
   };
   em_solver.createOutput("torque", torque_options);
   double torque = em_solver.calcOutput("torque", inputs);

   /// exact solution is -pi * 1e-3
   REQUIRE(torque == Approx(-0.0031583613).margin(1e-10));

}
