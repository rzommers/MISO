#include "default_options.hpp"

namespace miso
{
/// This is placed in its own file because options are likely to grow large.
/// Also, while it would have been nice to use a raw string here to define the
/// default options, this would not have permitted comments.

const nlohmann::json default_options{
    {"print-options", true},  // print out options when solver is constructed
    {"paraview",              // options related to paraview visualization
     {
         {"directory", "solver"},
         {"log", true},  // if false, disable all paraview logging
         {"fields", {"state"}},
         {"each-timestep", false}  // if true, paraview file is saved each step
     }},
    {"test-ode", false},  // if true, use a simple conservative controller
    {"flow-param",        // options related to flow simulations
     {
         {"entropy-state", false},  // if true, the states are entropy variables
         {"mach", 0.5},             // far-field mach number
         {"aoa", 0.0},              // far-field angle of attack
         {"roll-axis", 0},    // axis aligned with nose to tail of aircraft
         {"pitch-axis", 1},   // axis in the "vertical" direction
         {"viscous", false},  // if true, viscous terms are added
         {"Re", 0.0},         // far-field Reynolds number
         {"Pr", 0.72},        // the Prandtl number
         {"mu",
          -1.0},  // nondimensional viscosity (if negative, use Sutherland's)
         {"inviscid-mms", false}, // if true, include the inviscid MMS terms
         {"viscous-mms", false}  // if true, include MMS terms for viscous test
     }},

    {"space-dis",            // options related to spatial discretization
     {{"degree", 1},         // default operator degree
      {"lps-coeff", 1.0},    // scaling coefficient for local-proj stabilization
      {"iface-coeff", 1.0},  // scaling coefficient for interface dissipation
      {"basis-type", "csbp"},  // csbp & dsbp for continuous & discrete SBP
      {"flux-fun", "IR"}}},

    {"steady", false},  // deprecated; now included in "time-dis"
    {"time-dis",        // options related to unsteady time-marching
     {
         {"type", "RK4"},           // type of ODE solver to use
         {"steady", false},         // if true, solve a steady problem
         {"steady-abstol", 1e-12},  // absolute convergence tolerance for steady
         {"steady-reltol", 1e-10},  // relative convergence tolerance for steady
         {"res-exp",
          2.0},  // for steady problems, controls the step-size growth
         {"const-cfl", false},  // if true, adapt dt to keep cfl constant
         {"t-initial", 0.0},    // initial time at which to start the simulation
         {"t-final", 1.0},      // final time to simulate to
         {"dt", 0.01},          // time-step size when `const-cfl` is false
         {"cfl", 1.0},          // target CFL number
         {"max-iter", 10000},   // safe-guard upper bound number of iterations
         {"entropy-log", false}  // if true, time history of entropy is written
     }},

    {"nonlin-solver",  // options related to root-finding algorithms
     {
         {"type", "newton"},  // type of nonlinear solver
         {"printlevel", 1},   // linear solver print level (no printing if zero)
         {"maxiter", 100},    // default to 100 iterations
         {"reltol", 1e-14},   // solver relative tolerance
         {"abstol", 1e-14},   // solver absolute tolerance
         {"abort", true},     // should program abort if Newton doesn't converge
     }},

    {"lin-solver",
     {
         {"type", "hyprefgmres"},  // Default solver
         {"printlevel", 1},  // linear solver print level (no printing if zero)
         {"maxiter", 100},   // default to 100 iterations
         {"reltol", 1e-12},  // solver relative tolerance
         {"abstol", 1e-12},  // solver absolute tolerance
         {"kdim", 100}       // default restart value
     }},

    {"lin-prec",
     {
         {"type", "hypreilu"},  // default preconditioner
         {"lev-fill", 1},       // ILU(k) fill level
         {"ilu-type", 0},       // ILU type (see mfem HypreILU doc)
         {"ilu-reorder", 1},    // 0 = no reordering, 1 = RCM
         {"printlevel", 0}  // 0 = none, 1 = setup, 2 = solve, 3 = setup+solve
     }},

    {"adj-solver",
     {
         {"type", "hyprefgmres"},  // Default solver
         {"printlevel", 0},  // adjoint solver print level (no printing if zero)
         {"maxiter", 100},   // maximum number of solver iterations
         {"reltol", 1e-8},   // adjoint solver relative tolerance
         {"abstol", 1e-10},  // adjoint solver absolute tolerance
         {"kdim", 100}       // default restart value
     }},

    {"adj-prec",
     {
         {"type", "hypreilu"},  // default adjoint-solver preconditioner
         {"lev-fill", 1},       // ILU(k) fill level
         {"ilu-type", 10},      // ILU type (see mfem HypreILU doc)
         {"ilu-reorder", 1},    // 0 = no reordering, 1 = RCM
         {"printlevel", 0}  // 0 = none, 1 = setup, 2 = solve, 3 = setup+solve
     }},

    {"petscsolver",
     {{"ksptype", "gmres"},  // GMRES is the default solver for PETSC
      {"pctype", "lu"},      // TODO: LU or ILU?
      {"abstol", 1e-10},
      {"reltol", 1e-10},
      {"maxiter", 100},
      {"printlevel", 0}}},

    {"mesh",
     {
         {"file", "miso.mesh"},       // mesh file name
         {"model-file", "miso.dmg"},  // model file name
         {"refine", 0}  // recursive uniform refinement; 0 = no refinement
     }},
};

}  // namespace miso
