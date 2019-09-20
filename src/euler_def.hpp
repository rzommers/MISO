
template <int dim>
void EulerIntegrator<dim>::calcFluxJacState(const mfem::Vector &dir,
                                            const mfem::Vector &q,
                                            mfem::DenseMatrix &flux_jac)
{
   // declare vectors of active input variables
   std::vector<adouble> dir_a(dir.Size());
   std::vector<adouble> q_a(q.Size());
   // copy data from mfem::Vector
   adept::set_values(dir_a.data(), dir.Size(), dir.GetData());
   adept::set_values(q_a.data(), q.Size(), q.GetData());
   // start recording
   this->stack.new_recording();
   // the depedent variable must be declared after the recording
   std::vector<adouble> flux_a(q.Size());
   mach::calcEulerFlux<adouble, dim>(dir_a.data(), q_a.data(),
                                     flux_a.data());
   // set the independent and dependent variable
   this->stack.independent(q_a.data(), q.Size());
   this->stack.dependent(flux_a.data(), q.Size());
   // calculate the jacobian w.r.t state vaiables
   this->stack.jacobian(flux_jac.GetData());
}

template <int dim>
void EulerIntegrator<dim>::calcFluxJacDir(const mfem::Vector &dir,
                                          const mfem::Vector &q,
                                          mfem::DenseMatrix &flux_jac)
{
   // declare vectors of active input variables
   std::vector<adouble> dir_a(dir.Size());
   std::vector<adouble> q_a(q.Size());
   // copy data from mfem::Vector
   adept::set_values(dir_a.data(), dir.Size(), dir.GetData());
   adept::set_values(q_a.data(), q.Size(), q.GetData());
   // start recording
   this->stack.new_recording();
   // the depedent variable must be declared after the recording
   std::vector<adouble> flux_a(q.Size());
   mach::calcEulerFlux<adouble, dim>(dir_a.data(), q_a.data(),
                                     flux_a.data());
   this->stack.independent(dir_a.data(), dir.Size());
   this->stack.dependent(flux_a.data(), q.Size());
   // calculate the jacobian w.r.t state vaiables
   this->stack.jacobian(flux_jac.GetData());
}

template <int dim>
void IsmailRoeIntegrator<dim>::calcFluxJacStates(int di, const mfem::Vector &qL,
                                                 const mfem::Vector &qR,
                                                 mfem::DenseMatrix &jacL,
                                                 mfem::DenseMatrix &jacR)
{
   // store the full jacobian in jac
   mfem::DenseMatrix jac(dim + 2, 2 * (dim + 2));
   // vector of active input variables
   std::vector<adouble> qL_a(qL.Size());
   std::vector<adouble> qR_a(qR.Size());
   // initialize adouble inputs
   adept::set_values(qL_a.data(), qL.Size(), qL.GetData());
   adept::set_values(qR_a.data(), qR.Size(), qR.GetData());
   // start recording
   this->stack.new_recording();
   // create vector of active output variables
   std::vector<adouble> flux_a(qL.Size());
   // run algorithm
   mach::calcIsmailRoeFlux<adouble, dim>(di, qL_a.data(),
                                         qR_a.data(), flux_a.data());
   // identify independent and dependent variables
   this->stack.independent(qL_a.data(), qL.Size());
   this->stack.independent(qR_a.data(), qR.Size());
   this->stack.dependent(flux_a.data(), qL.Size());
   // compute and store jacobian in jac 
   this->stack.jacobian_reverse(jac.GetData());
   // retrieve the jacobian w.r.t left state
   jacL.CopyCols(jac, 0, dim + 1);
   // retrieve the jacobian w.r.t right state
   jacR.CopyCols(jac, dim + 2, 2 * (dim + 2) - 1);
}

template <int dim>
void SlipWallBC<dim>::calcFluxJacState(const mfem::Vector &x, const mfem::Vector &dir,
                                       const mfem::Vector &q, mfem::DenseMatrix &flux_jac)
{
   // create containers for active double objects for each input
   std::vector<adouble> x_a(x.Size());
   std::vector<adouble> dir_a(dir.Size());
   std::vector<adouble> q_a(q.Size());
   // initialize active double containers with data from inputs
   adept::set_values(x_a.data(), x.Size(), x.GetData());
   adept::set_values(dir_a.data(), dir.Size(), dir.GetData());
   adept::set_values(q_a.data(), q.Size(), q.GetData());
   // start new stack recording
   this->stack.new_recording();
   // create container for active double flux output
   std::vector<adouble> flux_a(q.Size());
   mach::calcSlipWallFlux<adouble, dim>(x_a.data(), dir_a.data(), q_a.data(),
                                        flux_a.data());
   this->stack.independent(q_a.data(), q.Size());
   this->stack.dependent(flux_a.data(), q.Size());
   this->stack.jacobian(flux_jac.GetData());
}

template <int dim>
void SlipWallBC<dim>::calcFluxJacDir(const mfem::Vector &x,
                                     const mfem::Vector &dir,
                                     const mfem::Vector &q,
                                     mfem::DenseMatrix &flux_jac)
{
   // create containers for active double objects for each input
   std::vector<adouble> x_a(x.Size());
   std::vector<adouble> dir_a(dir.Size());
   std::vector<adouble> q_a(q.Size());
   // initialize active double containers with data from inputs
   adept::set_values(x_a.data(), x.Size(), x.GetData());
   adept::set_values(dir_a.data(), dir.Size(), dir.GetData());
   adept::set_values(q_a.data(), q.Size(), q.GetData());
   // start new stack recording
   this->stack.new_recording();
   // create container for active double flux output
   std::vector<adouble> flux_a(q.Size());
   mach::calcSlipWallFlux<adouble, dim>(x_a.data(), dir_a.data(), q_a.data(),
                                        flux_a.data());
   this->stack.independent(dir_a.data(), dir.Size());
   this->stack.dependent(flux_a.data(), q.Size());
   this->stack.jacobian(flux_jac.GetData());
}

template <int dim>
void EntStableLPSIntegrator<dim>::calcSpectralRadiusJacState(const mfem::Vector &dir,
                                                  const mfem::Vector &q,
                                                  mfem::DenseMatrix &Jac)
{
   // create containers for active double objects for each input
   std::vector<adouble> dir_a(dir.Size());
   std::vector<adouble> q_a(q.Size());
   // initialize active double containers with data from inputs
   adept::set_values(dir_a.data(), dir.Size(), dir.GetData());
   adept::set_values(q_a.data(), q.Size(), q.GetData());
   // start new stack recording
   this->stack.new_recording();
   // create container for active double spectral radius input
   adouble sr = calcSpectralRadius<adouble, dim>(dir_a.data(), q_a.data());
   this->stack.independent(q_a.data(), q.Size());
   this->stack.dependent(sr);
   this->stack.jacobian(Jac.GetData());
}

template <int dim>
void EntStableLPSIntegrator<dim>::calcSpectralRadiusJacDir(const mfem::Vector &dir,
					                                 const mfem::Vector &q,
					                                 mfem::DenseMatrix &Jac)
{
   // create containers for active double objects for each input
   std::vector<adouble> dir_a(dir.Size());
   std::vector<adouble> q_a(q.Size());
   // initialize active double containers with data from inputs
   adept::set_values(dir_a.data(), dir.Size(), dir.GetData());
   adept::set_values(q_a.data(), q.Size(), q.GetData());
   // start new stack recording
   this->stack.new_recording();
   // create container for active double spectral radius input
   adouble sr = calcSpectralRadius<adouble, dim>(dir_a.data(), q_a.data());
   this->stack.independent(dir_a.data(), dir.Size());
   this->stack.dependent(sr);
   this->stack.jacobian(Jac.GetData());
}