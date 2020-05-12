#ifndef ELECTROMAG_TEST_DATA
#define ELECTROMAG_TEST_DATA

#include <limits>
#include <random>

#include "mfem.hpp"
#include "json.hpp"

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

double randState(const mfem::Vector &x)
{
   return 2.0 * uniform_rand(gen) - 1.0;
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

void mag_func(const mfem::Vector &x, mfem::Vector &y)
{
   y = 1.0;
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
      // return pow(state, 3.0);
      return 0.5*pow(state+1, -0.5);
   }

   double EvalStateDeriv(mfem::ElementTransformation &trans,
                        const mfem::IntegrationPoint &ip,
                        const double state) override
   {
      // mfem::Vector state;
      // stateGF->GetVectorValue(trans.ElementNo, ip, state);
      // double state_mag = state.Norml2();
      // return 3.0*pow(state, 2.0);
      return -0.25*pow(state+1, -1.5);
   }
};

nlohmann::json getBoxOptions(int order)
{
   nlohmann::json box_options = {
      {"space-dis", {
         {"basis-type", "nedelec"},
         {"degree", order}
      }},
      {"steady", true},
      {"lin-solver", {
         {"type", "hypregmres"},
         {"pctype", "hypreams"},
         {"printlevel", -1},
         {"maxiter", 100},
         {"abstol", 1e-10},
         {"reltol", 1e-14}
      }},
      {"adj-solver", {
         {"type", "hypregmres"},
         {"pctype", "hypreams"},
         {"printlevel", -1},
         {"maxiter", 100},
         {"abstol", 1e-10},
         {"reltol", 1e-14}
      }},
      {"newton", {
         {"printlevel", -1},
         {"reltol", 1e-10},
         {"abstol", 0.0}
      }},
      {"components", {
         {"attr1", {
            {"material", "box1"},
            {"attr", 1},
            {"linear", true}
         }},
         {"attr2", {
            {"material", "box2"},
            {"attr", 2},
            {"linear", true}
         }}
      }},
      {"problem-opts", {
         {"fill-factor", 1.0},
         {"current-density", 1.0},
         {"current", {
            {"box1", {1}},
            {"box2", {2}}
         }},
         {"box", true}
      }},
      {"outputs", {
         {"co-energy", {}}
      }}
   };
   return box_options;
}

std::unique_ptr<mfem::Mesh> getMesh(int nxy = 2, int nz = 2)
{
   using namespace mfem;
   // generate a simple tet mesh
   std::unique_ptr<Mesh> mesh(new Mesh(nxy, nxy, nz,
                              Element::TETRAHEDRON, true /* gen. edges */, 1.0,
                              1.0, (double)nz / (double)nxy, true));

   mesh->ReorientTetMesh();
   mesh->EnsureNodes();

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
         if (vtx[1] <= 0.5)
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
   return mesh;
}

} // namespace electromag_data

#endif