#ifndef MISO_MAGNETOSTATIC_RESIDUAL
#define MISO_MAGNETOSTATIC_RESIDUAL

#include <map>
#include <memory>
#include <string>

#include "mfem.hpp"
#include "nlohmann/json.hpp"

#include "coefficient.hpp"
#include "miso_input.hpp"
#include "miso_residual.hpp"
#include "miso_nonlinearform.hpp"
#include "thermal_integ.hpp"

namespace miso
{
class ThermalResidual final
{
public:
   friend int getSize(const ThermalResidual &residual);

   friend void setInputs(ThermalResidual &residual, const MISOInputs &inputs);

   friend void setOptions(ThermalResidual &residual,
                          const nlohmann::json &options);

   friend void evaluate(ThermalResidual &residual,
                        const MISOInputs &inputs,
                        mfem::Vector &res_vec);

   friend void linearize(ThermalResidual &residual,
                         const miso::MISOInputs &inputs);

   friend mfem::Operator &getJacobian(ThermalResidual &residual,
                                      const MISOInputs &inputs,
                                      const std::string &wrt);

   friend mfem::Operator &getJacobianTranspose(ThermalResidual &residual,
                                               const miso::MISOInputs &inputs,
                                               const std::string &wrt);

   friend void setUpAdjointSystem(ThermalResidual &residual,
                                  mfem::Solver &adj_solver,
                                  const miso::MISOInputs &inputs,
                                  mfem::Vector &state_bar,
                                  mfem::Vector &adjoint);

   friend void finalizeAdjointSystem(ThermalResidual &residual,
                                     mfem::Solver &adj_solver,
                                     const MISOInputs &inputs,
                                     mfem::Vector &state_bar,
                                     mfem::Vector &adjoint);

   friend double jacobianVectorProduct(ThermalResidual &residual,
                                       const mfem::Vector &wrt_dot,
                                       const std::string &wrt);

   friend void jacobianVectorProduct(ThermalResidual &residual,
                                     const mfem::Vector &wrt_dot,
                                     const std::string &wrt,
                                     mfem::Vector &res_dot);

   friend double vectorJacobianProduct(ThermalResidual &residual,
                                       const mfem::Vector &res_bar,
                                       const std::string &wrt);

   friend void vectorJacobianProduct(ThermalResidual &residual,
                                     const mfem::Vector &res_bar,
                                     const std::string &wrt,
                                     mfem::Vector &wrt_bar);

   friend mfem::Solver *getPreconditioner(ThermalResidual &residual);

   ThermalResidual(mfem::ParFiniteElementSpace &fes,
                   std::map<std::string, FiniteElementState> &fields,
                   const nlohmann::json &options,
                   const nlohmann::json &materials);

private:
   /// Nonlinear form that handles the weak form
   MISONonlinearForm res;
   /// coefficient for weakly imposed boundary conditions
   std::unique_ptr<MeshDependentCoefficient> g;
   // Material dependent coefficient representing thermal conductivity
   std::unique_ptr<MeshDependentCoefficient> kappa;
   /// Material dependent coefficient representing density
   std::unique_ptr<MeshDependentCoefficient> rho;
   // /// Material dependent coefficient representing specific heat
   // /// (at constant volume)
   // std::unique_ptr<MeshDependentCoefficient> cv;

   /// Right-hand-side load vector to apply to residual
   mfem::Vector load;

   /// preconditioner for inverting residual's state Jacobian
   std::unique_ptr<mfem::Solver> prec;

   static std::unique_ptr<mfem::Solver> constructPreconditioner(
       mfem::ParFiniteElementSpace &fes,
       const nlohmann::json &prec_options)
   {
      auto prec_type = prec_options["type"].get<std::string>();
      if (prec_type == "hypreboomeramg")
      {
         auto amg = std::make_unique<mfem::HypreBoomerAMG>();
         amg->SetPrintLevel(prec_options["printlevel"].get<int>());
         return amg;
      }
      else if (prec_type == "hypreilu")
      {
         auto ilu = std::make_unique<mfem::HypreILU>();
         HYPRE_ILUSetType(*ilu, prec_options["ilu-type"]);
         HYPRE_ILUSetLevelOfFill(*ilu, prec_options["lev-fill"]);
         HYPRE_ILUSetLocalReordering(*ilu, prec_options["ilu-reorder"]);
         HYPRE_ILUSetPrintLevel(*ilu, prec_options["printlevel"]);
      }
      return nullptr;
   }
};

}  // namespace miso

#endif
