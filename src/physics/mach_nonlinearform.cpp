#include <vector>

#include "mfem.hpp"

#include "mach_input.hpp"
#include "mach_integrator.hpp"
#include "mach_nonlinearform.hpp"
#include "utils.hpp"

namespace mach
{
int getSize(const MachNonlinearForm &form)
{
   return form.nf.ParFESpace()->GetTrueVSize();
}

void setInputs(MachNonlinearForm &form, const MachInputs &inputs)
{
   for (const auto &[name, input] : inputs)
   {
      if (std::holds_alternative<InputVector>(input))
      {
         auto it = form.nf_fields.find(name);
         if (it != form.nf_fields.end())
         {
            auto &field = it->second;
            mfem::Vector field_tv;
            setVectorFromInput(input, field_tv);

            field.distributeSharedDofs(field_tv);
         }
      }
   }
   setInputs(form.integs, inputs);
}

void setOptions(MachNonlinearForm &form, const nlohmann::json &options)
{
   setOptions(form.integs, options);

   if (options.contains("bcs"))
   {
      if (options["bcs"].contains("essential"))
      {
         auto &fes = *form.nf.ParFESpace();
         form.ess_bdr.SetSize(fes.GetParMesh()->bdr_attributes.Max());
         getMFEMBoundaryArray(options["bcs"]["essential"], form.ess_bdr);
         mfem::Array<int> ess_tdof_list;
         fes.GetEssentialTrueDofs(form.ess_bdr, ess_tdof_list);
         form.nf.SetEssentialTrueDofs(ess_tdof_list);
      }
   }
}

double calcFormOutput(MachNonlinearForm &form, const MachInputs &inputs)
{
   mfem::Vector state;
   setVectorFromInputs(inputs, "state", state, false, true);
   setInputs(form.integs, inputs);
   return form.nf.GetEnergy(state);
}

void evaluate(MachNonlinearForm &form,
              const MachInputs &inputs,
              mfem::Vector &res_vec)
{
   mfem::Vector state;
   setVectorFromInputs(inputs, "state", state, false, true);
   form.nf.Mult(state, res_vec);

   const auto &ess_tdof_list = form.getEssentialDofs();
   if (ess_tdof_list.Size() == 0)
   {
      return;
   }

   if (auto bc_iter = form.nf_fields.find("dirichlet_bc");
       bc_iter != form.nf_fields.end())
   {
      auto &dirichlet_bc = bc_iter->second;
      dirichlet_bc.setTrueVec(form.scratch);

      for (int i = 0; i < ess_tdof_list.Size(); ++i)
      {
         res_vec(ess_tdof_list[i]) =
             state(ess_tdof_list[i]) - form.scratch(ess_tdof_list[i]);
      }
   }
}

void linearize(MachNonlinearForm &form, const MachInputs &inputs)
{
   std::cout << "In linearize!\n";
   setInputs(form, inputs);
   if (form.jac.Ptr() == nullptr)
   {
      getJacobian(form, inputs, "state");
   }
   if (form.jac_trans == nullptr)
   {
      getJacobianTranspose(form, inputs, "state");
   }
}

mfem::Operator &getJacobian(MachNonlinearForm &form,
                            const MachInputs &inputs,
                            const std::string &wrt)
{
   std::cout << "Re-assembling Jacobian!\n";

   mfem::Vector state;
   setVectorFromInputs(inputs, "state", state, false, true);
   // std::cout << "mach_nonlinearform.cpp, state=\n";
   // state.Print(mfem::out,25);

   mfem::Array<int> ess_tdof_list(form.nf.GetEssentialTrueDofs());
   mfem::Array<int> zeros;
   // Setting our essential true dofs to zero to full Jacobian is preserved
   form.nf.SetEssentialTrueDofs(zeros);

   // get our gradient with everything preserved
   auto *hypre_jac =
       dynamic_cast<mfem::HypreParMatrix *>(&form.nf.GetGradient(state));
   form.jac.Reset(hypre_jac, false);

   // mfem::HypreParMatrix adjoint_jac(*hypre_jac);
   // adjoint_jac.EliminateRows(ess_tdof_list);
   // form.adjoint_jac_trans =
   //     std::unique_ptr<mfem::Operator>(adjoint_jac.Transpose());

   // Impose boundary conditions on pGrad
   form.jac_e.Clear();
   form.jac_e.EliminateRowsCols(form.jac, ess_tdof_list);
   form.jac_e.EliminateRows(ess_tdof_list);

   // auto *hypre_jac_e = form.jac_e.As<mfem::HypreParMatrix>();
   // if (hypre_jac_e == nullptr)
   // {
   //    throw MachException(
   //        "setUpAdjointSystem (MachNonlinearForm) only supports "
   //        "Jacobian matrices assembled to a HypreParMatrix!\n");
   // }
   // form.jac_e_trans =
   // std::unique_ptr<mfem::Operator>(hypre_jac_e->Transpose());

   // reset our essential BCs to what they used to be
   form.nf.SetEssentialTrueDofs(ess_tdof_list);

   // std::cout << "essential tdofs:\n";
   // ess_tdof_list.Print(mfem::out, 1);

   // reset transposed Jacobian to null (to indicate we should re-transpose it)
   form.jac_trans = nullptr;

   return *form.jac;
}

mfem::Operator &getJacobianTranspose(MachNonlinearForm &form,
                                     const MachInputs &inputs,
                                     const std::string &wrt)
{
   if (form.jac_trans == nullptr)
   {
      std::cout << "Re-transposing Jacobian!\n";
      // getJacobian(form, inputs, wrt);

      auto *hypre_jac = form.jac.As<mfem::HypreParMatrix>();
      if (hypre_jac == nullptr)
      {
         throw MachException(
             "getJacobianTranspose (MachNonlinearForm) only supports "
             "Jacobian matrices assembled to a HypreParMatrix!\n");
      }

      form.jac_trans = std::unique_ptr<mfem::Operator>(hypre_jac->Transpose());
      // form.jac_trans = std::make_unique<mfem::HypreParMatrix>(*hypre_jac);
   }
   return *form.jac_trans;
}

void setUpAdjointSystem(MachNonlinearForm &form,
                        mfem::Solver &adj_solver,
                        const MachInputs &inputs,
                        mfem::Vector &state_bar,
                        mfem::Vector &adjoint)
{
   // std::cout << "Setting up adjoint system!\n";

   auto &jac_trans = getJacobianTranspose(form, inputs, "state");
   adj_solver.SetOperator(jac_trans);

   // auto *hypre_jac_trans = dynamic_cast<mfem::HypreParMatrix
   // *>(form.jac_trans.get()); auto *hypre_jac_e_trans =
   // dynamic_cast<mfem::HypreParMatrix *>(form.jac_e_trans.get());

   // form.adjoint_jac_trans = std::unique_ptr<mfem::Operator>(Add(1.0,
   // *hypre_jac_trans, 1.0, *hypre_jac_e_trans));
   // adj_solver.SetOperator(*form.adjoint_jac_trans);

   const auto &ess_tdof_list = form.getEssentialDofs();
   if (ess_tdof_list.Size() == 0)
   {
      return;
   }

   /// New approach
   state_bar.GetSubVector(ess_tdof_list, form.scratch);
   state_bar.SetSubVector(ess_tdof_list, 0.0);
   adjoint.SetSubVector(ess_tdof_list, 0.0);

   /// Approach 1
   // adj_solver.Mult(state_bar, adjoint);

   // auto *hypre_jac_e = form.jac_e.As<mfem::HypreParMatrix>();
   // if (hypre_jac_e == nullptr)
   // {
   //    throw MachException(
   //        "setUpAdjointSystem (MachNonlinearForm) only supports "
   //        "Jacobian matrices assembled to a HypreParMatrix!\n");
   // }
   // hypre_jac_e->MultTranspose(-1.0, adjoint, 1.0, state_bar);

   /// Approach 2
   // state_bar.GetSubVector(ess_tdof_list, form.scratch);
   // state_bar.SetSubVector(ess_tdof_list, 0.0);

   /// Approach 3
   // form.adj_work1 = state_bar;
   // state_bar.SetSubVector(ess_tdof_list, 0.0);
   // form.adj_work2 = state_bar;

   /// Approach 4
   // form.adj_work1 = state_bar;
   // state_bar.SetSubVector(ess_tdof_list, 0.0);
}

void finalizeAdjointSystem(MachNonlinearForm &form,
                           mfem::Solver &adj_solver,
                           const MachInputs &inputs,
                           mfem::Vector &state_bar,
                           mfem::Vector &adjoint)
{
   const auto &ess_tdof_list = form.getEssentialDofs();
   if (ess_tdof_list.Size() == 0)
   {
      return;
   }

   /// New approach
   adjoint.SetSubVector(ess_tdof_list, form.scratch);

   auto *hypre_jac_e = form.jac_e.As<mfem::HypreParMatrix>();
   if (hypre_jac_e == nullptr)
   {
      throw MachException(
          "setUpAdjointSystem (MachNonlinearForm) only supports "
          "Jacobian matrices assembled to a HypreParMatrix!\n");
   }

   hypre_jac_e->MultTranspose(1.0, adjoint, -1.0, adjoint);

   // // adjoint.SetSubVector(ess_tdof_list, 0.0);
   // mfem::Vector test;
   // adjoint.GetSubVector(ess_tdof_list, test);

   // std::cout << "test norm: " << test.Norml2() << "\n";

   /// Approach 2
   // adjoint.SetSubVector(ess_tdof_list, form.scratch);

   /// Approach 3
   // mfem::Vector diff = form.adj_work2;
   // diff -= form.adj_work1;
   // adjoint -= diff;

   // std::cout << "adjoint:\n";
   // adjoint.Print(mfem::out, 1);

   /// Approach 4
   // auto *hypre_jac_e = form.jac_e.As<mfem::HypreParMatrix>();
   // if (hypre_jac_e == nullptr)
   // {
   //    throw MachException(
   //        "setUpAdjointSystem (MachNonlinearForm) only supports "
   //        "Jacobian matrices assembled to a HypreParMatrix!\n");
   // }
   // hypre_jac_e->MultTranspose(-1.0, form.adj_work1, 1.0, adjoint);

   // adj_solver.SetOperator(*form.jac_e_trans);
   // form.adj_work2.SetSize(adjoint.Size());
   // form.adj_work2 = 0.0;

   // state_bar.SetSubVector(ess_tdof_list, 0.0);

   // // adj_solver.Mult(form.adj_work1, form.adj_work2);
   // adj_solver.Mult(state_bar, form.adj_work2);
   // adjoint += form.adj_work2;
}

double jacobianVectorProduct(MachNonlinearForm &form,
                             const mfem::Vector &wrt_dot,
                             const std::string &wrt)
{
   if (form.fwd_scalar_sens.count(wrt) != 0)
   {
      throw NotImplementedException(
          "not implemented for scalar sensitivities!\n");
   }
   return 0.0;
}

void jacobianVectorProduct(MachNonlinearForm &form,
                           const mfem::Vector &wrt_dot,
                           const std::string &wrt,
                           mfem::Vector &res_dot)
{
   if (wrt == "state")
   {
      form.scratch = wrt_dot;
      const auto &ess_tdof_list = form.getEssentialDofs();
      form.scratch.SetSubVector(ess_tdof_list, 0.0);

      auto *hypre_jac = form.jac.As<mfem::HypreParMatrix>();
      if (hypre_jac == nullptr)
      {
         throw MachException(
             "getJacobianTranspose (MachNonlinearForm) only supports "
             "Jacobian matrices assembled to a HypreParMatrix!\n");
      }
      hypre_jac->Mult(1.0, form.scratch, 1.0, res_dot);

      auto *hypre_jac_e = form.jac_e.As<mfem::HypreParMatrix>();
      if (hypre_jac_e == nullptr)
      {
         throw MachException(
             "setUpAdjointSystem (MachNonlinearForm) only supports "
             "Jacobian matrices assembled to a HypreParMatrix!\n");
      }
      hypre_jac_e->Mult(1.0, wrt_dot, 1.0, res_dot);
   }
   else if (form.fwd_sens.count(wrt) != 0)
   {
      throw NotImplementedException(
          "not implemented for vector sensitivities (except for state)!\n");

      // /// Integrators added to fwd_sens will reference wrt_dot grid func so
      // /// we update it here
      // std::string wrt_dot_name = wrt + "_dot";
      // auto &wrt_dot_field = form.nf_fields.at(wrt_dot_name);
      // wrt_dot_field.distributeSharedDofs(wrt_dot);

      // /// Integrators added to fwd_sens will also reference the state grid
      // func
      // /// so that must have been distributed before calling this function
      // form.fwd_sens.at(wrt).Assemble();
      // form.scratch.SetSize(res_dot.Size());
      // form.fwd_sens.at(wrt).ParallelAssemble(form.scratch);

      // res_dot += form.scratch;
   }
}

double vectorJacobianProduct(MachNonlinearForm &form,
                             const mfem::Vector &res_bar,
                             const std::string &wrt)
{
   if (form.rev_scalar_sens.count(wrt) != 0)
   {
      /// Integrators added to rev_scalar_sens will reference the adjoint grid
      /// func so we update it here
      auto &adjoint = form.nf_fields.at(form.adjoint_name);
      adjoint.distributeSharedDofs(res_bar);

      /// The state must have previously been distributed before calling this
      /// function
      auto &state = form.nf_fields.at("state").gridFunc();
      return form.rev_scalar_sens.at(wrt).GetGridFunctionEnergy(state);
   }
   return 0.0;
}

void vectorJacobianProduct(MachNonlinearForm &form,
                           const mfem::Vector &res_bar,
                           const std::string &wrt,
                           mfem::Vector &wrt_bar)
{
   if (wrt == "state")
   {
      form.scratch = res_bar;
      const auto &ess_tdof_list = form.getEssentialDofs();
      form.scratch.SetSubVector(ess_tdof_list, 0.0);

      auto *hypre_jac = form.jac.As<mfem::HypreParMatrix>();
      if (hypre_jac == nullptr)
      {
         throw MachException(
             "getJacobianTranspose (MachNonlinearForm) only supports "
             "Jacobian matrices assembled to a HypreParMatrix!\n");
      }
      // hypre_jac->MultTranspose(1.0, res_bar, 1.0, wrt_bar);
      hypre_jac->MultTranspose(1.0, form.scratch, 1.0, wrt_bar);
      // form.scratch.SetSize(res_bar.Size());
      // hypre_jac->MultTranspose(1.0, res_bar, 1.0, form.scratch);
      // form.scratch.SetSubVector(ess_tdof_list, 0.0);

      auto *hypre_jac_e = form.jac_e.As<mfem::HypreParMatrix>();
      if (hypre_jac_e == nullptr)
      {
         throw MachException(
             "setUpAdjointSystem (MachNonlinearForm) only supports "
             "Jacobian matrices assembled to a HypreParMatrix!\n");
      }
      hypre_jac_e->MultTranspose(1.0, res_bar, 1.0, wrt_bar);

      // form.adjoint_jac_trans->Mult(res_bar, wrt_bar);
   }
   else if (form.rev_sens.count(wrt) != 0)
   {
      form.scratch.SetSize(res_bar.Size());
      form.scratch = res_bar;
      const auto &ess_tdof_list = form.getEssentialDofs();
      form.scratch.SetSubVector(ess_tdof_list, 0.0);

      /// Integrators added to rev_sens will reference the adjoint, grid func
      /// so we update it here
      auto &adjoint = form.nf_fields.at(form.adjoint_name);
      adjoint.distributeSharedDofs(form.scratch);

      /// Integrators added to rev_sens will also reference the state grid func,
      /// so that must have been distributed before calling this function
      auto &wrt_rev_sens = form.rev_sens.at(wrt);
      wrt_rev_sens.Assemble();
      form.scratch.SetSize(wrt_bar.Size());
      form.scratch = 0.0;
      wrt_rev_sens.ParallelAssemble(form.scratch);

      // mfem::Array<int> wrt_ess_tdof_list;
      // wrt_rev_sens.ParFESpace()->GetEssentialTrueDofs(form.ess_bdr,
      // wrt_ess_tdof_list); for (int i = 0; i < wrt_ess_tdof_list.Size(); ++i)
      // {
      //    form.scratch(wrt_ess_tdof_list[i]) = 0.0;
      // }
      wrt_bar += form.scratch;
   }
}

}  // namespace mach
