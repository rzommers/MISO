#include "joule.hpp"

#include "magnetostatic.hpp"
#include "thermal.hpp"

using namespace std;
using namespace mfem;

namespace mach
{

/// TODO: read options file and construct options files for EM and 
JouleSolver::JouleSolver(
	const std::string &opt_file_name,
   std::unique_ptr<mfem::Mesh> smesh)
	// : AbstractSolver(opt_file_name, move(smesh))
   : AbstractSolver()
{
   nlohmann::json em_opts = options["em-opts"];
   nlohmann::json thermal_opts = options["thermal-opts"];

   auto mesh_file = options["mesh"]["file"].get<std::string>();
   auto model_file = options["mesh"]["file"].get<std::string>();
   auto mesh_out_file = options["mesh"]["out-file"].get<std::string>();

   /// get mesh file name and extension 
   std::string mesh_name;
   std::string mesh_ext;
   {
      size_t i = mesh_file.rfind('.', mesh_file.length());
      if (i != string::npos) {
         mesh_name = (mesh_file.substr(0, i));
         mesh_ext = (mesh_file.substr(i+1, mesh_file.length() - i));
      }
      else
      {
         throw MachException("JouleSolver::JouleSolver()\n"
                           "\tMesh file has no extension!\n");
      }
   }

   em_opts["mesh"]["file"] = mesh_name + "_em" + mesh_ext;
   thermal_opts["mesh"]["file"] = mesh_name + "_thermal" + mesh_ext;

   /// get model file name and extension 
   std::string model_name;
   std::string model_ext;
   {
      size_t i = model_file.rfind('.', model_file.length());
      if (i != string::npos) {
         model_name = (model_file.substr(0, i));
         model_ext = (model_file.substr(i+1, model_file.length() - i));
      }
      else
      {
         throw MachException("JouleSolver::JouleSolver()\n"
                           "\tModel file has no extension!\n");
      }
   }

   em_opts["mesh"]["model-file"] = model_name + "_em" + model_ext;
   thermal_opts["mesh"]["model-file"] = model_name + "_thermal" + model_ext;

   em_opts["mesh"]["out-file"] = mesh_out_file + "_em";
   thermal_opts["mesh"]["out-file"] = mesh_out_file + "_thermal";

   em_opts["components"] = options["components"];
   thermal_opts["components"] = options["components"];

   em_opts["problem-opts"] = options["problem-opts"];
   thermal_opts["problem-opts"] = options["problem-opts"];


   std::string em_opt_file = "1";
   std::string thermal_opt_file = "1";

   em_solver.reset(new MagnetostaticSolver(em_opt_file, nullptr));
   /// TODO: this should be moved to an init derived when a factory is made
   em_solver->initDerived();
   em_fields = em_solver->getFields();
   int dim = em_solver->getMesh()->Dimension();

   int order = em_opts["space-dis"]["order"].get<int>();
	/// Create the H(Div) finite element collection for the representation the
   /// magnetic flux density field in the thermal solver
   h_div_coll.reset(new RT_FECollection(order, dim));
	/// Create the H(Div) finite element space
	h_div_space.reset(new SpaceType(mesh.get(), h_div_coll.get()));
   /// Create magnetic flux grid function
	mapped_mag_field.reset(new GridFunType(h_div_space.get()));

   thermal_solver.reset(new ThermalSolver(thermal_opts, nullptr,
                                          mapped_mag_field.get()));
   // thermal_solver->initDerived();
}

/// TODO: Change this in AbstractSolver to mark a flag so that unsteady solutions can be saved
void JouleSolver::printSolution(const std::string &filename, int refine)
{
   std::string em_filename = filename + "_em";
   std::string thermal_filename = filename + "_thermal";
   em_solver->printSolution(thermal_filename, refine);
   thermal_solver->printSolution(thermal_filename, refine);
}

std::vector<GridFunType*> JouleSolver::getFields(void)
{
	return {thermal_fields[0], em_fields[0], em_fields[1]};
}

void JouleSolver::solveForState()
{
   em_solver->solveForState();

   transferSolution(*em_solver->getMesh(), *thermal_solver->getMesh(),
                    *em_fields[1], *mapped_mag_field);
   thermal_solver->initDerived();
   thermal_fields = thermal_solver->getFields();
   thermal_solver->solveForState();
}

JouleSolver::~JouleSolver() = default;

} // namespace mach
