#include "mfem.hpp"
#include "galer_diff.hpp"
#include "rbfgridfunc.hpp"
#include "linear_optimization.hpp"
#include "bfgsnewton.hpp"
#include <random>
#include <fstream>
#include <iostream>
#include <iomanip>

using namespace std;
using namespace mfem;
using namespace mach;


std::default_random_engine gen(std::random_device{}());
std::uniform_real_distribution<double> normal_rand(0.0,1.0);

// problem field coefficient and boundary
void velocity_function(const Vector &x, Vector &v);
double inflow1_function(const Vector &x);

// 2 methods the build element basis centers
void buildBasisCenter(mfem::Mesh *mesh, mfem::Vector& centers);
void buildBasisCenter2(int nx, int ny, mfem::Vector& centers);

template<typename T>
void writeBasisCentervtp(const mfem::Vector &q, T& stream);

int main(int argc, char *argv[])
{
   const char *options_file = "linear_optimization_test_options.json";
   int myid = 0;
   // Parse command-line options
   OptionsParser args(argc, argv);
   int nx = 1;
   int ny = 1;
	 int method = 1;
	 int ref_levels = 0;
	 const char *mesh_file = "square_triangle.mesh";
   args.AddOption(&options_file, "-o", "--options",
                  "Options file to use.");
   args.AddOption(&nx, "-nx", "--num-rad", "number of radial segments");
   args.AddOption(&ny, "-ny", "--num-theta", "number of angular segments");
	 args.AddOption(&method, "-m", "--method", "method to build basis centers");
	 args.AddOption(&ref_levels, "-r", "--refine", "mesh refinement level.");
   args.Parse();
   if (!args.Good())
   {
      args.PrintUsage(cout);
      return 1;
   }
   try
   {
      // 1. Initialize option files
			std::string optfile(options_file);

			// 2. Initialize optimizer
			std::unique_ptr<Mesh> smesh(new Mesh(mesh_file, 1, 1));
  		for (int lev = 0; lev < ref_levels; lev++)
  		{
				smesh->UniformRefinement();
  		}
			ofstream savevtk("linear_optimization.vtk");
      smesh->PrintVTK(savevtk, 0);
      savevtk.close();

			// 3. Initilize disign vars
	    int numElement = smesh->GetNE();
			int dim = smesh->Dimension();
	    mfem::Vector center;
			if (method == 1)
			{
					buildBasisCenter(smesh.get(), center);
			}
			else
			{
					buildBasisCenter2(nx, ny, center);
			}
			ofstream centerwrite("distri_initial.vtp");
      writeBasisCentervtp(center, centerwrite);
      centerwrite.close();

			// 4. Initialize problem
			VectorFunctionCoefficient velocity(dim, velocity_function);
			FunctionCoefficient inflow1(inflow1_function);
      LinearOptimizer dgdopt(center, optfile, move(smesh));
			dgdopt.InitializeSolver(velocity, inflow1);


			// 5. BFGS
			BFGSNewtonSolver bfgsSolver(1.0,1e6,1e-4,0.7,40);
      bfgsSolver.SetOperator(dgdopt);
      Vector opti_value(center.Size());
      bfgsSolver.Mult(center,opti_value);


			// 6. write results
			ofstream optwrite("distri_optimal.vtp");
      writeBasisCentervtp(opti_value, optwrite);
      optwrite.close();
   }
   catch (MachException &exception)
   {
      exception.print_message();
   }
   catch (std::exception &exception)
   {
      cerr << exception.what() << endl;
   }
   return 0;
}


// build the basis center
void buildBasisCenter(mfem::Mesh *mesh, mfem::Vector& centers)
{
	int ne = mesh->GetNE();
	int dim = mesh->Dimension();
	centers.SetSize(dim * ne);
	mfem::Vector loc(dim);
	for (int i = 0; i < ne; ++i)
	{
		mesh->GetElementCenter(i, loc);
		centers(dim * i) = loc(0);
		centers(dim * i+1) = loc(1);
	}
}

void buildBasisCenter2(int nx, int ny, mfem::Vector& centers)
{
	int num_basis = nx * ny;
	centers.SetSize(num_basis * 2);

	double dx = 2./(nx+1);
	double dy = 2./(ny+1);

	double x_st = -1.0 + 0.5 * dx;
	double y_st = -1.0 + 0.5 * dy;

	for (int j = 0; j < ny; ++j)
	{
		double y = y_st + j * dy;
		for (int i = 0; i < nx; ++i)
		{
			double x = x_st + i * dx;
			int count = j * nx + i;
			centers(2*count) = x;
			centers(2*count+1) = y;
		}
	}
}


template <typename T>
void writeBasisCentervtp(const mfem::Vector &center, T &stream)
{
   int nb = center.Size()/2;
   stream << "<?xml version=\"1.0\"?>\n";
   stream << "<VTKFile type=\"PolyData\" version=\"0.1\" byte_order=\"LittleEndian\">\n";
   stream << "<PolyData>\n";
   stream << "<Piece NumberOfPoints=\"" << nb << "\" NumberOfVerts=\"" << nb << "\" NumberOfLines=\"0\" NumberOfStrips=\"0\" NumberOfPolys=\"0\">\n";
   stream << "<Points>\n";
   stream << "  <DataArray type=\"Float32\" Name=\"Points\" NumberOfComponents=\"3\" format=\"ascii\">";
   for (int i = 0; i < nb; i++)
   {
      stream << center(i*2) << ' ' << center(i*2+1) << ' ' << 0.0 << ' ';
   }
   stream << "</DataArray>\n";
   stream << "</Points>\n";
   stream << "<Verts>\n";
   stream << "  <DataArray type=\"Int32\" Name=\"connectivity\" format=\"ascii\">";
   for (size_t i = 0; i < nb; ++i)
      stream << i << ' ';
   stream << "</DataArray>\n";
   stream << "  <DataArray type=\"Int32\" Name=\"offsets\" format=\"ascii\">";
   for (size_t i = 1; i <= nb; ++i)
      stream << i << ' ';
   stream << "</DataArray>\n";
   stream << "</Verts>\n";
   stream << "<PointData Scalars=\"w\">\n";
   stream << "  <DataArray type=\"Float32\" Name=\"w\" NumberOfComponents=\"1\" format=\"ascii\">";
   for (int i = 0; i < nb; i++)
      stream << 1.0 << ' ';
   stream << "</DataArray>\n";
   stream << "</PointData>\n";
   stream << "</Piece>\n";
   stream << "</PolyData>\n";
   stream << "</VTKFile>\n";
}

void velocity_function(const Vector &x, Vector &v)
{
	v(0) = 3./ sqrt(10.0);
	v(1) = 1./ sqrt(10.0);
}

double inflow1_function(const Vector &x)
{
	// 1. at y = -1
	if (fabs(x(1)+1.0) < 1e-14)
	{
			return 1.0;
	}

	// 2. at x = -1, -1 <= y <= -0.6667
	if (fabs(x(0)+1.0) < 1e-14)
	{
		if (-1.0 <= x(1) && x(1) <= -0.33333333333)	
		{
			return 1.0;
		}	
	}
	return 0.0;
}