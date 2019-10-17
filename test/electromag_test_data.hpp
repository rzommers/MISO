#ifndef ELECTROMAG_TEST_DATA
#define ELECTROMAG_TEST_DATA

#include <limits>
#include <random>
#include "mfem.hpp"

#include "coefficient.hpp"

namespace electromag_data
{
// define the random-number generator; uniform between -1 and 1
static std::default_random_engine gen;
static std::uniform_real_distribution<double> uniform_rand(-1.0,1.0);

void randBaselinePert(const mfem::Vector &x, mfem::Vector &u)
{
   const double scale = 0.5;
   for (int i = 0; i < u.Size(); ++i)
   {
      u(i) = (2.0 + scale*uniform_rand(gen));
   }
}

void randState(const mfem::Vector &x, mfem::Vector &u)
{
	// std::cout << "u size: " << u.Size() << std::endl;
   for (int i = 0; i < u.Size(); ++i)
   {
		// std::cout << i << std::endl;
      u(i) = uniform_rand(gen);
   }
}

/// Simple linear coefficient for testing CurlCurlNLFIntegrator
class LinearCoefficient : public mach::StateCoefficient
{
public:
	LinearCoefficient(double val = 1.0) : value(val) {}

	double Eval(mfem::ElementTransformation &trans,
					const mfem::IntegrationPoint &ip,
					const double state) override
	{
		return value;
	}

	double EvalStateDeriv(mfem::ElementTransformation &trans,
								 const mfem::IntegrationPoint &ip,
								 const double state) override
	{
		return 0.0;
	}

private:
	double value;
};

/// Simple nonlinear coefficient for testing CurlCurlNLFIntegrator
class NonLinearCoefficient : public mach::StateCoefficient
{
public:
	NonLinearCoefficient() {};

	double Eval(mfem::ElementTransformation &trans,
					const mfem::IntegrationPoint &ip,
					const double state) override
	{
		// mfem::Vector state;
		// stateGF->GetVectorValue(trans.ElementNo, ip, state);
		// double state_mag = state.Norml2();
		return pow(state, 3.0);
	}

	double EvalStateDeriv(mfem::ElementTransformation &trans,
								 const mfem::IntegrationPoint &ip,
								 const double state) override
	{
		// mfem::Vector state;
		// stateGF->GetVectorValue(trans.ElementNo, ip, state);
		// double state_mag = state.Norml2();
		return 3.0*pow(state, 2.0);
	}
};

} // namespace electromag_data

#endif