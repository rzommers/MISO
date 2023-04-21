#ifndef MACH_CONDUCTIVITY_COEFFICIENT
#define MACH_CONDUCTIVITY_COEFFICIENT

#include <map>
#include <string>

#include "adept.h"
#include "mfem.hpp"

#include "coefficient.hpp"
#include "mach_input.hpp"

namespace mach
{
class ConductivityCoefficient : public StateCoefficient
{
public:
   friend void setInputs(ConductivityCoefficient &current,
                         const MachInputs &inputs)
   { }

   double Eval(mfem::ElementTransformation &trans,
               const mfem::IntegrationPoint &ip) override;

   double Eval(mfem::ElementTransformation &trans,
               const mfem::IntegrationPoint &ip,
               double state) override;

   double EvalStateDeriv(mfem::ElementTransformation &trans,
                         const mfem::IntegrationPoint &ip,
                         double state) override;

   double EvalState2ndDeriv(mfem::ElementTransformation &trans,
                            const mfem::IntegrationPoint &ip,
                            const double state) override;

   /// TODO: Adapt EvalRevDiff as needed for conductivity
   void EvalRevDiff(const double Q_bar,
                    mfem::ElementTransformation &trans,
                    const mfem::IntegrationPoint &ip,
                    mfem::DenseMatrix &PointMat_bar) override;

   ConductivityCoefficient(const nlohmann::json &sigma_options,
                           const nlohmann::json &materials);

private:
   /// The underlying coefficient that does all the heavy lifting
   MeshDependentCoefficient sigma;
};

}  // namespace mach

#endif