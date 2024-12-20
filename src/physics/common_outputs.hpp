#ifndef MISO_COMMON_OUTPUTS
#define MISO_COMMON_OUTPUTS

#include <string>
#include <unordered_map>

#include "coefficient.hpp"
#include "mfem.hpp"
#include "nlohmann/json.hpp"

#include "functional_output.hpp"
#include "miso_input.hpp"

namespace miso
{
class VolumeFunctional final
{
public:
   friend inline int getSize(const VolumeFunctional &output)
   {
      return getSize(output.output);
   }

   friend void setOptions(VolumeFunctional &output,
                          const nlohmann::json &options)
   {
      setOptions(output.output, options);
   }

   friend void setInputs(VolumeFunctional &output, const MISOInputs &inputs)
   {
      setInputs(output.output, inputs);
   }

   friend double calcOutput(VolumeFunctional &output, const MISOInputs &inputs)
   {
      return calcOutput(output.output, inputs);
   }

   friend double jacobianVectorProduct(VolumeFunctional &output,
                                       const mfem::Vector &wrt_dot,
                                       const std::string &wrt)
   {
      return jacobianVectorProduct(output.output, wrt_dot, wrt);
   }

   friend void vectorJacobianProduct(VolumeFunctional &output,
                                     const mfem::Vector &out_bar,
                                     const std::string &wrt,
                                     mfem::Vector &wrt_bar)
   {
      vectorJacobianProduct(output.output, out_bar, wrt, wrt_bar);
   }

   VolumeFunctional(std::map<std::string, FiniteElementState> &fields,
                    const nlohmann::json &options);

private:
   FunctionalOutput output;
};

class MassFunctional final
{
public:
   friend inline int getSize(const MassFunctional &output)
   {
      return getSize(output.output);
   }

   friend void setOptions(MassFunctional &output, const nlohmann::json &options)
   {
      setOptions(output.output, options);
   }

   friend void setInputs(MassFunctional &output, const MISOInputs &inputs)
   {
      output.rho->setInputs(inputs);
      output.drho_df->setInputs(inputs);
      setInputs(output.output, inputs);
   }

   friend double calcOutput(MassFunctional &output, const MISOInputs &inputs)
   {
      output.rho->setInputs(inputs);
      output.drho_df->setInputs(inputs);
      setInputs(output.output, inputs);
      return calcOutput(output.output, inputs);
   }

   friend double jacobianVectorProduct(MassFunctional &output,
                                       const mfem::Vector &wrt_dot,
                                       const std::string &wrt)
   {
      return jacobianVectorProduct(output.output, wrt_dot, wrt);
   }

   friend double vectorJacobianProduct(MassFunctional &output,
                                       const mfem::Vector &out_bar,
                                       const std::string &wrt);

   friend void vectorJacobianProduct(MassFunctional &output,
                                     const mfem::Vector &out_bar,
                                     const std::string &wrt,
                                     mfem::Vector &wrt_bar)
   {
      vectorJacobianProduct(output.output, out_bar, wrt, wrt_bar);
   }

   MassFunctional(std::map<std::string, FiniteElementState> &fields,
                  const nlohmann::json &components,
                  const nlohmann::json &materials,
                  const nlohmann::json &options);

private:
   FunctionalOutput output;
   /// Density
   std::unique_ptr<miso::MeshDependentCoefficient> rho;
   /// Derivative of density wrt fill factor
   std::unique_ptr<miso::MeshDependentCoefficient> drho_df;
   /// Coefficient pointer that integrators use
   std::unique_ptr<mfem::Coefficient *> rho_ptr;
};

class StateAverageFunctional
{
public:
   friend inline int getSize(const StateAverageFunctional &output)
   {
      return getSize(output.state_integ);
   }

   friend void setOptions(StateAverageFunctional &output,
                          const nlohmann::json &options)
   {
      setOptions(output.state_integ, options);
      setOptions(output.volume, options);
   }

   friend void setInputs(StateAverageFunctional &output,
                         const MISOInputs &inputs)
   {
      output.inputs = &inputs;
      setInputs(output.state_integ, inputs);
      setInputs(output.volume, inputs);
   }

   friend double calcOutput(StateAverageFunctional &output,
                            const MISOInputs &inputs)
   {
      output.inputs = &inputs;
      double state = calcOutput(output.state_integ, inputs);
      double volume = calcOutput(output.volume, inputs);
      return state / volume;
   }

   friend double jacobianVectorProduct(StateAverageFunctional &output,
                                       const mfem::Vector &wrt_dot,
                                       const std::string &wrt)
   {
      const MISOInputs &inputs = *output.inputs;

      double state = calcOutput(output.state_integ, inputs);
      double volume = calcOutput(output.volume, inputs);

      auto state_dot =
          volume * jacobianVectorProduct(output.state_integ, wrt_dot, wrt);
      state_dot -= state * jacobianVectorProduct(output.volume, wrt_dot, wrt);
      state_dot /= pow(volume, 2);
      return state_dot;
   }

   friend void vectorJacobianProduct(StateAverageFunctional &output,
                                     const mfem::Vector &out_bar,
                                     const std::string &wrt,
                                     mfem::Vector &wrt_bar)
   {
      const MISOInputs &inputs = *output.inputs;

      double state = calcOutput(output.state_integ, inputs);
      double volume = calcOutput(output.volume, inputs);

      output.scratch.SetSize(wrt_bar.Size());

      output.scratch = 0.0;
      vectorJacobianProduct(output.state_integ, out_bar, wrt, output.scratch);
      wrt_bar.Add(1 / volume, output.scratch);

      output.scratch = 0.0;
      vectorJacobianProduct(output.volume, out_bar, wrt, output.scratch);
      wrt_bar.Add(-state / pow(volume, 2), output.scratch);
   }

   StateAverageFunctional(mfem::ParFiniteElementSpace &fes,
                          std::map<std::string, FiniteElementState> &fields)
    : StateAverageFunctional(fes, fields, {})
   { }

   StateAverageFunctional(mfem::ParFiniteElementSpace &fes,
                          std::map<std::string, FiniteElementState> &fields,
                          const nlohmann::json &options);

private:
   FunctionalOutput state_integ;
   FunctionalOutput volume;

   MISOInputs const *inputs = nullptr;
   mfem::Vector scratch;
};

class AverageMagnitudeCurlState
{
public:
   friend inline int getSize(const AverageMagnitudeCurlState &output)
   {
      return getSize(output.state_integ);
   }

   friend void setOptions(AverageMagnitudeCurlState &output,
                          const nlohmann::json &options)
   {
      setOptions(output.state_integ, options);
      setOptions(output.volume, options);
   }

   friend void setInputs(AverageMagnitudeCurlState &output,
                         const MISOInputs &inputs)
   {
      output.inputs = &inputs;
      setInputs(output.state_integ, inputs);
      setInputs(output.volume, inputs);
   }

   friend double calcOutput(AverageMagnitudeCurlState &output,
                            const MISOInputs &inputs)
   {
      output.inputs = &inputs;
      double state = calcOutput(output.state_integ, inputs);
      double volume = calcOutput(output.volume, inputs);
      return state / volume;
   }

   friend double jacobianVectorProduct(AverageMagnitudeCurlState &output,
                                       const mfem::Vector &wrt_dot,
                                       const std::string &wrt);

   friend void vectorJacobianProduct(AverageMagnitudeCurlState &output,
                                     const mfem::Vector &out_bar,
                                     const std::string &wrt,
                                     mfem::Vector &wrt_bar);

   AverageMagnitudeCurlState(mfem::ParFiniteElementSpace &fes,
                             std::map<std::string, FiniteElementState> &fields)
    : AverageMagnitudeCurlState(fes, fields, {})
   { }

   AverageMagnitudeCurlState(mfem::ParFiniteElementSpace &fes,
                             std::map<std::string, FiniteElementState> &fields,
                             const nlohmann::json &options);

private:
   FunctionalOutput state_integ;
   FunctionalOutput volume;
   MISOInputs const *inputs = nullptr;
   mfem::Vector scratch;
};

class IEAggregateFunctional
{
public:
   friend inline int getSize(const IEAggregateFunctional &output)
   {
      return getSize(output.numerator);
   }

   friend void setOptions(IEAggregateFunctional &output,
                          const nlohmann::json &options)
   {
      setOptions(output.numerator, options);
      setOptions(output.denominator, options);
   }

   friend void setInputs(IEAggregateFunctional &output,
                         const MISOInputs &inputs)
   {
      output.inputs = &inputs;
      setInputs(output.numerator, inputs);
      setInputs(output.denominator, inputs);
   }

   friend double calcOutput(IEAggregateFunctional &output,
                            const MISOInputs &inputs)
   {
      mfem::Vector state;
      setVectorFromInputs(inputs, "state", state);
      double true_max = state.Max();
      setInputs(output, {{"true_max", true_max}});

      double num = calcOutput(output.numerator, inputs);
      double denom = calcOutput(output.denominator, inputs);

      std::cout << "IEAggregateFunctional::calcOutput\n";
      std::cout << "num: " << num << " denom: " << denom << "\n";

      return num / denom;
   }

   friend double jacobianVectorProduct(IEAggregateFunctional &output,
                                       const mfem::Vector &wrt_dot,
                                       const std::string &wrt);

   friend void vectorJacobianProduct(IEAggregateFunctional &output,
                                     const mfem::Vector &out_bar,
                                     const std::string &wrt,
                                     mfem::Vector &wrt_bar);

   IEAggregateFunctional(mfem::ParFiniteElementSpace &fes,
                         std::map<std::string, FiniteElementState> &fields,
                         const nlohmann::json &options);

private:
   FunctionalOutput numerator;
   FunctionalOutput denominator;
   MISOInputs const *inputs = nullptr;
   mfem::Vector scratch;
};

class IECurlMagnitudeAggregateFunctional
{
public:
   friend inline int getSize(const IECurlMagnitudeAggregateFunctional &output)
   {
      return getSize(output.numerator);
   }

   friend void setOptions(IECurlMagnitudeAggregateFunctional &output,
                          const nlohmann::json &options)
   {
      setOptions(output.numerator, options);
      setOptions(output.denominator, options);
   }

   friend void setInputs(IECurlMagnitudeAggregateFunctional &output,
                         const MISOInputs &inputs)
   {
      output.inputs = &inputs;
      setInputs(output.numerator, inputs);
      setInputs(output.denominator, inputs);
   }

   friend double calcOutput(IECurlMagnitudeAggregateFunctional &output,
                            const MISOInputs &inputs)
   {
      double num = calcOutput(output.numerator, inputs);
      double denom = calcOutput(output.denominator, inputs);
      return num / denom;
   }

   friend double jacobianVectorProduct(
       IECurlMagnitudeAggregateFunctional &output,
       const mfem::Vector &wrt_dot,
       const std::string &wrt);

   friend void vectorJacobianProduct(IECurlMagnitudeAggregateFunctional &output,
                                     const mfem::Vector &out_bar,
                                     const std::string &wrt,
                                     mfem::Vector &wrt_bar);

   IECurlMagnitudeAggregateFunctional(
       mfem::ParFiniteElementSpace &fes,
       std::map<std::string, FiniteElementState> &fields,
       const nlohmann::json &options);

private:
   FunctionalOutput numerator;
   FunctionalOutput denominator;
   MISOInputs const *inputs = nullptr;
   mfem::Vector scratch;
};

/// TODO: Either create new induced exponential functional for demag here that
/// looks much like IEAggregateFunctional
/// TODO: OR add conditional logic to
/// IEAggregateFunctional::IEAggregateFunctional in common_outputs.cpp to use
/// the new integrators

}  // namespace miso

#endif
