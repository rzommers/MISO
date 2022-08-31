#ifndef MACH_COMMON_OUTPUTS
#define MACH_COMMON_OUTPUTS

#include <string>
#include <unordered_map>

#include "mfem.hpp"
#include "nlohmann/json.hpp"

#include "functional_output.hpp"
#include "mach_input.hpp"

namespace mach
{
class VolumeFunctional : public FunctionalOutput
{
public:
   friend inline int getSize(const VolumeFunctional &output)
   {
      const auto &fun_output = dynamic_cast<const FunctionalOutput &>(output);
      return getSize(fun_output);
   }

   friend void setOptions(VolumeFunctional &output,
                          const nlohmann::json &options)
   {
      auto &fun_output = dynamic_cast<FunctionalOutput &>(output);
      setOptions(fun_output, options);
   }

   friend void setInputs(VolumeFunctional &output, const MachInputs &inputs)
   {
      auto &fun_output = dynamic_cast<FunctionalOutput &>(output);
      setInputs(fun_output, inputs);
   }

   friend double calcOutput(VolumeFunctional &output, const MachInputs &inputs);

   VolumeFunctional(std::map<std::string, FiniteElementState> &fields,
                    const nlohmann::json &options);
};

class MassFunctional : public FunctionalOutput
{
public:
   friend inline int getSize(const MassFunctional &output)
   {
      const auto &fun_output = dynamic_cast<const FunctionalOutput &>(output);
      return getSize(fun_output);
   }

   friend void setOptions(MassFunctional &output, const nlohmann::json &options)
   {
      auto &fun_output = dynamic_cast<FunctionalOutput &>(output);
      setOptions(fun_output, options);
   }

   friend void setInputs(MassFunctional &output, const MachInputs &inputs)
   {
      auto &fun_output = dynamic_cast<FunctionalOutput &>(output);
      setInputs(fun_output, inputs);
   }

   friend double calcOutput(MassFunctional &output, const MachInputs &inputs);

   MassFunctional(std::map<std::string, FiniteElementState> &fields,
                  const nlohmann::json &components,
                  const nlohmann::json &materials,
                  const nlohmann::json &options);

private:
   /// Density
   std::unique_ptr<mfem::Coefficient> rho;
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
                         const MachInputs &inputs)
   {
      setInputs(output.state_integ, inputs);
      setInputs(output.volume, inputs);
   }

   friend double calcOutput(StateAverageFunctional &output,
                            const MachInputs &inputs)
   {
      double state = calcOutput(output.state_integ, inputs);
      double volume = calcOutput(output.volume, inputs);
      return state / volume;
   }

   StateAverageFunctional(mfem::ParFiniteElementSpace &fes,
                          std::map<std::string, FiniteElementState> &fields);

   StateAverageFunctional(mfem::ParFiniteElementSpace &fes,
                          std::map<std::string, FiniteElementState> &fields,
                          const nlohmann::json &options);

private:
   FunctionalOutput state_integ;
   FunctionalOutput volume;
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
                         const MachInputs &inputs)
   {
      setInputs(output.state_integ, inputs);
      setInputs(output.volume, inputs);
   }

   friend double calcOutput(AverageMagnitudeCurlState &output,
                            const MachInputs &inputs)
   {
      double state = calcOutput(output.state_integ, inputs);
      double volume = calcOutput(output.volume, inputs);
      return state / volume;
   }

   AverageMagnitudeCurlState(mfem::ParFiniteElementSpace &fes,
                             std::map<std::string, FiniteElementState> &fields);

   AverageMagnitudeCurlState(mfem::ParFiniteElementSpace &fes,
                             std::map<std::string, FiniteElementState> &fields,
                             const nlohmann::json &options);

private:
   FunctionalOutput state_integ;
   FunctionalOutput volume;
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
                         const MachInputs &inputs)
   {
      setInputs(output.numerator, inputs);
      setInputs(output.denominator, inputs);
   }

   friend double calcOutput(IEAggregateFunctional &output,
                            const MachInputs &inputs)
   {
      double num = calcOutput(output.numerator, inputs);
      double denom = calcOutput(output.denominator, inputs);
      return num / denom;
   }

   IEAggregateFunctional(mfem::ParFiniteElementSpace &fes,
                         std::map<std::string, FiniteElementState> &fields,
                         const nlohmann::json &options);

private:
   FunctionalOutput numerator;
   FunctionalOutput denominator;
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
                         const MachInputs &inputs)
   {
      setInputs(output.numerator, inputs);
      setInputs(output.denominator, inputs);
   }

   friend double calcOutput(IECurlMagnitudeAggregateFunctional &output,
                            const MachInputs &inputs)
   {
      double num = calcOutput(output.numerator, inputs);
      double denom = calcOutput(output.denominator, inputs);
      return num / denom;
   }

   IECurlMagnitudeAggregateFunctional(
       mfem::ParFiniteElementSpace &fes,
       std::map<std::string, FiniteElementState> &fields,
       const nlohmann::json &options);

private:
   FunctionalOutput numerator;
   FunctionalOutput denominator;
};

}  // namespace mach

#endif