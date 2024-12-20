#include "mfem.hpp"
#include "thermal.hpp"

#include <fstream>
#include <iostream>

using namespace std;
using namespace mfem;
using namespace miso;

static double temp_0;

static double t_final;

static double InitialTemperature(const Vector &x);

static double ExactSolution(const Vector &x);

int main(int argc, char *argv[])
{
   // Initialize MPI
   MPI_Init(&argc, &argv);
   int rank;
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   ostream *out = getOutStream(rank);

   // Parse command-line options
   OptionsParser args(argc, argv);
   const char *options_file = "thermal_square_options.json";
   args.AddOption(&options_file, "-o", "--options",
                  "Options file to use.");
   args.Parse();
   if (!args.Good())
   {
      args.PrintUsage(cout);
      return 1;
   }

   string opt_file_name(options_file);
   nlohmann::json options;
   nlohmann::json file_options;
   ifstream opts(opt_file_name);
   opts >> file_options;
   options.merge_patch(file_options);

   temp_0 = options["init-temp"].get<double>();
   t_final = options["time-dis"]["t-final"].get<double>();

   // generate a simple tet mesh
   int num_edge_x = options["mesh"]["num-edge-x"].get<int>();
   int num_edge_y = options["mesh"]["num-edge-y"].get<int>();
   int num_edge_z = options["mesh"]["num-edge-z"].get<int>();

   int dim = 3;
   std::unique_ptr<Mesh> mesh(
      new Mesh(Mesh::MakeCartesian3D(num_edge_x, num_edge_y, num_edge_z,
                                     Element::TETRAHEDRON, 1.0, 1.0, 1.0,
                                     true)));
   mesh->EnsureNodes();

   std::cout << "Number of Boundary Attributes: "<< mesh->bdr_attributes.Size() <<std::endl;
   // assign attributes to top and bottom sides
   for (int i = 0; i < mesh->GetNE(); ++i)
   {
      Element *elem = mesh->GetElement(i);

      Array<int> verts;
      elem->GetVertices(verts);

      bool below = true;
      for (int i = 0; i < 4; ++i)
      {
         auto vtx = mesh->GetVertex(verts[i]);
         if (vtx[0] <= 0.5)
         {
            below = below & true;
         }
         else
         {
            below = below & false;
         }
      }
      if (below)
      {
         elem->SetAttribute(1);
      }
      else
      {
         elem->SetAttribute(2);
      }
   }

   ofstream mesh_ofs("test_cube.vtk");
   mesh_ofs.precision(8);
   mesh->SetAttributes(); //need to do this to set the attributes array
   mesh->PrintVTK(mesh_ofs);

   try
   {
      // Verify the total sensitivity of the functional to the mesh nodes

      
      // ThermalSolver solver(opt_file_name, move(mesh));
      // solver.initDerived();
      auto solver = createSolver<ThermalSolver>(opt_file_name, move(mesh));
      solver->setInitialCondition(InitialTemperature);
      // unique_ptr<MagnetostaticSolver<3>> solver(
      //    new MagnetostaticSolver<3>(opt_file_name, nullptr));
      std::cout << "Solving..." << std::endl;
      solver->solveForState();
      // std::cout << "Solving Adjoint..." << std::endl;
      // solver.solveForAdjoint(options["outputs"]["temp-agg"].get<std::string>());
      std::cout << "Solver Done" << std::endl;
      std::cout.precision(17);
      std::cout << "\n|| rho_h - rho ||_{L^2} = " 
                << solver->calcL2Error(ExactSolution) << '\n' << endl;
   }
   catch (MISOException &exception)
   {
      exception.print_message();
   }
   catch (std::exception &exception)
   {
      cerr << exception.what() << endl;
   }

   MPI_Finalize();
}

double InitialTemperature(const Vector &x)
{
   //return cos(M_PI*x(0));

   //return sin(M_PI*x(0)/2) - x(0)*x(0)/2;

   // if (x(0) <= .5)
   // {
   //    return sin(M_PI*x(0)/2) - x(0)*x(0)/2;
   // }
   // else
   // {
   //    return sin(M_PI*x(0)/2) + x(0)*x(0)/2 - 1.0/4;
   // }

   //For Use Testing Aggregated Constraint
   return temp_0;//sin(M_PI*x(0))*sin(M_PI*x(0));
}

double ExactSolution(const Vector &x)
{
   return cos(M_PI*x(0))*exp(-M_PI*M_PI*t_final);
   
   //return sin(M_PI*x(0)/2)*exp(-M_PI*M_PI*t_final/4) - x(0)*x(0)/2;

   // if (x(0) <= .5)
   // {
   //    return sin(M_PI*x(0)/2)*exp(-M_PI*M_PI*t_final/4) - x(0)*x(0)/2;
   // }
   // else
   // {
   //    return sin(M_PI*x(0)/2)*exp(-M_PI*M_PI*t_final/4) + x(0)*x(0)/2 - 1.0/4;
   // }
}
