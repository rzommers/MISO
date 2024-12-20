#include <memory>
#include <string>

#include "mfem.hpp"
#include "nlohmann/json.hpp"

#include "linesearch.hpp"

#include "relaxed_newton.hpp"
#include "utils.hpp"

namespace
{

std::unique_ptr<miso::LineSearch> createLineSearch(
    const std::string &type,
    const nlohmann::json &options)
{
   if (type == "backtracking")
   {
      auto ls = std::make_unique<miso::BacktrackingLineSearch>();
      if (options.is_object())
      {
         ls->mu = options.value("mu", ls->mu);
         ls->rho_hi = options.value("rhohi", ls->rho_hi);
         ls->rho_lo = options.value("rholo", ls->rho_lo);
         ls->interp_order = options.value("interp-order", ls->interp_order);
         ls->max_iter = options.value("maxiter", ls->max_iter);
      }
      return ls;
   }
   else
   {
      std::string err_msg = "Unknown linesearch type \"";
      err_msg += type;
      err_msg += "\"!\n";
      throw miso::MISOException(err_msg);
   }
}

}  // namespace

namespace miso
{
RelaxedNewton::RelaxedNewton(MPI_Comm comm, const nlohmann::json &options)
 : NewtonSolver(comm)
{
   if (options.contains("linesearch"))
   {
      const auto &ls_opts = options["linesearch"];
      if (ls_opts.is_string())
      {
         auto ls_type = ls_opts.get<std::string>();
         ls = createLineSearch(ls_type, {});
      }
      else
      {
         auto ls_type = ls_opts["type"].get<std::string>();
         ls = createLineSearch(ls_type, ls_opts);
      }
   }
   else
   {
      ls = std::make_unique<BacktrackingLineSearch>();
   }
}

double RelaxedNewton::ComputeScalingFactor(const mfem::Vector &x,
                                           const mfem::Vector &b) const
{
   auto calcRes = [&](const mfem::Vector &x, mfem::Vector &res)
   {
      oper->Mult(x, res);
      const bool have_b = (b.Size() == Height());
      if (have_b)
      {
         res -= b;
      }
   };

   Phi phi(calcRes, x, c, r, *grad);

   return ls->search(phi, phi.phi0, phi.dphi0, 1.0);
}

}  // namespace miso
