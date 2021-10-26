#include <vector>

#include "mfem.hpp"

#include "utils.hpp"
#include "mach_input.hpp"
#include "mach_integrator.hpp"
#include "mach_nonlinearform.hpp"

namespace mach
{
int getSize(const MachNonlinearForm &form)
{
   return form.nf.ParFESpace()->GetTrueVSize();
}

void setInputs(MachNonlinearForm &form, const MachInputs &inputs)
{
   for (const auto &in : inputs)
   {
      const auto &input = in.second;
      if (input.isField())
      {
         const auto &name = in.first;
         auto it = form.nf_fields->find(name);
         if (it != form.nf_fields->end())
         {
            auto &field = it->second;
            field.GetTrueVector().SetDataAndSize(
                input.getField(), field.ParFESpace()->GetTrueVSize());
            field.SetFromTrueVector();
         }
      }
   }
   setInputs(form.integs, inputs);
}

void setOptions(MachNonlinearForm &form, const nlohmann::json &options)
{
   setOptions(form.integs, options);

   if (options.contains("ess-bdr"))
   {
      auto fes = *form.nf.ParFESpace();
      mfem::Array<int> ess_bdr(fes.GetParMesh()->bdr_attributes.Max());
      ess_bdr = 0;
      auto tmp = options["ess-bdr"].get<std::vector<int>>();
      for (auto &bdr : tmp)
      {
         ess_bdr[bdr - 1] = 1;
      }
      mfem::Array<int> ess_tdof_list;
      fes.GetEssentialTrueDofs(ess_bdr, ess_tdof_list);
      if (ess_tdof_list != nullptr)
      {
         form.nf.SetEssentialTrueDofs(ess_tdof_list);
      }
   }
}

void evaluate(MachNonlinearForm &form,
              const MachInputs &inputs,
              mfem::Vector &res_vec)
{
   auto *pfes = form.nf.ParFESpace();
   auto state = bufferToHypreParVector(inputs.at("state").getField(), *pfes);
   form.nf.Mult(state, res_vec);
}

mfem::Operator &getJacobian(MachNonlinearForm &form,
                            const MachInputs &inputs,
                            std::string wrt)
{
   auto *pfes = form.nf.ParFESpace();
   auto state = bufferToHypreParVector(inputs.at("state").getField(), *pfes);
   return form.nf.GetGradient(state);
}

}  // namespace mach
