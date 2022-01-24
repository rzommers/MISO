#include "mfem.hpp"
#include "nlohmann/json.hpp"

#include "mach.hpp"

using namespace std;
using namespace mfem;
using namespace mach;

void uinit(const Vector &x, Vector& u0);

int main(int argc, char *argv[])
{
   // Get the options
   const char *options_file = "forced_box_options.json";
   nlohmann::json options;
   ifstream option_source(options_file);
   option_source >> options;
   // Initialize MPI
   int num_procs, rank;
   MPI_Init(&argc, &argv);
   MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   ostream *out = getOutStream(rank);

   // Parse command-line options
   OptionsParser args(argc, argv);
   args.AddOption(&options_file, "-o", "--options",
                  "Options file to use.");
   args.Parse();
   if (!args.Good())
   {
      args.PrintUsage(cout);
      return 1;
   }

   try
   {
      // construct the solver, set the initial condition, and solve
      string opt_file_name(options_file);

      // Build mesh over the square domain
      const int num = 5;
      auto smesh = std::make_unique<Mesh>(Mesh::MakeCartesian2D(
         num, num, Element::TRIANGLE, true /* gen. edges */, 1.0, 1.0, true));

      // Create solver and set initial guess to constant
      FlowSolver<2> solver(MPI_COMM_WORLD, options, std::move(smesh));
      mfem::Vector state_tv(solver.getStateSize());
      solver.setState(uinit, state_tv);

      // get the initial entropy 
      solver.createOutput("entropy", options["outputs"].at("entropy"));
      MachInputs inputs({{"state", state_tv}});
      double entropy0 = solver.calcOutput("entropy", inputs);
      cout << "initial entropy = " << entropy0 << endl;

      // Solve for the state; inputs are not used at present...
      solver.solveForState(inputs, state_tv);

      // get the final entropy 
      double entropy = solver.calcOutput("entropy", inputs);
      cout << "final entropy = " << entropy << endl;
   }
   catch (MachException &exception)
   {
      exception.print_message();
   }
   catch (std::exception &exception)
   {
      cerr << exception.what() << endl;
   }
   MPI_Finalize();
}

void uinit(const Vector &x, Vector& u0)
{
   u0.SetSize(4);
   u0(0) = 1.0;
   u0(1) = 0.0;
   u0(2) = 0.0;
   double press = pow(u0(0), euler::gamma);
   u0(3) = press/euler::gami;
}
