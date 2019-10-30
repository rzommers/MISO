#include "magnetostatic.hpp"
#include "coefficient.hpp"
#include "electromag_integ.hpp"

using namespace std;
using namespace mfem;

namespace mach
{

MagnetostaticSolver::MagnetostaticSolver(
	 const std::string &opt_file_name,
    std::unique_ptr<mfem::Mesh> smesh,
	 int dim)
	: AbstractSolver(opt_file_name, move(smesh))
{
	int fe_order = options["space-dis"]["degree"].get<int>();

	/// Create the H(Curl) finite element collection
   h_curl_coll.reset(new ND_FECollection(fe_order, dim));
	/// Create the H(Div) finite element collection
   h_div_coll.reset(new RT_FECollection(fe_order, dim));

	/// Create the H(Curl) finite element space
	h_curl_space.reset(new SpaceType(mesh.get(), h_curl_coll.get()));
	/// Create the H(Div) finite element space
	h_div_space.reset(new SpaceType(mesh.get(), h_div_coll.get()));

	/// Create MVP grid function
	A.reset(new GridFunType(h_curl_space.get()));
	/// Create magnetic flux grid function
	B.reset(new GridFunType(h_div_space.get()));

#ifdef MFEM_USE_MPI
   cout << "Number of finite element unknowns: "
        << h_curl_space->GlobalTrueVSize() << endl;
#else
   cout << "Number of finite element unknowns: "
        << h_curl_space->GetTrueVSize() << endl;
#endif

	/// Construct current source coefficient
	constructCurrent();

	/// Assemble current source vector
	assembleCurrentSource();

	/// set up the spatial semi-linear form
   double alpha = 1.0;
   res.reset(new NonlinearFormType(h_curl_space.get()));

	/// Construct reluctivity coefficient
	constructReluctivity();

	/// add curl curl integrator to residual
	res->AddDomainIntegrator(new CurlCurlNLFIntegrator(nu.get()));

	/// Construct magnetization coefficient
	constructMagnetization();

	/// add magnetization integrator to residual
	res->AddDomainIntegrator(new MagnetizationIntegrator(nu.get(), mag_coeff.get()));

	/// Costruct linear system solver
#ifdef MFEM_USE_MPI
   prec.reset(new HypreBoomerAMG());
   prec->SetPrintLevel(0);

   solver.reset(new HyprePCG());
   solver->SetTol(1e-14);
   solver->SetMaxIter(200);
   solver->SetPrintLevel(0);
   solver->SetPreconditioner(prec);
#else
	/// TODO look at example 3 or other serial EM examples to see what
	/// preconditioner they use, this one is probably not the best
	prec.reset(new GSSmoother());

	solver.reset(new CGSolver());
   solver->SetPrintLevel(0);
   solver->SetMaxIter(400);
   solver->SetRelTol(1e-14);
   solver->SetAbsTol(1e-14);
   solver->SetPreconditioner(*prec);
#endif

	/// TODO - have this use options
	/// Set up Newton solver
	newton_solver.iterative_mode = false;
   newton_solver.SetSolver(*solver);
   newton_solver.SetOperator(*res);
   newton_solver.SetPrintLevel(1); // print Newton iterations
   newton_solver.SetRelTol(1e-10);
   newton_solver.SetAbsTol(0.0);
   newton_solver.SetMaxIter(10);
}

void MagnetostaticSolver::solveSteady()
{
	/// I think this is all I need?
	newton_solver.Mult(*current_vec, *A);
	MFEM_VERIFY(newton_solver.GetConverged(), "Newton solver did not converge.");
}

void MagnetostaticSolver::constructCurrent()
{
	current_coeff.reset(new VectorMeshDependentCoefficient());

	std::unique_ptr<mfem::VectorCoefficient> winding_coeff(
		new VectorFunctionCoefficient(num_dim, winding_current_source));

	/// TODO - use options to select material attribute for windings
	/// picked 1 arbitrarily for now
	current_coeff->addCoefficient(1, move(winding_coeff));
}

void MagnetostaticSolver::assembleCurrentSource()
{
	int fe_order = options["space-dis"]["degree"].get<int>();

	/// Create the H1 finite element collection and space, only used by the
	/// divergence free projectors so we define them here and then throw them
	/// away
   auto h1_coll = H1_FECollection(fe_order, num_dim);
	auto h1_space = SpaceType(mesh.get(), &h1_coll);

	/// get int rule (approach followed my MFEM Tesla Miniapp)
	int irOrder = h_curl_space->GetElementTransformation(0)->OrderW()
                 + 2 * fe_order;
   int geom = h_curl_space->GetFE(0)->GetGeomType();
   const IntegrationRule *ir = &IntRules.Get(geom, irOrder);

	/// compute the divergence free current source
	auto *grad = new DiscreteGradOperator(&h1_space, h_curl_space.get());

	// assemble gradient form
	grad->Assemble();
   grad->Finalize();

	auto div_free_proj = DivergenceFreeProjector(h1_space, *h_curl_space,
                                              irOrder, NULL, NULL, grad);

	GridFunType j = GridFunType(h_curl_space.get());
	j.ProjectCoefficient(*current_coeff);

	GridFunType j_div_free = GridFunType(h_curl_space.get());
	// Compute the discretely divergence-free portion of j
	div_free_proj.Mult(j, j_div_free);

	/// create current linear form vector by multiplying mass matrix by
	/// divergene free current source grid function
	BilinearFormIntegrator h_curl_mass_integ = VectorFEMassIntegrator();
	h_curl_mass_integ.SetIntRule(ir);
	BilinearFormType *h_curl_mass = new BilinearFormType(h_curl_space.get());
	h_curl_mass->AddDomainIntegrator(&h_curl_mass_integ);

	// assemble mass matrix
	h_curl_mass->Assemble();
   h_curl_mass->Finalize();

	// Compute the dual of j
	h_curl_mass->AddMult(j_div_free, *current_vec);

	// I had strange errors when not using pointer versions of these
	delete h_curl_mass;
	delete grad;
}

} // namespace mach