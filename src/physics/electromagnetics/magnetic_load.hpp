#ifndef MISO_MAGNETIC_LOAD
#define MISO_MAGNETIC_LOAD

#include "mfem.hpp"
#include "nlohmann/json.hpp"

#include "coefficient.hpp"
#include "finite_element_state.hpp"
#include "miso_input.hpp"
#include "miso_linearform.hpp"
#include "magnetic_source_functions.hpp"

namespace miso
{
class MagneticLoad final
{
public:
   friend int getSize(const MagneticLoad &load) { return getSize(load.lf); }

   friend inline void setInputs(MagneticLoad &load, const MISOInputs &inputs)
   {
      setInputs(load.lf, inputs);
   }

   friend inline void setOptions(MagneticLoad &load,
                                 const nlohmann::json &options)
   {
      setOptions(load.lf, options);
   }

   /// Assemble the load vector on the true dofs and store in tv
   friend inline void addLoad(MagneticLoad &load, mfem::Vector &tv)
   {
      addLoad(load.lf, tv);
   }

   friend double jacobianVectorProduct(MagneticLoad &load,
                                       const mfem::Vector &wrt_dot,
                                       const std::string &wrt)
   {
      return jacobianVectorProduct(load.lf, wrt_dot, wrt);
   }

   friend void jacobianVectorProduct(MagneticLoad &load,
                                     const mfem::Vector &wrt_dot,
                                     const std::string &wrt,
                                     mfem::Vector &res_dot)
   {
      jacobianVectorProduct(load.lf, wrt_dot, wrt, res_dot);
   }

   friend inline double vectorJacobianProduct(MagneticLoad &load,
                                              const mfem::Vector &load_bar,
                                              const std::string &wrt)
   {
      return vectorJacobianProduct(load.lf, load_bar, wrt);
   }

   friend inline void vectorJacobianProduct(MagneticLoad &load,
                                            const mfem::Vector &load_bar,
                                            const std::string &wrt,
                                            mfem::Vector &wrt_bar)
   {
      vectorJacobianProduct(load.lf, load_bar, wrt, wrt_bar);
   }

   MagneticLoad(adept::Stack &diff_stack,
                mfem::ParFiniteElementSpace &fes,
                std::map<std::string, FiniteElementState> &fields,
                const nlohmann::json &options,
                const nlohmann::json &materials,
                mfem::Coefficient &nu);

private:
   std::unordered_map<std::string, mfem::ParGridFunction> mag_load_fields;
   MISOLinearForm lf;
   /// Coefficient to represent magnetization
   std::unique_ptr<MagnetizationCoefficient> mag_coeff;
   std::unique_ptr<mfem::ScalarVectorProductCoefficient> nuM;
};

}  // namespace miso

#endif
