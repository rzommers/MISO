/// Functions related to Euler equations

#ifndef MACH_EULER_FLUXES
#define MACH_EULER_FLUXES

#include <algorithm>  // std::max

#include "adept.h"

#include "utils.hpp"
#include "potential_fluxes.hpp"
using adept::adouble;

namespace mach
{
// /// For constants related to the Euler equations
// namespace euler
// {
// /// gas constant
// const double R = 287;
// /// heat capcity ratio for air
// const double gamma = 1.4;
// /// ratio minus one
// const double gami = gamma - 1.0;
// }  // namespace euler

/// Pressure based on the ideal gas law equation of state
/// \param[in] q - the conservative variables
/// \tparam xdouble - either double or adouble
/// \tparam dim - number of physical dimensions
template <typename xdouble, int dim>
inline xdouble pressure(const xdouble *q)
{
   return euler::gami *
          (q[dim + 1] - 0.5 * dot<xdouble, dim>(q + 1, q + 1) / q[0]);
}
/// Pressure based on the ideal gas law equation of state
/// \param[in] q - the conservative variables
/// \tparam xdouble - either double or adouble
/// \tparam dim - number of physical dimensions
template <typename xdouble, int dim>
inline xdouble pressureBernoulli(const xdouble *q)
{
   return euler::gami *
          (q[dim + 1] - 0.5 * dot<xdouble, dim>(q + 1, q + 1) / q[0]);
}
// Check that the state is physical - enabled in debug mode
/// \param[in] q - the conservative variables
/// \tparam xdouble - either double or adouble
/// \tparam dim - number of physical dimensions
template <typename xdouble, int dim>
bool StateIsPhysical(const xdouble *q)
{
   using namespace std;
   xdouble rho = q[0];
   xdouble u[dim];
   for (int i = 0; i < dim; ++i)
   {
      u[i] = q[i + 1] / q[0];
   }
   xdouble energy = q[dim + 1];

   if (rho < 0)
   {
      cout << "Negative density: ";
      for (int i = 0; i < dim + 2; ++i)
      {
         cout << q[i] << " ";
      }
      cout << endl;
      return false;
   }
   if (energy <= 0)
   {
      cout << "Negative energy: ";
      for (int i = 0; i < dim + 2; ++i)
      {
         cout << q[i] << " ";
      }
      cout << endl;
      return false;
   }

   auto press = pressure<xdouble, dim>(q);

   if (press <= 0)
   {
      cout << "Negative pressure: " << press << ", state: ";
      for (int i = 0; i < dim + 2; ++i)
      {
         cout << q[i] << " ";
      }
      cout << endl;
      return false;
   }
   return true;
}
/// Convert conservative variables `q` to entropy variables `w`
/// \param[in] q - conservative variables that we want to convert from
/// \param[out] w - entropy variables we want to convert to
/// \tparam xdouble - typically `double` or `adept::adouble`
/// \tparam dim - number of spatial dimensions (1, 2, or 3)
template <typename xdouble, int dim>
void calcEntropyVars(const xdouble *q, xdouble *w)
{
   xdouble u[dim];
   for (int i = 0; i < dim; ++i)
   {
      u[i] = q[i + 1] / q[0];
   }
   auto p = pressure<xdouble, dim>(q);
   xdouble s = log(p / pow(q[0], euler::gamma));
   xdouble fac = 1.0 / p;
   w[0] = (euler::gamma - s) / euler::gami -
          0.5 * dot<xdouble, dim>(u, u) * fac * q[0];
   for (int i = 0; i < dim; ++i)
   {
      w[i + 1] = q[i + 1] * fac;
   }
   w[dim + 1] = -q[0] * fac;
}

/// Convert entropy variables `w` to conservative variables `q`
/// \param[in] w - entropy variables we want to convert from
/// \param[out] q - conservative variables that we want to convert to
/// \tparam xdouble - typically `double` or `adept::adouble`
/// \tparam dim - number of spatial dimensions (1, 2, or 3)
template <typename xdouble, int dim>
void calcConservativeVars(const xdouble *w, xdouble *q)
{
   xdouble u[dim];
   xdouble Vel2 = 0.0;
   for (int i = 0; i < dim; ++i)
   {
      u[i] = -w[i + 1] / w[dim + 1];
      Vel2 += u[i] * u[i];
   }
   xdouble s = euler::gamma + euler::gami * (0.5 * Vel2 * w[dim + 1] - w[0]);
   q[0] = pow(-exp(-s) / w[dim + 1], 1.0 / euler::gami);
   for (int i = 0; i < dim; ++i)
   {
      q[i + 1] = q[0] * u[i];
   }
   xdouble p = -q[0] / w[dim + 1];
   q[dim + 1] = p / euler::gami + 0.5 * q[0] * Vel2;
}

/// Mathematical entropy function rho*s/(gamma-1), where s = ln(p/rho^gamma)
/// \param[in] q - state variables (either conservative or entropy variables)
/// \tparam xdouble - either double or adouble
/// \tparam dim - number of physical dimensions
/// \tparam entvar - if true q = conservative vars, if false q = entropy vars
template <typename xdouble, int dim, bool entvar = false>
inline xdouble entropy(const xdouble *q)
{
   if (entvar)
   {
      auto Vel2 = dot<xdouble, dim>(q + 1, q + 1);  // Vel2*rho^2/p^2
      double s =
          -euler::gamma + euler::gami * (q[0] - 0.5 * Vel2 / q[dim + 1]);  // -s
      double rho = pow(-exp(s) / q[dim + 1], 1.0 / euler::gami);
      return rho * s / euler::gami;
   }
   else
   {
      return -q[0] * log(pressure<xdouble, dim>(q) / pow(q[0], euler::gamma)) /
             euler::gami;
   }
}

/// Euler flux function in a given (scaled) direction
/// \param[in] dir - direction in which the flux is desired
/// \param[in] q - conservative variables
/// \param[out] flux - fluxes in the direction `dir`
/// \tparam xdouble - typically `double` or `adept::adouble`
/// \tparam dim - number of spatial dimensions (1, 2, or 3)
template <typename xdouble, int dim>
void calcEulerFlux(const xdouble *dir, const xdouble *q, xdouble *flux)
{
   if (!StateIsPhysical<xdouble, dim>(q))
   {
      throw MachException("State is not physical!\n");
   }
   auto press = pressure<xdouble, dim>(q);
   auto U = dot<xdouble, dim>(q + 1, dir);
   flux[0] = U;
   U /= q[0];
   for (int i = 0; i < dim; ++i)
   {
      flux[i + 1] = q[i + 1] * U + dir[i] * press;
   }
   flux[dim + 1] = (q[dim + 1] + press) * U;
}
/// Roe flux function in direction `dir`
/// \param[in] dir - vector direction in which flux is wanted
/// \param[in] qL - conservative variables at "left" state
/// \param[in] qR - conservative variables at "right" state
/// \param[out] flux - fluxes in the direction `dir`
/// \tparam xdouble - typically `double` or `adept::adouble`
/// \tparam dim - number of spatial dimensions (1, 2, or 3)
template <typename xdouble, int dim>
void calcRoeFaceFlux(const xdouble *dir,
                     const xdouble *qL,
                     const xdouble *qR,
                     xdouble *flux)
{
   using adept::max;
   using std::max;

   // Define some constants
   const xdouble sat_Vn = 0.025;  // 0.25 (with shocks)
   const xdouble sat_Vl = 0.025;

   // Define the Roe-average state
   const xdouble sqL = sqrt(qL[0]);
   const xdouble sqR = sqrt(qR[0]);
   const xdouble facL = 1.0 / qL[0];
   const xdouble facR = 1.0 / qR[0];
   const xdouble fac = 1.0 / (sqL + sqR);
   xdouble u[dim];  // should pass in another work array?
   for (int i = 0; i < dim; ++i)
   {
      u[i] = (sqL * qL[i + 1] * facL + sqR * qR[i + 1] * facR) * fac;
   }
   const xdouble phi = 0.5 * dot<xdouble, dim>(u, u);
   const xdouble Un = dot<xdouble, dim>(dir, u);
   const xdouble HL =
       (euler::gamma * qL[dim + 1] -
        0.5 * euler::gami * dot<xdouble, dim>(qL + 1, qL + 1) * facL) *
       facL;
   const xdouble HR =
       (euler::gamma * qR[dim + 1] -
        0.5 * euler::gami * dot<xdouble, dim>(qR + 1, qR + 1) * facR) *
       facR;
   const xdouble H = (sqL * HL + sqR * HR) * fac;
   const xdouble a = sqrt(euler::gami * (H - phi));
   const xdouble dA = sqrt(dot<xdouble, dim>(dir, dir));

   // Define the wave speeds
   const xdouble rhoA = fabs(Un) + dA * a;
   const xdouble lambda1 = 0.5 * max(fabs(Un + dA * a), sat_Vn * rhoA);
   const xdouble lambda2 = 0.5 * max(fabs(Un - dA * a), sat_Vn * rhoA);
   const xdouble lambda3 = 0.5 * max(fabs(Un), sat_Vl * rhoA);

   // start flux computation by averaging the Euler flux
   xdouble dq[dim + 2];
   calcEulerFlux<xdouble, dim>(dir, qL, flux);
   calcEulerFlux<xdouble, dim>(dir, qR, dq);
   for (int i = 0; i < dim + 2; ++i)
   {
      flux[i] = 0.5 * (flux[i] + dq[i]);
      dq[i] = qL[i] - qR[i];
      flux[i] += lambda3 * dq[i];  // diagonal matrix multiply
   }

   // some scalars needed for E1*dq, E2*dq, E3*dq, and E4*dq
   xdouble tmp1 = 0.5 * (lambda1 + lambda2) - lambda3;
   xdouble E1dq_fac = tmp1 * euler::gami / (a * a);
   xdouble E2dq_fac = tmp1 / (dA * dA);
   xdouble E34dq_fac = 0.5 * (lambda1 - lambda2) / (dA * a);

   // get E1*dq + E4*dq and add to flux
   xdouble Edq = phi * dq[0] + dq[dim + 1] - dot<xdouble, dim>(u, dq + 1);
   flux[0] += E1dq_fac * Edq;
   for (int i = 0; i < dim; ++i)
   {
      flux[i + 1] += Edq * (E1dq_fac * u[i] + euler::gami * E34dq_fac * dir[i]);
   }
   flux[dim + 1] += Edq * (E1dq_fac * H + euler::gami * E34dq_fac * Un);

   // get E2*dq + E3*dq and add to flux
   Edq = -Un * dq[0] + dot<xdouble, dim>(dir, dq + 1);
   flux[0] += E34dq_fac * Edq;
   for (int i = 0; i < dim; ++i)
   {
      flux[i + 1] += Edq * (E2dq_fac * dir[i] + E34dq_fac * u[i]);
   }
   flux[dim + 1] += Edq * (E2dq_fac * Un + E34dq_fac * H);
}
#if 0
/// Roe flux function in direction `dir`
/// \param[in] dir - vector direction in which flux is wanted
/// \param[in] qL - conservative variables at "left" state
/// \param[in] qR - conservative variables at "right" state
/// \param[out] flux - fluxes in the direction `dir`
/// \tparam xdouble - typically `double` or `adept::adouble`
/// \tparam dim - number of spatial dimensions (1, 2, or 3)
template <typename xdouble, int dim>
void calcRoeFaceFlux(const xdouble *dir,
                     const xdouble *qL,
                     const xdouble *qR,
                     xdouble *flux)
{
   using adept::max;
   using std::max;

   // Define some constants
   const xdouble sat_Vn = 0.025;
   const xdouble sat_Vl = 0.025;

   // Define the Roe-average state
   const xdouble sqL = sqrt(qL[0]);
   const xdouble sqR = sqrt(qR[0]);
   const xdouble facL = 1.0 / qL[0];
   const xdouble facR = 1.0 / qR[0];
   const xdouble fac = 1.0 / (sqL + sqR);
   xdouble u[dim];  // should pass in another work array?
   for (int i = 0; i < dim; ++i)
   {
      u[i] = (sqL * qL[i + 1] * facL + sqR * qR[i + 1] * facR) * fac;
   }
   const xdouble phi = 0.5 * dot<xdouble, dim>(u, u);
   const xdouble Un = dot<xdouble, dim>(dir, u);
   const xdouble HL =
       (euler::gamma * qL[dim + 1] -
        0.5 * euler::gami * dot<xdouble, dim>(qL + 1, qL + 1) * facL) *
       facL;
   const xdouble HR =
       (euler::gamma * qR[dim + 1] -
        0.5 * euler::gami * dot<xdouble, dim>(qR + 1, qR + 1) * facR) *
       facR;
   const xdouble H = (sqL * HL + sqR * HR) * fac;
   const xdouble a = sqrt(euler::gami * (H - phi));
   const xdouble dA = sqrt(dot<xdouble, dim>(dir, dir));

   // Define the wave speeds
   const xdouble rhoA = fabs(Un) + dA * a;
   const xdouble lambda1 = 0.5 * max(fabs(Un + dA * a), sat_Vn * rhoA);
   const xdouble lambda2 = 0.5 * max(fabs(Un - dA * a), sat_Vn * rhoA);
   const xdouble lambda3 = 0.5 * max(fabs(Un), sat_Vl * rhoA);

   // start flux computation by averaging the Euler flux
   xdouble dq[dim + 2];
   calcEulerFlux<xdouble, dim>(dir, qL, flux);
   calcEulerFlux<xdouble, dim>(dir, qR, dq);
   for (int i = 0; i < dim + 2; ++i)
   {
      flux[i] = 0.5 * (flux[i] + dq[i]);
      dq[i] = qL[i] - qR[i];
      flux[i] += lambda3 * dq[i];  // diagonal matrix multiply
   }

   // some scalars needed for E1*dq, E2*dq, E3*dq, and E4*dq
   xdouble tmp1 = 0.5 * (lambda1 + lambda2) - lambda3;
   xdouble E1dq_fac = tmp1 * euler::gami / (a * a);
   xdouble E2dq_fac = tmp1 / (dA * dA);
   xdouble E34dq_fac = 0.5 * (lambda1 - lambda2) / (dA * a);

   // get E1*dq + E4*dq and add to flux
   xdouble Edq = phi * dq[0] + dq[dim + 1] - dot<xdouble, dim>(u, dq + 1);
   flux[0] += E1dq_fac * Edq;
   for (int i = 0; i < dim; ++i)
   {
      flux[i + 1] += Edq * (E1dq_fac * u[i] + euler::gami * E34dq_fac * dir[i]);
   }
   flux[dim + 1] += Edq * (E1dq_fac * H + euler::gami * E34dq_fac * Un);

   // get E2*dq + E3*dq and add to flux
   Edq = -Un * dq[0] + dot<xdouble, dim>(dir, dq + 1);
   flux[0] += E34dq_fac * Edq;
   for (int i = 0; i < dim; ++i)
   {
      flux[i + 1] += Edq * (E2dq_fac * dir[i] + E34dq_fac * u[i]);
   }
   flux[dim + 1] += Edq * (E2dq_fac * Un + E34dq_fac * H);
}
#endif
/// Log-average function used in the Ismail-Roe flux function
/// \param[in] aL - nominal left state variable to average
/// \param[in] aR - nominal right state variable to average
/// \returns the logarithmic average of `aL` and `aR`.
/// \tparam xdouble - typically `double` or `adept::adouble`
template <typename xdouble>
xdouble logavg(const xdouble &aL, const xdouble &aR)
{
   xdouble xi = aL / aR;
   xdouble f = (xi - 1) / (xi + 1);
   xdouble u = f * f;
   double eps = 1.0e-3;
   xdouble F;
   if (u < eps)
   {
      F = 1.0 + u * (1. / 3. + u * (1. / 5. + u * (1. / 7. + u / 9.)));
   }
   else
   {
      F = (log(xi) / 2.0) / f;
   }
   return (aL + aR) / (2.0 * F);
}

/// Ismail-Roe two-point (dyadic) entropy conservative flux function
/// \param[in] di - physical coordinate direction in which flux is wanted
/// \param[in] qL - conservative variables at "left" state
/// \param[in] qR - conservative variables at "right" state
/// \param[out] flux - fluxes in the direction `di`
/// \tparam xdouble - typically `double` or `adept::adouble`
/// \tparam dim - number of spatial dimensions (1, 2, or 3)
template <typename xdouble, int dim>
void calcIsmailRoeFlux(int di,
                       const xdouble *qL,
                       const xdouble *qR,
                       xdouble *flux)
{
   auto pL = pressure<xdouble, dim>(qL);
   auto pR = pressure<xdouble, dim>(qR);
   xdouble zL[dim + 2];
   xdouble zR[dim + 2];
   zL[0] = sqrt(qL[0] / pL);
   zR[0] = sqrt(qR[0] / pR);
   for (int i = 0; i < dim; ++i)
   {
      zL[i + 1] = zL[0] * qL[i + 1] / qL[0];
      zR[i + 1] = zR[0] * qR[i + 1] / qR[0];
   }
   zL[dim + 1] = sqrt(qL[0] * pL);
   zR[dim + 1] = sqrt(qR[0] * pR);

   xdouble rho_hat = 0.5 * (zL[0] + zR[0]) * logavg(zL[dim + 1], zR[dim + 1]);
   xdouble U = (zL[di + 1] + zR[di + 1]) / (zL[0] + zR[0]);
   xdouble p1_hat = (zL[dim + 1] + zR[dim + 1]) / (zL[0] + zR[0]);
   xdouble p2_hat =
       ((euler::gamma + 1.0) * logavg(zL[dim + 1], zR[dim + 1]) /
            logavg(zL[0], zR[0]) +
        (euler::gami) * (zL[dim + 1] + zR[dim + 1]) / (zL[0] + zR[0])) /
       (2.0 * euler::gamma);
   xdouble h_hat = euler::gamma * p2_hat / (rho_hat * euler::gami);

   flux[0] = rho_hat * U;
   for (int i = 0; i < dim; ++i)
   {
      flux[i + 1] = (zL[i + 1] + zR[i + 1]) / (zL[0] + zR[0]);  // u_hat
      h_hat += 0.5 * flux[i + 1] * flux[i + 1];
      flux[i + 1] *= rho_hat * U;
   }
   flux[di + 1] += p1_hat;
   flux[dim + 1] = rho_hat * h_hat * U;
}

/// Ismail-Roe flux function evaluated using entropy variables
/// \param[in] di - physical coordinate direction in which flux is wanted
/// \param[in] wL - entropy variables at "left" state
/// \param[in] wR - entropy variables at "right" state
/// \param[out] flux - fluxes in the direction `di`
/// \tparam xdouble - typically `double` or `adept::adouble`
/// \tparam dim - number of spatial dimensions (1, 2, or 3)
template <typename xdouble, int dim>
void calcIsmailRoeFluxUsingEntVars(int di,
                                   const xdouble *wL,
                                   const xdouble *wR,
                                   xdouble *flux)
{
   xdouble zL[dim + 2];
   xdouble zR[dim + 2];
   zL[0] = sqrt(-wL[dim + 1]);
   zR[0] = sqrt(-wR[dim + 1]);
   xdouble sL = 0.0;
   xdouble sR = 0.0;
   for (int i = 0; i < dim; ++i)
   {
      zL[i + 1] = -wL[i + 1] * zL[0] / wL[dim + 1];
      sL += wL[i + 1] * wL[i + 1];
      zR[i + 1] = -wR[i + 1] * zR[0] / wR[dim + 1];
      sR += wR[i + 1] * wR[i + 1];
   }
   sL = euler::gamma +
        euler::gami * (0.5 * sL / wL[dim + 1] - wL[0]);  // physical ent.
   zL[dim + 1] = pow(-exp(-sL) / wL[dim + 1], 1.0 / euler::gami) / zL[0];
   sR = euler::gamma +
        euler::gami * (0.5 * sR / wR[dim + 1] - wR[0]);  // physical ent.
   zR[dim + 1] = pow(-exp(-sR) / wR[dim + 1], 1.0 / euler::gami) / zR[0];

   xdouble rho_hat = 0.5 * (zL[0] + zR[0]) * logavg(zL[dim + 1], zR[dim + 1]);
   xdouble U = (zL[di + 1] + zR[di + 1]) / (zL[0] + zR[0]);
   xdouble p1_hat = (zL[dim + 1] + zR[dim + 1]) / (zL[0] + zR[0]);
   xdouble p2_hat =
       ((euler::gamma + 1.0) * logavg(zL[dim + 1], zR[dim + 1]) /
            logavg(zL[0], zR[0]) +
        (euler::gami) * (zL[dim + 1] + zR[dim + 1]) / (zL[0] + zR[0])) /
       (2.0 * euler::gamma);
   xdouble h_hat = euler::gamma * p2_hat / (rho_hat * euler::gami);

   flux[0] = rho_hat * U;
   for (int i = 0; i < dim; ++i)
   {
      flux[i + 1] = (zL[i + 1] + zR[i + 1]) / (zL[0] + zR[0]);  // u_hat
      h_hat += 0.5 * flux[i + 1] * flux[i + 1];
      flux[i + 1] *= rho_hat * U;
   }
   flux[di + 1] += p1_hat;
   flux[dim + 1] = rho_hat * h_hat * U;
}

/// Ismail-Roe entropy conservative flux function in direction `dir`
/// \param[in] dir - vector direction in which flux is wanted
/// \param[in] qL - conservative variables at "left" state
/// \param[in] qR - conservative variables at "right" state
/// \param[out] flux - fluxes in the direction `dir`
/// \tparam xdouble - typically `double` or `adept::adouble`
/// \tparam dim - number of spatial dimensions (1, 2, or 3)
template <typename xdouble, int dim>
void calcIsmailRoeFaceFlux(const xdouble *dir,
                           const xdouble *qL,
                           const xdouble *qR,
                           xdouble *flux)
{
   xdouble pL = pressure<xdouble, dim>(qL);
   xdouble pR = pressure<xdouble, dim>(qR);
   xdouble zL[dim + 2];
   xdouble zR[dim + 2];
   zL[0] = sqrt(qL[0] / pL);
   zR[0] = sqrt(qR[0] / pR);
   for (int i = 0; i < dim; ++i)
   {
      zL[i + 1] = zL[0] * qL[i + 1] / qL[0];
      zR[i + 1] = zR[0] * qR[i + 1] / qR[0];
   }
   zL[dim + 1] = sqrt(qL[0] * pL);
   zR[dim + 1] = sqrt(qR[0] * pR);

   xdouble rho_hat = 0.5 * (zL[0] + zR[0]) * logavg(zL[dim + 1], zR[dim + 1]);

   xdouble U = 0.0;
   for (int i = 0; i < dim; ++i)
   {
      U += (zL[0] * qL[i + 1] / qL[0] + zR[0] * qR[i + 1] / qR[0]) * dir[i] /
           (zL[0] + zR[0]);
   }
   xdouble p1_hat = (zL[dim + 1] + zR[dim + 1]) / (zL[0] + zR[0]);
   xdouble p2_hat =
       ((euler::gamma + 1.0) * logavg(zL[dim + 1], zR[dim + 1]) /
            logavg(zL[0], zR[0]) +
        (euler::gami) * (zL[dim + 1] + zR[dim + 1]) / (zL[0] + zR[0])) /
       (2.0 * euler::gamma);
   xdouble h_hat = euler::gamma * p2_hat / (rho_hat * euler::gami);

   flux[0] = rho_hat * U;
   for (int i = 0; i < dim; ++i)
   {
      flux[i + 1] = (zL[i + 1] + zR[i + 1]) / (zL[0] + zR[0]);  // u_hat
      h_hat += 0.5 * flux[i + 1] * flux[i + 1];
      flux[i + 1] *= rho_hat * U;
      flux[i + 1] += p1_hat * dir[i];
   }
   flux[dim + 1] = rho_hat * h_hat * U;
}

/// Ismail-Roe entropy conservative flux function in direction `dir`
/// \param[in] dir - vector direction in which flux is wanted
/// \param[in] qL - entropy variables at "left" state
/// \param[in] qR - entropy variables at "right" state
/// \param[out] flux - fluxes in the direction `dir`
/// \tparam xdouble - typically `double` or `adept::adouble`
/// \tparam dim - number of spatial dimensions (1, 2, or 3)
template <typename xdouble, int dim>
void calcIsmailRoeFaceFluxUsingEntVars(const xdouble *dir,
                                       const xdouble *wL,
                                       const xdouble *wR,
                                       xdouble *flux)
{
   xdouble zL[dim + 2];
   xdouble zR[dim + 2];
   zL[0] = sqrt(-wL[dim + 1]);
   zR[0] = sqrt(-wR[dim + 1]);
   xdouble sL = 0.0;
   xdouble sR = 0.0;
   for (int i = 0; i < dim; ++i)
   {
      zL[i + 1] = -wL[i + 1] * zL[0] / wL[dim + 1];
      sL += wL[i + 1] * wL[i + 1];
      zR[i + 1] = -wR[i + 1] * zR[0] / wR[dim + 1];
      sR += wR[i + 1] * wR[i + 1];
   }
   sL = euler::gamma +
        euler::gami * (0.5 * sL / wL[dim + 1] - wL[0]);  // physical ent.
   zL[dim + 1] = pow(-exp(-sL) / wL[dim + 1], 1.0 / euler::gami) / zL[0];
   sR = euler::gamma +
        euler::gami * (0.5 * sR / wR[dim + 1] - wR[0]);  // physical ent.
   zR[dim + 1] = pow(-exp(-sR) / wR[dim + 1], 1.0 / euler::gami) / zR[0];

   xdouble rho_hat = 0.5 * (zL[0] + zR[0]) * logavg(zL[dim + 1], zR[dim + 1]);
   xdouble U = 0.0;
   for (int i = 0; i < dim; ++i)
   {
      U += (zL[i + 1] + zR[i + 1]) * dir[i] / (zL[0] + zR[0]);
   }
   xdouble p1_hat = (zL[dim + 1] + zR[dim + 1]) / (zL[0] + zR[0]);
   xdouble p2_hat =
       ((euler::gamma + 1.0) * logavg(zL[dim + 1], zR[dim + 1]) /
            logavg(zL[0], zR[0]) +
        (euler::gami) * (zL[dim + 1] + zR[dim + 1]) / (zL[0] + zR[0])) /
       (2.0 * euler::gamma);
   xdouble h_hat = euler::gamma * p2_hat / (rho_hat * euler::gami);

   flux[0] = rho_hat * U;
   for (int i = 0; i < dim; ++i)
   {
      flux[i + 1] = (zL[i + 1] + zR[i + 1]) / (zL[0] + zR[0]);  // u_hat
      h_hat += 0.5 * flux[i + 1] * flux[i + 1];
      flux[i + 1] *= rho_hat * U;
      flux[i + 1] += p1_hat * dir[i];
   }
   flux[dim + 1] = rho_hat * h_hat * U;
}

/// The spectral radius of flux Jacobian in direction `dir` w.r.t. conservative
/// \param[in] dir - desired direction of flux Jacobian
/// \param[in] u - state variables used to evaluate Jacobian
/// \returns absolute value of the largest eigenvalue of the Jacobian
/// \tparam xdouble - typically `double` or `adept::adouble`
/// \tparam dim - number of spatial dimensions (1, 2, or 3)
/// \tparam entvar - if true u = conservative vars, if false u = entropy vars
template <typename xdouble, int dim, bool entvar = false>
xdouble calcSpectralRadius(const xdouble *dir, const xdouble *u)
{
   xdouble q[dim + 2];
   if (entvar)
   {
      calcConservativeVars<xdouble, dim>(u, q);
   }
   else
   {
      for (int i = 0; i < dim + 2; ++i)
      {
         q[i] = u[i];
      }
   }
   auto press = pressure<xdouble, dim>(q);
   xdouble sndsp = sqrt(euler::gamma * press / q[0]);
   // U = u*dir[0] + v*dir[1] + ...
   xdouble U = dot<xdouble, dim>(q + 1, dir) / q[0];
   // xdouble U = dot<xdouble, dim>(q + 1, q + 1) / q[0];
   // xdouble vel = sqrt(U/q[0]);
   xdouble dir_norm = sqrt(dot<xdouble, dim>(dir, dir));
   return fabs(U) + sndsp * dir_norm;
   // return vel + sndsp;
}

// TODO: How should we return matrices, particularly when they will be
// differentiated?
template <typename xdouble, int dim>
void calcdQdWProduct(const xdouble *q, const xdouble *vec, xdouble *dqdw_vec)
{
   auto p = pressure<xdouble, dim>(q);
   xdouble rho_inv = 1.0 / q[0];
   xdouble h = (q[dim + 1] + p) * rho_inv;   // scaled version of h
   xdouble a2 = euler::gamma * p * rho_inv;  // square of speed of sound

   // first row of dq/dw times vec
   dqdw_vec[0] = 0.0;
   for (int i = 0; i < dim + 2; ++i)
   {
      dqdw_vec[0] += q[i] * vec[i];
   }

   // second through dim-1 rows of dq/dw times vec
   for (int j = 0; j < dim; ++j)
   {
      dqdw_vec[j + 1] = 0.0;
      xdouble u = q[j + 1] * rho_inv;
      for (int i = 0; i < dim + 2; ++i)
      {
         dqdw_vec[j + 1] += u * q[i] * vec[i];
      }
      dqdw_vec[j + 1] += p * vec[j + 1];
      dqdw_vec[j + 1] += p * u * vec[dim + 1];
   }

   // dim-th row of dq/dw times vec
   dqdw_vec[dim + 1] = q[dim + 1] * vec[0];
   for (int i = 0; i < dim; ++i)
   {
      dqdw_vec[dim + 1] += q[i + 1] * h * vec[i + 1];
   }
   dqdw_vec[dim + 1] += (q[0] * h * h - a2 * p / euler::gami) * vec[dim + 1];
}

/// Applies the matrix `dQ/dW` to `vec`, and scales by the avg. spectral radius
/// \param[in] adjJ - the adjugate of the mapping Jacobian
/// \param[in] q - cons. variable at which `dQ/dW` and radius are to be
/// evaluated \param[in] vec - the vector being multiplied
/// \param[out] mat_vec -
/// the result of the operation \warning adjJ must be supplied transposed from
/// its `mfem` storage format, so we can use pointer arithmetic to access its
/// rows.
template <typename xdouble, int dim>
void applyLPSScaling(const xdouble *adjJ,
                     const xdouble *q,
                     const xdouble *vec,
                     xdouble *mat_vec)
{
   // first, get the average spectral radii
   xdouble spect = 0.0;
   for (int i = 0; i < dim; ++i)
   {
      spect += calcSpectralRadius<xdouble, dim>(adjJ + i * dim, q);
   }
   spect /= static_cast<xdouble>(dim);
   calcdQdWProduct<xdouble, dim>(q, vec, mat_vec);
   for (int i = 0; i < dim + 2; ++i)
   {
      mat_vec[i] *= spect;
   }
}

/// Lax-Friedrichs flux function
/// \param[in] dir - vector direction in which flux is wanted
/// \param[in] diss_coeff - scales the dissipation (must be non-negative!)
/// \param[in] qL - conservative variables at "left" state
/// \param[in] qR - conservative variables at "right" state
/// \param[out] flux - fluxes in the direction `di`
/// \tparam double - typically `double` or `adept::adouble`
/// \tparam dim - number of spatial dimensions (1, 2, or 3)
template <typename xdouble, int dim>
void calcLaxFriedrichsFlux(const xdouble *dir,
                           xdouble diss_coeff,
                           const xdouble *qL,
                           const xdouble *qR,
                           xdouble *flux)
{
   xdouble fluxL[dim + 2];
   xdouble fluxR[dim + 2];
   xdouble q_ave[dim + 2];
   xdouble q_diff[dim + 2];
   if (!StateIsPhysical<xdouble, dim>(qL))
   {
      throw MachException("qL is not physical in LF!\n");
   }
   if (!StateIsPhysical<xdouble, dim>(qR))
   {
      throw MachException("qR is not physical in LF!\n");
   }
   calcEulerFlux<xdouble, dim>(dir, qL, fluxL);
   calcEulerFlux<xdouble, dim>(dir, qR, fluxR);
   for (int i = 0; i < dim + 2; i++)
   {
      q_ave[i] = 0.5 * (qL[i] + qR[i]);
      q_diff[i] = -qR[i] + qL[i];
   }
   xdouble lambda = diss_coeff * calcSpectralRadius<xdouble, dim>(dir, q_ave);

   for (int k = 0; k < dim + 2; ++k)
   {
      flux[k] = fluxL[k] + fluxR[k] + (lambda * q_diff[k]);
      flux[k] *= 0.5;
   }
}
/// Applies the matrix `dQ/dW` to `vec`, and scales by the avg. spectral radius
/// \param[in] adjJ - the adjugate of the mapping Jacobian
/// \param[in] w - ent. variables at which `dQ/dW` and radius are to be
/// evaluated \param[in] vec - the vector being multiplied \param[out] mat_vec -
/// the result of the operation \warning adjJ must be supplied transposed from
/// its `mfem` storage format, so we can use pointer arithmetic to access its
/// rows. \todo This converts w to conservative variables, which is inefficient
template <typename xdouble, int dim>
void applyLPSScalingUsingEntVars(const xdouble *adjJ,
                                 const xdouble *w,
                                 const xdouble *vec,
                                 xdouble *mat_vec)
{
   xdouble q[dim + 2];
   calcConservativeVars<xdouble, dim>(w, q);
   applyLPSScaling<xdouble, dim>(adjJ, q, vec, mat_vec);
}

/// Boundary flux that uses characteristics to determine which state to use
/// \param[in] dir - direction in which the flux is desired
/// \param[in] qbnd - boundary values of the conservative variables
/// \param[in] q - interior domain values of the conservative variables
/// \param[in] work - a work vector of size `dim+2`
/// \param[out] flux - fluxes in the direction `dir`
/// \tparam xdouble - typically `double` or `adept::adouble`
/// \tparam dim - number of spatial dimensions (1, 2, or 3)
/// \note the "flux Jacobian" is computed using `qbnd`, so the boundary values
/// define what is inflow and what is outflow.
template <typename xdouble, int dim>
void calcBoundaryFlux(const xdouble *dir,
                      const xdouble *qbnd,
                      const xdouble *q,
                      xdouble *work,
                      xdouble *flux)
{
   using adept::max;
   using std::max;

   // Define some constants
   const xdouble sat_Vn = 0.0;  // 0.025
   const xdouble sat_Vl = 0.0;  // 0.025

   // Define some constants used to construct the "Jacobian"
   const xdouble dA = sqrt(dot<xdouble, dim>(dir, dir));
   const xdouble fac = 1.0 / qbnd[0];
   const xdouble phi = 0.5 * dot<xdouble, dim>(qbnd + 1, qbnd + 1) * fac * fac;
   const xdouble H = euler::gamma * qbnd[dim + 1] * fac - euler::gami * phi;
   const xdouble a = sqrt(euler::gami * (H - phi));
   const xdouble Un = dot<xdouble, dim>(qbnd + 1, dir) * fac;
   xdouble lambda1 = Un + dA * a;
   xdouble lambda2 = Un - dA * a;
   xdouble lambda3 = Un;
   const xdouble rhoA = fabs(Un) + dA * a;
   lambda1 = 0.5 * (max(fabs(lambda1), sat_Vn * rhoA) - lambda1);
   lambda2 = 0.5 * (max(fabs(lambda2), sat_Vn * rhoA) - lambda2);
   lambda3 = 0.5 * (max(fabs(lambda3), sat_Vl * rhoA) - lambda3);

   xdouble *dq = work;
   for (int i = 0; i < dim + 2; ++i)
   {
      dq[i] = q[i] - qbnd[i];
   }
   calcEulerFlux<xdouble, dim>(dir, q, flux);

   // diagonal matrix multiply; note that flux was initialized by calcEulerFlux
   for (int i = 0; i < dim + 2; ++i)
   {
      flux[i] += lambda3 * dq[i];
   }

   // some scalars needed for E1*dq, E2*dq, E3*dq, and E4*dq
   xdouble tmp1 = 0.5 * (lambda1 + lambda2) - lambda3;
   xdouble E1dq_fac = tmp1 * euler::gami / (a * a);
   xdouble E2dq_fac = tmp1 / (dA * dA);
   xdouble E34dq_fac = 0.5 * (lambda1 - lambda2) / (dA * a);

   // get E1*dq + E4*dq and add to flux
   xdouble Edq =
       phi * dq[0] + dq[dim + 1] - dot<xdouble, dim>(qbnd + 1, dq + 1) * fac;
   flux[0] += E1dq_fac * Edq;
   for (int i = 0; i < dim; ++i)
   {
      flux[i + 1] += Edq * (E1dq_fac * qbnd[i + 1] * fac +
                            euler::gami * E34dq_fac * dir[i]);
   }
   flux[dim + 1] += Edq * (E1dq_fac * H + euler::gami * E34dq_fac * Un);

   // get E2*dq + E3*dq and add to flux
   Edq = -Un * dq[0] + dot<xdouble, dim>(dir, dq + 1);
   flux[0] += E34dq_fac * Edq;
   for (int i = 0; i < dim; ++i)
   {
      flux[i + 1] += Edq * (E2dq_fac * dir[i] + E34dq_fac * qbnd[i + 1] * fac);
   }
   flux[dim + 1] += Edq * (E2dq_fac * Un + E34dq_fac * H);
}

/// Boundary flux that uses characteristics to determine which state to use
/// \param[in] dir - direction in which the flux is desired
/// \param[in] qbnd - boundary values of the **conservative** variables
/// \param[in] q - interior domain values of the state variables
/// \param[in] work - a work vector of size `dim+2`
/// \param[out] flux - fluxes in the direction `dir`
/// \tparam xdouble - typically `double` or `adept::adouble`
/// \tparam dim - number of spatial dimensions (1, 2, or 3)
/// \tparam entvar - if true, `q` is entropy var; otherwise, `q` is conservative
/// \note the "flux Jacobian" is computed using `qbnd`, so the boundary values
/// define what is inflow and what is outflow.
template <typename xdouble, int dim, bool entvar = false>
void calcFarFieldFlux(const xdouble *dir,
                      const xdouble *qbnd,
                      const xdouble *q,
                      xdouble *work,
                      xdouble *flux)
{
   if (entvar)
   {
      xdouble qcons[dim + 2];
      calcConservativeVars<xdouble, dim>(q, qcons);
      calcBoundaryFlux<xdouble, dim>(dir, qbnd, qcons, work, flux);
   }
   else
   {
      calcBoundaryFlux<xdouble, dim>(dir, qbnd, q, work, flux);
   }
}

/// Isentropic vortex exact state as a function of position
/// \param[in] x - location at which the exact state is desired
/// \param[out] qbnd - vortex conservative variable at `x`
/// \tparam xdouble - typically `double` or `adept::adouble`
/// \note  I reversed the flow direction to be clockwise, so the problem and
/// mesh are consistent with the LPS paper (that is, because the triangles are
/// subdivided from the quads using the opposite diagonal)
template <typename xdouble>
void calcIsentropicVortexState(const xdouble *x, xdouble *qbnd)
{
   double ri = 1.0;
   double Mai = 0.5;  // 0.95
   double rhoi = 2.0;
   double prsi = 1.0 / euler::gamma;
   xdouble rinv = ri / sqrt(x[0] * x[0] + x[1] * x[1]);
   xdouble rho =
       rhoi * pow(1.0 + 0.5 * euler::gami * Mai * Mai * (1.0 - rinv * rinv),
                  1.0 / euler::gami);
   xdouble Ma =
       sqrt((2.0 / euler::gami) * ((pow(rhoi / rho, euler::gami)) *
                                       (1.0 + 0.5 * euler::gami * Mai * Mai) -
                                   1.0));
   xdouble theta;
   if (x[0] > 1e-15)
   {
      theta = atan(x[1] / x[0]);
   }
   else
   {
      theta = M_PI / 2.0;
   }
   xdouble press = prsi * pow((1.0 + 0.5 * euler::gami * Mai * Mai) /
                                  (1.0 + 0.5 * euler::gami * Ma * Ma),
                              euler::gamma / euler::gami);
   xdouble a = sqrt(euler::gamma * press / rho);

   qbnd[0] = rho;
   qbnd[1] = -rho * a * Ma * sin(theta);
   qbnd[2] = rho * a * Ma * cos(theta);
   qbnd[3] = press / euler::gami + 0.5 * rho * a * a * Ma * Ma;
}

/// A wrapper for `calcBoundaryFlux` in the case of the isentropic vortex
/// \param[in] x - location at which the boundary flux is desired
/// \param[in] dir - desired (scaled) direction of the flux
/// \param[in] q - state variable on the interior of the boundary
/// \param[out] flux - the boundary flux in the direction `dir`
/// \tparam xdouble - typically `double` or `adept::adouble`
/// \tparam entvar - if true, `q` is entropy var; otherwise, `q` is conservative
template <typename xdouble, bool entvar = false>
void calcIsentropicVortexFlux(const xdouble *x,
                              const xdouble *dir,
                              const xdouble *q,
                              xdouble *flux)
{
   xdouble qbnd[4];
   xdouble work[4];
   calcIsentropicVortexState<xdouble>(x, qbnd);
   if (entvar)
   {
      xdouble qcons[4];
      calcConservativeVars<xdouble, 2>(q, qcons);
      calcBoundaryFlux<xdouble, 2>(dir, qbnd, qcons, work, flux);
   }
   else
   {
      calcBoundaryFlux<xdouble, 2>(dir, qbnd, q, work, flux);
   }
}

/// removes the component of momentum normal to the wall from `q`
/// \param[in] dir - vector perpendicular to the wall (does not need to be unit)
/// \param[in] q - the state whose momentum is being projected
/// \param[in] qbnd - the state with the normal component removed
/// \tparam xdouble - typically `double` or `adept::adouble`
/// \tparam dim - number of spatial dimensions (1, 2, or 3)
template <typename xdouble, int dim>
void projectStateOntoWall(const xdouble *dir, const xdouble *q, xdouble *qbnd)
{
   xdouble nrm[dim];
   xdouble fac = 1.0 / sqrt(dot<xdouble, dim>(dir, dir));
   xdouble Unrm = 0.0;
   for (int i = 0; i < dim; ++i)
   {
      nrm[i] = dir[i] * fac;
      Unrm += nrm[i] * q[i + 1];
   }
   qbnd[0] = q[0];
   qbnd[dim + 1] = q[dim + 1];
   for (int i = 0; i < dim; ++i)
   {
      qbnd[i + 1] = q[i + 1] - nrm[i] * Unrm;
   }
}

/// computes an adjoint consistent slip wall boundary condition
/// \param[in] x - not used
/// \param[in] dir - desired (scaled) normal vector to the wall
/// \param[in] q - conservative state variable on the boundary
/// \param[out] flux - the boundary flux in the direction `dir`
/// \tparam xdouble - typically `double` or `adept::adouble`
/// \tparam dim - number of spatial dimensions (1, 2, or 3)
/// \tparam entvar - if true, `q` = ent. vars; otherwise, `q` = conserv. vars
template <typename xdouble, int dim, bool entvar = false>
void calcSlipWallFlux(const xdouble *x,
                      const xdouble *dir,
                      const xdouble *q,
                      xdouble *flux)
{
#if 0
   xdouble qbnd[dim+2];
   projectStateOntoWall<xdouble,dim>(dir, q, qbnd);
   calcEulerFlux<xdouble,dim>(dir, qbnd, flux);
   //calcIsentropicVortexFlux<xdouble>(x, dir, q, flux);
#endif
   xdouble press;
   xdouble press_g = 1.0 / euler::gamma;
   if (entvar)
   {
      auto Vel2 = dot<xdouble, dim>(q + 1, q + 1);
      xdouble s = euler::gamma + euler::gami * (0.5 * Vel2 / q[dim + 1] - q[0]);
      press = -pow(-exp(-s) / q[dim + 1], 1.0 / euler::gami) / q[dim + 1];
   }
   else
   {
      xdouble Ma = 0.2;
      xdouble theta;
      xdouble xc = 5.0;
      xdouble yc = 5.0;
      xdouble circ = 0.0;
      xdouble rad = 0.5;
      xdouble press_e;
      theta = atan2(x[1] - yc, x[0] - xc);
      if (abs(x[0] - xc) < 1e-14)
      {
         theta = 0.5 * M_PI;
      }
      xdouble r =
          sqrt(((x[0] - xc) * (x[0] - xc)) + ((x[1] - yc) * (x[1] - yc)));
      xdouble rinv = rad / r;
      xdouble rtilde = 1.0 / rinv;
      xdouble Vr = Ma * (1.0 - rinv * rinv) * cos(theta);
      xdouble Vth =
          -Ma * (1.0 + rinv * rinv) * sin(theta) - circ / (M_PI * rtilde);
      xdouble ux = Vr * cos(theta) - (Vth * sin(theta));
      xdouble uy = Vr * sin(theta) + (Vth * cos(theta));
      xdouble p_bern =
          1.0 / euler::gamma + 0.5 * Ma * Ma - 0.5 * (ux * ux + uy * uy);
      press = pressure<xdouble, dim>(q);
      // press = p_bern;
      //    std::cout << "pressure diff: " << press - press_e << std::endl;
   }
   flux[0] = 0.0;
   for (int i = 0; i < dim; ++i)
   {
      flux[i + 1] = dir[i] * press;
   }
   flux[dim + 1] = 0.0;
}
/// Compute the Jacobian of the mapping `convert` w.r.t. `u`
/// \param[in] q - conservative variables that are to be converted
/// \param[out] dwdu - Jacobian of entropy variables w.r.t. `u`
template <int dim>
void convertVarsJac(const mfem::Vector &q,
                    adept::Stack &stack,
                    mfem::DenseMatrix &dwdu)
{
   // vector of active input variables
   std::vector<adouble> q_a(q.Size());
   // initialize adouble inputs
   adept::set_values(q_a.data(), q.Size(), q.GetData());
   // start recording
   stack.new_recording();
   // create vector of active output variables
   std::vector<adouble> w_a(q.Size());
   // run algorithm
   calcEntropyVars<adouble, dim>(q_a.data(), w_a.data());
   // identify independent and dependent variables
   stack.independent(q_a.data(), q.Size());
   stack.dependent(w_a.data(), q.Size());
   // compute and store jacobian in dwdu
   stack.jacobian(dwdu.GetData());
}

// computes an adjoint consistent slip wall boundary condition
/// \param[in] x - not used
/// \param[in] dir - desired (scaled) normal vector to the wall
/// \param[in] q - conservative state variable on the boundary
/// \param[out] flux - the boundary flux in the direction `dir`
/// \tparam xdouble - typically `double` or `adept::adouble`
/// \tparam dim - number of spatial dimensions (1, 2, or 3)
template <int dim>
void calcFluxJacState(const mfem::Vector &x,
                      const mfem::Vector &dir,
                      const double jac,
                      const mfem::Vector &q,
                      const mfem::DenseMatrix &Dw,
                      const mfem::Vector &q_ref,
                      const mfem::Vector &work_vec,
                      adept::Stack &stack,
                      mfem::DenseMatrix &flux_jac)
{
   int Dw_size = Dw.Height() * Dw.Width();
   // create containers for active double objects for each input
   std::vector<adouble> q_a(q.Size());
   std::vector<adouble> dir_a(dir.Size());
   std::vector<adouble> x_a(x.Size());
   std::vector<adouble> Dw_a(Dw_size);
   std::vector<adouble> q_ref_a(q_ref.Size());
   std::vector<adouble> work_vec_a(work_vec.Size());
   // initialize active double containers with data from inputs
   adept::set_values(q_a.data(), q.Size(), q.GetData());
   adept::set_values(dir_a.data(), dir.Size(), dir.GetData());
   adept::set_values(x_a.data(), x.Size(), x.GetData());
   adept::set_values(Dw_a.data(), Dw_size, Dw.GetData());
   adept::set_values(q_ref_a.data(), q_ref.Size(), q_ref.GetData());
   // start new stack recording
   stack.new_recording();
   // create container for active double flux output
   std::vector<adouble> flux_a(q.Size());
   mach::calcBoundaryFlux<adouble, dim>(dir_a.data(),
                                        q_ref_a.data(),
                                        q_a.data(),
                                        work_vec_a.data(),
                                        flux_a.data());
   stack.independent(q_a.data(), q.Size());
   stack.dependent(flux_a.data(), q.Size());
   stack.jacobian(flux_jac.GetData());
}

/// Ismail-Roe flux function in direction `dir` with Lax-Friedichs dissipation
/// \param[in] dir - vector direction in which flux is wanted
/// \param[in] diss_coeff - scales the dissipation (must be non-negative!)
/// \param[in] qL - conservative variables at "left" state
/// \param[in] qR - conservative variables at "right" state
/// \param[out] flux - fluxes in the direction `dir`
/// \tparam xdouble - typically `double` or `adept::adouble`
/// \tparam dim - number of spatial dimensions (1, 2, or 3)
template <typename xdouble, int dim>
void calcIsmailRoeFaceFluxWithDiss(const xdouble *dir,
                                   xdouble diss_coeff,
                                   const xdouble *qL,
                                   const xdouble *qR,
                                   xdouble *flux)
{
   auto pL = pressure<xdouble, dim>(qL);
   auto pR = pressure<xdouble, dim>(qR);
   xdouble zL[dim + 2];
   xdouble zR[dim + 2];
   zL[0] = sqrt(qL[0] / pL);
   zR[0] = sqrt(qR[0] / pR);
   for (int i = 0; i < dim; ++i)
   {
      zL[i + 1] = zL[0] * qL[i + 1] / qL[0];
      zR[i + 1] = zR[0] * qR[i + 1] / qR[0];
   }
   zL[dim + 1] = sqrt(qL[0] * pL);
   zR[dim + 1] = sqrt(qR[0] * pR);

   xdouble rho_hat = 0.5 * (zL[0] + zR[0]) * logavg(zL[dim + 1], zR[dim + 1]);

   xdouble U = 0.0;
   for (int i = 0; i < dim; ++i)
   {
      U += (zL[0] * qL[i + 1] / qL[0] + zR[0] * qR[i + 1] / qR[0]) * dir[i] /
           (zL[0] + zR[0]);
   }
   xdouble p1_hat = (zL[dim + 1] + zR[dim + 1]) / (zL[0] + zR[0]);
   xdouble p2_hat =
       ((euler::gamma + 1.0) * logavg(zL[dim + 1], zR[dim + 1]) /
            logavg(zL[0], zR[0]) +
        (euler::gami) * (zL[dim + 1] + zR[dim + 1]) / (zL[0] + zR[0])) /
       (2.0 * euler::gamma);
   xdouble h_hat = euler::gamma * p2_hat / (rho_hat * euler::gami);

   flux[0] = rho_hat * U;
   for (int i = 0; i < dim; ++i)
   {
      flux[i + 1] = (zL[i + 1] + zR[i + 1]) / (zL[0] + zR[0]);  // u_hat
      h_hat += 0.5 * flux[i + 1] * flux[i + 1];
      flux[i + 1] *= rho_hat * U;
      flux[i + 1] += p1_hat * dir[i];
   }
   flux[dim + 1] = rho_hat * h_hat * U;

   // add the dissipation
   xdouble q_ave[dim + 2];
   xdouble wL[dim + 2];
   xdouble wR[dim + 2];
   xdouble w_diff[dim + 2];
   xdouble dqdw_vec[dim + 2];
   calcEntropyVars<xdouble, dim>(qL, wL);  // first convert to entropy vars
   calcEntropyVars<xdouble, dim>(qR, wR);
   for (int i = 0; i < dim + 2; i++)
   {
      q_ave[i] = 0.5 * (qL[i] + qR[i]);
      w_diff[i] = wL[i] - wR[i];
   }
   xdouble lambda = diss_coeff * calcSpectralRadius<xdouble, dim>(dir, q_ave);

   calcdQdWProduct<xdouble, dim>(q_ave, w_diff, dqdw_vec);
   for (int i = 0; i < dim + 2; i++)
   {
      flux[i] = flux[i] + lambda * dqdw_vec[i];
   }
}
/// Ismail-Roe flux function in direction `dir` with Lax-Friedichs dissipation
/// \param[in] dir - vector direction in which flux is wanted
/// \param[in] diss_coeff - scales the dissipation (must be non-negative!)
/// \param[in] qL - entropy variables at "left" state
/// \param[in] qR - entropy variables at "right" state
/// \param[out] flux - fluxes in the direction `dir`
/// \tparam xdouble - typically `double` or `adept::adouble`
/// \tparam dim - number of spatial dimensions (1, 2, or 3)
template <typename xdouble, int dim>
void calcIsmailRoeFaceFluxWithDissUsingEntVars(const xdouble *dir,
                                               xdouble diss_coeff,
                                               const xdouble *wL,
                                               const xdouble *wR,
                                               xdouble *flux)
{
   xdouble zL[dim + 2];
   xdouble zR[dim + 2];
   zL[0] = sqrt(-wL[dim + 1]);
   zR[0] = sqrt(-wR[dim + 1]);
   xdouble sL = 0.0;
   xdouble sR = 0.0;
   for (int i = 0; i < dim; ++i)
   {
      zL[i + 1] = -wL[i + 1] * zL[0] / wL[dim + 1];
      sL += wL[i + 1] * wL[i + 1];
      zR[i + 1] = -wR[i + 1] * zR[0] / wR[dim + 1];
      sR += wR[i + 1] * wR[i + 1];
   }
   sL = euler::gamma +
        euler::gami * (0.5 * sL / wL[dim + 1] - wL[0]);  // physical ent.
   zL[dim + 1] = pow(-exp(-sL) / wL[dim + 1], 1.0 / euler::gami) / zL[0];
   sR = euler::gamma +
        euler::gami * (0.5 * sR / wR[dim + 1] - wR[0]);  // physical ent.
   zR[dim + 1] = pow(-exp(-sR) / wR[dim + 1], 1.0 / euler::gami) / zR[0];

   xdouble rho_hat = 0.5 * (zL[0] + zR[0]) * logavg(zL[dim + 1], zR[dim + 1]);
   xdouble U = 0.0;
   for (int i = 0; i < dim; ++i)
   {
      U += (zL[i + 1] + zR[i + 1]) * dir[i] / (zL[0] + zR[0]);
   }
   xdouble p1_hat = (zL[dim + 1] + zR[dim + 1]) / (zL[0] + zR[0]);
   xdouble p2_hat =
       ((euler::gamma + 1.0) * logavg(zL[dim + 1], zR[dim + 1]) /
            logavg(zL[0], zR[0]) +
        (euler::gami) * (zL[dim + 1] + zR[dim + 1]) / (zL[0] + zR[0])) /
       (2.0 * euler::gamma);
   xdouble h_hat = euler::gamma * p2_hat / (rho_hat * euler::gami);

   flux[0] = rho_hat * U;
   for (int i = 0; i < dim; ++i)
   {
      flux[i + 1] = (zL[i + 1] + zR[i + 1]) / (zL[0] + zR[0]);  // u_hat
      h_hat += 0.5 * flux[i + 1] * flux[i + 1];
      flux[i + 1] *= rho_hat * U;
      flux[i + 1] += p1_hat * dir[i];
   }
   flux[dim + 1] = rho_hat * h_hat * U;

   // add the dissipation
   xdouble qL[dim + 2];
   xdouble qR[dim + 2];
   xdouble q_ave[dim + 2];
   xdouble w_diff[dim + 2];
   xdouble dqdw_vec[dim + 2];
   calcConservativeVars<xdouble, dim>(wL, qL);
   calcConservativeVars<xdouble, dim>(wR, qR);
   for (int i = 0; i < dim + 2; i++)
   {
      q_ave[i] = 0.5 * (qL[i] + qR[i]);
      w_diff[i] = wL[i] - wR[i];
   }
   xdouble lambda = diss_coeff * calcSpectralRadius<xdouble, dim>(dir, q_ave);
   calcdQdWProduct<xdouble, dim>(q_ave, w_diff, dqdw_vec);
   for (int i = 0; i < dim + 2; i++)
   {
      flux[i] = flux[i] + lambda * dqdw_vec[i];
   }
}
/// ------------------- Potential flow over a circle -------------------
#if 1
/// Potential solution exact state as a function of position
/// \param[in] x - location at which the exact state is desired
/// \param[out] qbnd - conservative variable at `x`
/// \tparam xdouble - typically `double` or `adept::adouble`
template <typename xdouble>
void calcPotentialFlowState(const xdouble *x, xdouble *qbnd)
{
   xdouble Ma = 0.2;
   xdouble rho = 1.0;
   xdouble p = 1.0 / euler::gamma;
   xdouble theta;
   xdouble xc = 5.0;
   xdouble yc = 5.0;
   xdouble circ = 0.0;
   xdouble rad = 0.5;
   theta = atan2(x[1] - yc, x[0] - xc);
   if (abs(x[0] - xc) < 1e-14)
   {
      theta = 0.5 * M_PI;
   }
   xdouble r = sqrt(((x[0] - xc) * (x[0] - xc)) + ((x[1] - yc) * (x[1] - yc)));
   xdouble rinv = rad / r;
   xdouble rtilde = 1.0 / rinv;
   xdouble Vr = Ma * (1.0 - rinv * rinv) * cos(theta);
   xdouble Vth =
       -Ma * (1.0 + rinv * rinv) * sin(theta) - circ / (M_PI * rtilde);
   xdouble ux = Vr * cos(theta) - (Vth * sin(theta));
   xdouble uy = Vr * sin(theta) + (Vth * cos(theta));
   //    rho = 1.0 - 0.5*euler::gami*Ma*Ma*((ux * ux + uy * uy) - 1.0);
   //    rho = pow(rho, 1.0/euler::gami);
   xdouble p_bern =
       1.0 / euler::gamma + 0.5 * Ma * Ma - 0.5 * rho * (ux * ux + uy * uy);
   qbnd[0] = rho;
   qbnd[1] = rho * ux;
   qbnd[2] = rho * uy;
   qbnd[3] = p_bern / euler::gami + 0.5 * rho * (ux * ux + uy * uy);
}
#endif
/// ------------------- Potential flow over an ellipse -------------------
/// Potential solution exact state as a function of position
/// \param[in] x - location at which the exact state is desired
/// \param[out] qbnd - vortex conservative variable at `x`
/// \tparam xdouble - typically `double` or `adept::adouble`
#if 0
template <typename xdouble>
void calcPotentialFlowState(const xdouble *x, xdouble *qbnd)
{
   xdouble Ma = 0.2;  // 0.95
   xdouble rho = 1.0;
   xdouble p = 1.0 / euler::gamma;
   xdouble theta;
   // ellipse parameters
   xdouble a = 2.5;
   xdouble b = sqrt(a * (a - 1.0));
   xdouble xc = 10.0;
   xdouble yc = 10.0;
   xdouble s =
       ((x[0] - xc) * (x[0] - xc)) - ((x[1] - yc) * (x[1] - yc)) - 4.0 * b * b;
   xdouble t = 2.0 * (x[0] - xc) * (x[1] - yc);
   theta = atan2(t, s);
   xdouble signx = 1.0;
   if (x[0] - xc < 0)
   {
      signx = -1.0;
   }
   xdouble r = sqrt(t * t + s * s);
   xdouble xi = 0.5 * (x[0] - xc + (signx * sqrt(r) * cos(theta / 2.0)));
   xdouble eta = 0.5 * (x[1] - yc + (signx * sqrt(r) * sin(theta / 2.0)));
   xdouble term_a = xi * xi - eta * eta - a * a;
   xdouble term_b = xi * xi - eta * eta - b * b;
   xdouble term_c = 4.0 * xi * xi * eta * eta;
   xdouble term_d = (term_b * term_b) + term_c;
   qbnd[0] = rho;
   qbnd[1] = rho * Ma * ((term_a * term_b) + term_c) / term_d;
   qbnd[2] = -rho * Ma * 2.0 * xi * eta * (term_b - term_a) / term_d;
   qbnd[3] = p / euler::gami + 0.5 * Ma * Ma;
}
#endif
/// A wrapper for `calcBoundaryFlux` in the case of the isentropic vortex
/// \param[in] x - location at which the boundary flux is desired
/// \param[in] dir - desired (scaled) direction of the flux
/// \param[in] q - state variable on the interior of the boundary
/// \param[out] flux - the boundary flux in the direction `dir`
/// \tparam xdouble - typically `double` or `adept::adouble`
/// \tparam entvar - if true, `q` is entropy var; otherwise, `q` is conservative
template <typename xdouble, bool entvar = false>
void calcPotentialFlowFlux(const xdouble *x,
                           const xdouble *dir,
                           const xdouble *q,
                           xdouble *flux)
{
   xdouble qbnd[4];
   xdouble work[4];
   using namespace std;
   calcPotentialFlowState<xdouble>(x, qbnd);
   if (entvar)
   {
      xdouble qcons[4];
      calcConservativeVars<xdouble, 2>(q, qcons);
      calcBoundaryFlux<xdouble, 2>(dir, qbnd, qcons, work, flux);
   }
   else
   {
      calcBoundaryFlux<xdouble, 2>(dir, qbnd, q, work, flux);
   }
}

/// Isentropic vortex exact state as a function of position
/// \param[in] x - location at which the exact state is desired
/// \param[out] qbnd - vortex conservative variable at `x`
/// \tparam xdouble - typically `double` or `adept::adouble`
/// \note  I reversed the flow direction to be clockwise, so the problem and
/// mesh are consistent with the LPS paper (that is, because the triangles are
/// subdivided from the quads using the opposite diagonal)
template <typename xdouble>
void calcInviscidMMSState(const xdouble *x, xdouble *qbnd)
{
   double rho0 = 1.0;
   double rhop = 0.05;
   double u0 = 0.5;
   double up = 0.05;
   double T0 = 1.0;
   double Tp = 0.05;
   double scale = 40.0;
   double xc = 0.0;
   double yc = 0.0;
   /// define the exact solution
   xdouble rho = rho0 + rhop * pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                            sin(M_PI * (x[1] - yc) / scale);
   xdouble ux = 4.0 * u0 * ((x[1] - yc) / scale) * (1.0 - (x[1] - yc) / scale) +
                (up * sin(2.0 * M_PI * ((x[1] - yc)) / scale) *
                 pow(sin(M_PI * (x[0] - xc) / scale), 2));
   xdouble uy = -up * pow(sin(2.0 * M_PI * (x[0] - xc) / scale), 2) *
                sin(M_PI * (x[1] - yc) / scale);
   xdouble T =
       T0 +
       Tp * (pow((x[0] - xc) / scale, 4) - (2.0 * pow((x[0] - xc) / scale, 3)) +
             pow((x[0] - xc) / scale, 2) + pow((x[1] - yc) / scale, 4) -
             (2.0 * pow((x[1] - yc) / scale, 3)) + pow((x[1] - yc) / scale, 2));
   xdouble p = rho * T;
   xdouble e = (p / (euler::gamma - 1)) + 0.5 * rho * (ux * ux + uy * uy);
   qbnd[0] = rho;
   qbnd[1] = rho * ux;  // multiply by rho ?
   qbnd[2] = rho * uy;
   qbnd[3] = e;
}

/// A wrapper for `calcBoundaryFlux` in the case of the inviscid MMS euler
/// \param[in] x - location at which the boundary flux is desired
/// \param[in] dir - desired (scaled) direction of the flux
/// \param[in] q - state variable on the interior of the boundary
/// \param[out] flux - the boundary flux in the direction `dir`
/// \tparam xdouble - typically `double` or `adept::adouble`
/// \tparam entvar - if true, `q` is entropy var; otherwise, `q` is conservative
template <typename xdouble, bool entvar = false>
void calcInviscidMMSFlux(const xdouble *x,
                         const xdouble *dir,
                         const xdouble *q,
                         xdouble *flux)
{
   xdouble qbnd[4];
   xdouble work[4];
   calcInviscidMMSState<xdouble>(x, qbnd);
   if (entvar)
   {
      xdouble qcons[4];
      calcConservativeVars<xdouble, 2>(q, qcons);
      calcBoundaryFlux<xdouble, 2>(dir, qbnd, qcons, work, flux);
   }
   else
   {
      calcBoundaryFlux<xdouble, 2>(dir, qbnd, q, work, flux);
   }
}

#if 1
/// MMS source term for a particular Euler solution verification
/// \param[in] x - location at which to evaluate the source
/// \param[out] src - the source value
/// \tparam xdouble - typically `double` or `adept::adouble`
template <typename xdouble>
void calcInviscidMMS(const xdouble *x, xdouble *src)
{
   using euler::gamma;
   const double rho0 = 1.0;
   const double rhop = 0.05;
   const double u0 = 0.5;
   const double up = 0.05;
   const double T0 = 1.0;
   const double Tp = 0.05;
   const double scale = 40.0;
   const double xc = 0.0;
   const double yc = 0.0;
   src[0] =
       M_PI *
       (2 * rhop *
            (pow(scale, 2) * up * pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                 sin(2 * M_PI * (x[1] - yc) / scale) +
             4 * u0 * (x[1] - yc) * (scale - x[1] + yc)) *
            sin(M_PI * (x[0] - xc) / scale) * sin(M_PI * (x[1] - yc) / scale) *
            cos(M_PI * (x[0] - xc) / scale) +
        pow(scale, 2) * up *
            (-rhop * pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                 pow(sin(2 * M_PI * (x[0] - xc) / scale), 2) *
                 sin(M_PI * (x[1] - yc) / scale) *
                 cos(M_PI * (x[1] - yc) / scale) +
             2 *
                 (rho0 + rhop * pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                             sin(M_PI * (x[1] - yc) / scale)) *
                 sin(M_PI * (x[0] - xc) / scale) *
                 sin(2 * M_PI * (x[1] - yc) / scale) *
                 cos(M_PI * (x[0] - xc) / scale) -
             (rho0 + rhop * pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                         sin(M_PI * (x[1] - yc) / scale)) *
                 pow(sin(2 * M_PI * (x[0] - xc) / scale), 2) *
                 cos(M_PI * (x[1] - yc) / scale))) /
       pow(scale, 3);
   src[1] =
       Tp *
           (rho0 + rhop * pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                       sin(M_PI * (x[1] - yc) / scale)) *
           ((2 * x[0] - 2 * xc) / pow(scale, 2) -
            6 * pow(x[0] - xc, 2) / pow(scale, 3) +
            4 * pow(x[0] - xc, 3) / pow(scale, 4)) -
       M_PI * rhop * up *
           (up * pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                sin(2 * M_PI * (x[1] - yc) / scale) +
            4 * u0 * (1 - (x[1] - yc) / scale) * (x[1] - yc) / scale) *
           pow(sin(M_PI * (x[0] - xc) / scale), 2) *
           pow(sin(2 * M_PI * (x[0] - xc) / scale), 2) *
           sin(M_PI * (x[1] - yc) / scale) * cos(M_PI * (x[1] - yc) / scale) /
           scale +
       2 * M_PI * rhop *
           (T0 + Tp * (pow(x[0] - xc, 2) / pow(scale, 2) +
                       pow(x[1] - yc, 2) / pow(scale, 2) -
                       2 * pow(x[0] - xc, 3) / pow(scale, 3) -
                       2 * pow(x[1] - yc, 3) / pow(scale, 3) +
                       pow(x[0] - xc, 4) / pow(scale, 4) +
                       pow(x[1] - yc, 4) / pow(scale, 4))) *
           sin(M_PI * (x[0] - xc) / scale) * sin(M_PI * (x[1] - yc) / scale) *
           cos(M_PI * (x[0] - xc) / scale) / scale +
       2 * M_PI * rhop *
           pow(up * pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                       sin(2 * M_PI * (x[1] - yc) / scale) +
                   4 * u0 * (1 - (x[1] - yc) / scale) * (x[1] - yc) / scale,
               2) *
           sin(M_PI * (x[0] - xc) / scale) * sin(M_PI * (x[1] - yc) / scale) *
           cos(M_PI * (x[0] - xc) / scale) / scale -
       up *
           (rho0 + rhop * pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                       sin(M_PI * (x[1] - yc) / scale)) *
           (4 * u0 * (1 - (x[1] - yc) / scale) / scale +
            2 * M_PI * up * pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                cos(2 * M_PI * (x[1] - yc) / scale) / scale -
            4 * u0 * (x[1] - yc) / pow(scale, 2)) *
           pow(sin(2 * M_PI * (x[0] - xc) / scale), 2) *
           sin(M_PI * (x[1] - yc) / scale) +
       4 * M_PI * up *
           (rho0 + rhop * pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                       sin(M_PI * (x[1] - yc) / scale)) *
           (up * pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                sin(2 * M_PI * (x[1] - yc) / scale) +
            4 * u0 * (1 - (x[1] - yc) / scale) * (x[1] - yc) / scale) *
           sin(M_PI * (x[0] - xc) / scale) *
           sin(2 * M_PI * (x[1] - yc) / scale) *
           cos(M_PI * (x[0] - xc) / scale) / scale -
       M_PI * up *
           (rho0 + rhop * pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                       sin(M_PI * (x[1] - yc) / scale)) *
           (up * pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                sin(2 * M_PI * (x[1] - yc) / scale) +
            4 * u0 * (1 - (x[1] - yc) / scale) * (x[1] - yc) / scale) *
           pow(sin(2 * M_PI * (x[0] - xc) / scale), 2) *
           cos(M_PI * (x[1] - yc) / scale) / scale;
   src[2] = Tp *
                (rho0 + rhop * pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                            sin(M_PI * (x[1] - yc) / scale)) *
                ((2 * x[1] - 2 * yc) / pow(scale, 2) -
                 6 * pow(x[1] - yc, 2) / pow(scale, 3) +
                 4 * pow(x[1] - yc, 3) / pow(scale, 4)) +
            M_PI * rhop * pow(up, 2) * pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                pow(sin(2 * M_PI * (x[0] - xc) / scale), 4) *
                pow(sin(M_PI * (x[1] - yc) / scale), 2) *
                cos(M_PI * (x[1] - yc) / scale) / scale -
            2 * M_PI * rhop * up *
                (up * pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                     sin(2 * M_PI * (x[1] - yc) / scale) +
                 4 * u0 * (1 - (x[1] - yc) / scale) * (x[1] - yc) / scale) *
                sin(M_PI * (x[0] - xc) / scale) *
                pow(sin(2 * M_PI * (x[0] - xc) / scale), 2) *
                pow(sin(M_PI * (x[1] - yc) / scale), 2) *
                cos(M_PI * (x[0] - xc) / scale) / scale +
            M_PI * rhop *
                (T0 + Tp * (pow(x[0] - xc, 2) / pow(scale, 2) +
                            pow(x[1] - yc, 2) / pow(scale, 2) -
                            2 * pow(x[0] - xc, 3) / pow(scale, 3) -
                            2 * pow(x[1] - yc, 3) / pow(scale, 3) +
                            pow(x[0] - xc, 4) / pow(scale, 4) +
                            pow(x[1] - yc, 4) / pow(scale, 4))) *
                pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                cos(M_PI * (x[1] - yc) / scale) / scale -
            2 * M_PI * pow(up, 2) *
                (rho0 + rhop * pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                            sin(M_PI * (x[1] - yc) / scale)) *
                sin(M_PI * (x[0] - xc) / scale) *
                pow(sin(2 * M_PI * (x[0] - xc) / scale), 2) *
                sin(M_PI * (x[1] - yc) / scale) *
                sin(2 * M_PI * (x[1] - yc) / scale) *
                cos(M_PI * (x[0] - xc) / scale) / scale +
            2 * M_PI * pow(up, 2) *
                (rho0 + rhop * pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                            sin(M_PI * (x[1] - yc) / scale)) *
                pow(sin(2 * M_PI * (x[0] - xc) / scale), 4) *
                sin(M_PI * (x[1] - yc) / scale) *
                cos(M_PI * (x[1] - yc) / scale) / scale -
            4 * M_PI * up *
                (rho0 + rhop * pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                            sin(M_PI * (x[1] - yc) / scale)) *
                (up * pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                     sin(2 * M_PI * (x[1] - yc) / scale) +
                 4 * u0 * (1 - (x[1] - yc) / scale) * (x[1] - yc) / scale) *
                sin(2 * M_PI * (x[0] - xc) / scale) *
                sin(M_PI * (x[1] - yc) / scale) *
                cos(2 * M_PI * (x[0] - xc) / scale) / scale;
   src[3] =
       -up *
           (Tp *
                (rho0 + rhop * pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                            sin(M_PI * (x[1] - yc) / scale)) *
                ((2 * x[1] - 2 * yc) / pow(scale, 2) -
                 6 * pow(x[1] - yc, 2) / pow(scale, 3) +
                 4 * pow(x[1] - yc, 3) / pow(scale, 4)) +
            Tp *
                (rho0 + rhop * pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                            sin(M_PI * (x[1] - yc) / scale)) *
                ((2 * x[1] - 2 * yc) / pow(scale, 2) -
                 6 * pow(x[1] - yc, 2) / pow(scale, 3) +
                 4 * pow(x[1] - yc, 3) / pow(scale, 4)) /
                (gamma - 1) +
            M_PI * rhop *
                (T0 + Tp * (pow(x[0] - xc, 2) / pow(scale, 2) +
                            pow(x[1] - yc, 2) / pow(scale, 2) -
                            2 * pow(x[0] - xc, 3) / pow(scale, 3) -
                            2 * pow(x[1] - yc, 3) / pow(scale, 3) +
                            pow(x[0] - xc, 4) / pow(scale, 4) +
                            pow(x[1] - yc, 4) / pow(scale, 4))) *
                pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                cos(M_PI * (x[1] - yc) / scale) / scale +
            M_PI * rhop *
                (T0 + Tp * (pow(x[0] - xc, 2) / pow(scale, 2) +
                            pow(x[1] - yc, 2) / pow(scale, 2) -
                            2 * pow(x[0] - xc, 3) / pow(scale, 3) -
                            2 * pow(x[1] - yc, 3) / pow(scale, 3) +
                            pow(x[0] - xc, 4) / pow(scale, 4) +
                            pow(x[1] - yc, 4) / pow(scale, 4))) *
                pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                cos(M_PI * (x[1] - yc) / scale) / (scale * (gamma - 1)) +
            (1.0 / 2.0) * M_PI * rhop *
                (pow(up, 2) * pow(sin(2 * M_PI * (x[0] - xc) / scale), 4) *
                     pow(sin(M_PI * (x[1] - yc) / scale), 2) +
                 pow(up * pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                             sin(2 * M_PI * (x[1] - yc) / scale) +
                         4 * u0 * (1 - (x[1] - yc) / scale) * (x[1] - yc) /
                             scale,
                     2)) *
                pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                cos(M_PI * (x[1] - yc) / scale) / scale +
            ((1.0 / 2.0) * rho0 + (1.0 / 2.0) * rhop *
                                      pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                                      sin(M_PI * (x[1] - yc) / scale)) *
                ((up * pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                      sin(2 * M_PI * (x[1] - yc) / scale) +
                  4 * u0 * (1 - (x[1] - yc) / scale) * (x[1] - yc) / scale) *
                     (8 * u0 * (1 - (x[1] - yc) / scale) / scale +
                      4 * M_PI * up * pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                          cos(2 * M_PI * (x[1] - yc) / scale) / scale -
                      8 * u0 * (x[1] - yc) / pow(scale, 2)) +
                 2 * M_PI * pow(up, 2) *
                     pow(sin(2 * M_PI * (x[0] - xc) / scale), 4) *
                     sin(M_PI * (x[1] - yc) / scale) *
                     cos(M_PI * (x[1] - yc) / scale) / scale)) *
           pow(sin(2 * M_PI * (x[0] - xc) / scale), 2) *
           sin(M_PI * (x[1] - yc) / scale) +
       (up * pow(sin(M_PI * (x[0] - xc) / scale), 2) *
            sin(2 * M_PI * (x[1] - yc) / scale) +
        4 * u0 * (1 - (x[1] - yc) / scale) * (x[1] - yc) / scale) *
           (Tp *
                (rho0 + rhop * pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                            sin(M_PI * (x[1] - yc) / scale)) *
                ((2 * x[0] - 2 * xc) / pow(scale, 2) -
                 6 * pow(x[0] - xc, 2) / pow(scale, 3) +
                 4 * pow(x[0] - xc, 3) / pow(scale, 4)) +
            Tp *
                (rho0 + rhop * pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                            sin(M_PI * (x[1] - yc) / scale)) *
                ((2 * x[0] - 2 * xc) / pow(scale, 2) -
                 6 * pow(x[0] - xc, 2) / pow(scale, 3) +
                 4 * pow(x[0] - xc, 3) / pow(scale, 4)) /
                (gamma - 1) +
            2 * M_PI * rhop *
                (T0 + Tp * (pow(x[0] - xc, 2) / pow(scale, 2) +
                            pow(x[1] - yc, 2) / pow(scale, 2) -
                            2 * pow(x[0] - xc, 3) / pow(scale, 3) -
                            2 * pow(x[1] - yc, 3) / pow(scale, 3) +
                            pow(x[0] - xc, 4) / pow(scale, 4) +
                            pow(x[1] - yc, 4) / pow(scale, 4))) *
                sin(M_PI * (x[0] - xc) / scale) *
                sin(M_PI * (x[1] - yc) / scale) *
                cos(M_PI * (x[0] - xc) / scale) / scale +
            2 * M_PI * rhop *
                (T0 + Tp * (pow(x[0] - xc, 2) / pow(scale, 2) +
                            pow(x[1] - yc, 2) / pow(scale, 2) -
                            2 * pow(x[0] - xc, 3) / pow(scale, 3) -
                            2 * pow(x[1] - yc, 3) / pow(scale, 3) +
                            pow(x[0] - xc, 4) / pow(scale, 4) +
                            pow(x[1] - yc, 4) / pow(scale, 4))) *
                sin(M_PI * (x[0] - xc) / scale) *
                sin(M_PI * (x[1] - yc) / scale) *
                cos(M_PI * (x[0] - xc) / scale) / (scale * (gamma - 1)) +
            M_PI * rhop *
                (pow(up, 2) * pow(sin(2 * M_PI * (x[0] - xc) / scale), 4) *
                     pow(sin(M_PI * (x[1] - yc) / scale), 2) +
                 pow(up * pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                             sin(2 * M_PI * (x[1] - yc) / scale) +
                         4 * u0 * (1 - (x[1] - yc) / scale) * (x[1] - yc) /
                             scale,
                     2)) *
                sin(M_PI * (x[0] - xc) / scale) *
                sin(M_PI * (x[1] - yc) / scale) *
                cos(M_PI * (x[0] - xc) / scale) / scale +
            ((1.0 / 2.0) * rho0 + (1.0 / 2.0) * rhop *
                                      pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                                      sin(M_PI * (x[1] - yc) / scale)) *
                (8 * M_PI * pow(up, 2) *
                     pow(sin(2 * M_PI * (x[0] - xc) / scale), 3) *
                     pow(sin(M_PI * (x[1] - yc) / scale), 2) *
                     cos(2 * M_PI * (x[0] - xc) / scale) / scale +
                 4 * M_PI * up *
                     (up * pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                          sin(2 * M_PI * (x[1] - yc) / scale) +
                      4 * u0 * (1 - (x[1] - yc) / scale) * (x[1] - yc) /
                          scale) *
                     sin(M_PI * (x[0] - xc) / scale) *
                     sin(2 * M_PI * (x[1] - yc) / scale) *
                     cos(M_PI * (x[0] - xc) / scale) / scale)) +
       2 * M_PI * up *
           ((T0 + Tp * (pow(x[0] - xc, 2) / pow(scale, 2) +
                        pow(x[1] - yc, 2) / pow(scale, 2) -
                        2 * pow(x[0] - xc, 3) / pow(scale, 3) -
                        2 * pow(x[1] - yc, 3) / pow(scale, 3) +
                        pow(x[0] - xc, 4) / pow(scale, 4) +
                        pow(x[1] - yc, 4) / pow(scale, 4))) *
                (rho0 + rhop * pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                            sin(M_PI * (x[1] - yc) / scale)) +
            (T0 + Tp * (pow(x[0] - xc, 2) / pow(scale, 2) +
                        pow(x[1] - yc, 2) / pow(scale, 2) -
                        2 * pow(x[0] - xc, 3) / pow(scale, 3) -
                        2 * pow(x[1] - yc, 3) / pow(scale, 3) +
                        pow(x[0] - xc, 4) / pow(scale, 4) +
                        pow(x[1] - yc, 4) / pow(scale, 4))) *
                (rho0 + rhop * pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                            sin(M_PI * (x[1] - yc) / scale)) /
                (gamma - 1) +
            (1.0 / 2.0) *
                (rho0 + rhop * pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                            sin(M_PI * (x[1] - yc) / scale)) *
                (pow(up, 2) * pow(sin(2 * M_PI * (x[0] - xc) / scale), 4) *
                     pow(sin(M_PI * (x[1] - yc) / scale), 2) +
                 pow(up * pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                             sin(2 * M_PI * (x[1] - yc) / scale) +
                         4 * u0 * (1 - (x[1] - yc) / scale) * (x[1] - yc) /
                             scale,
                     2))) *
           sin(M_PI * (x[0] - xc) / scale) *
           sin(2 * M_PI * (x[1] - yc) / scale) *
           cos(M_PI * (x[0] - xc) / scale) / scale -
       M_PI * up *
           ((T0 + Tp * (pow(x[0] - xc, 2) / pow(scale, 2) +
                        pow(x[1] - yc, 2) / pow(scale, 2) -
                        2 * pow(x[0] - xc, 3) / pow(scale, 3) -
                        2 * pow(x[1] - yc, 3) / pow(scale, 3) +
                        pow(x[0] - xc, 4) / pow(scale, 4) +
                        pow(x[1] - yc, 4) / pow(scale, 4))) *
                (rho0 + rhop * pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                            sin(M_PI * (x[1] - yc) / scale)) +
            (T0 + Tp * (pow(x[0] - xc, 2) / pow(scale, 2) +
                        pow(x[1] - yc, 2) / pow(scale, 2) -
                        2 * pow(x[0] - xc, 3) / pow(scale, 3) -
                        2 * pow(x[1] - yc, 3) / pow(scale, 3) +
                        pow(x[0] - xc, 4) / pow(scale, 4) +
                        pow(x[1] - yc, 4) / pow(scale, 4))) *
                (rho0 + rhop * pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                            sin(M_PI * (x[1] - yc) / scale)) /
                (gamma - 1) +
            (1.0 / 2.0) *
                (rho0 + rhop * pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                            sin(M_PI * (x[1] - yc) / scale)) *
                (pow(up, 2) * pow(sin(2 * M_PI * (x[0] - xc) / scale), 4) *
                     pow(sin(M_PI * (x[1] - yc) / scale), 2) +
                 pow(up * pow(sin(M_PI * (x[0] - xc) / scale), 2) *
                             sin(2 * M_PI * (x[1] - yc) / scale) +
                         4 * u0 * (1 - (x[1] - yc) / scale) * (x[1] - yc) /
                             scale,
                     2))) *
           pow(sin(2 * M_PI * (x[0] - xc) / scale), 2) *
           cos(M_PI * (x[1] - yc) / scale) / scale;
}
#endif
#if 0
/// MMS source term for a particular Euler solution verification
/// \param[in] x - location at which to evaluate the source
/// \param[out] src - the source value
/// \tparam xdouble - typically `double` or `adept::adouble`
template <typename xdouble>
void calcInviscidMMS(const xdouble *x, xdouble *src)
{
   using euler::gamma;
   const double rho0 = 1.0;
   const double rhop = 0.05;
   const double u0 = 0.5;
   const double up = 0.05;
   const double T0 = 1.0;
   const double Tp = 0.05;
   const double scale = 1.0;
   src[0] =
       2 * M_PI *
       (2 * up * pow(scale, 2) *
            (rho0 * pow(sin(M_PI * x[0] / scale), 3) -
             rho0 * sin(M_PI * x[0] / scale) +
             rho0 * sin(M_PI * x[1] / scale) * cos(M_PI * x[0] / scale) +
             2 * rhop * pow(sin(M_PI * x[0] / scale), 5) *
                 sin(M_PI * x[1] / scale) -
             2 * rhop * pow(sin(M_PI * x[0] / scale), 3) *
                 sin(M_PI * x[1] / scale) +
             rhop * pow(sin(M_PI * x[0] / scale), 2) *
                 pow(sin(M_PI * x[1] / scale), 2) * cos(M_PI * x[0] / scale)) *
            cos(M_PI * x[1] / scale) +
        rhop *
            (4 * u0 * x[1] * (scale - x[1]) +
             up * pow(scale, 2) * pow(sin(M_PI * x[0] / scale), 2) *
                 sin(2 * M_PI * x[1] / scale)) *
            sin(M_PI * x[1] / scale) * cos(M_PI * x[0] / scale)) *
       sin(M_PI * x[0] / scale) / pow(scale, 3);

   src[1] = (2 * Tp * scale * x[0] *
                 (rho0 + rhop * pow(sin(M_PI * x[0] / scale), 2) *
                             sin(M_PI * x[1] / scale)) *
                 (pow(scale, 2) - 3 * scale * x[0] + 2 * pow(x[0], 2)) +
             up * pow(scale, 2) *
                 (-M_PI * rhop *
                      (4 * u0 * x[1] * (scale - x[1]) +
                       up * pow(scale, 2) * pow(sin(M_PI * x[0] / scale), 2) *
                           sin(2 * M_PI * x[1] / scale)) *
                      pow(sin(M_PI * x[0] / scale), 2) *
                      pow(sin(2 * M_PI * x[0] / scale), 2) *
                      sin(M_PI * x[1] / scale) * cos(M_PI * x[1] / scale) -
                  2 * scale *
                      (rho0 + rhop * pow(sin(M_PI * x[0] / scale), 2) *
                                  sin(M_PI * x[1] / scale)) *
                      (2 * u0 * (scale - 2 * x[1]) +
                       M_PI * up * scale * pow(sin(M_PI * x[0] / scale), 2) *
                           cos(2 * M_PI * x[1] / scale)) *
                      pow(sin(2 * M_PI * x[0] / scale), 2) *
                      sin(M_PI * x[1] / scale) +
                  4 * M_PI *
                      (rho0 + rhop * pow(sin(M_PI * x[0] / scale), 2) *
                                  sin(M_PI * x[1] / scale)) *
                      (4 * u0 * x[1] * (scale - x[1]) +
                       up * pow(scale, 2) * pow(sin(M_PI * x[0] / scale), 2) *
                           sin(2 * M_PI * x[1] / scale)) *
                      sin(M_PI * x[0] / scale) * sin(2 * M_PI * x[1] / scale) *
                      cos(M_PI * x[0] / scale) -
                  M_PI *
                      (rho0 + rhop * pow(sin(M_PI * x[0] / scale), 2) *
                                  sin(M_PI * x[1] / scale)) *
                      (4 * u0 * x[1] * (scale - x[1]) +
                       up * pow(scale, 2) * pow(sin(M_PI * x[0] / scale), 2) *
                           sin(2 * M_PI * x[1] / scale)) *
                      pow(sin(2 * M_PI * x[0] / scale), 2) *
                      cos(M_PI * x[1] / scale)) +
             (1.0 / 2.0) * M_PI * rhop *
                 (T0 * pow(scale, 4) +
                  Tp * (pow(scale, 2) * (pow(x[0], 2) + pow(x[1], 2)) -
                        2 * scale * (pow(x[0], 3) + pow(x[1], 3)) +
                        pow(x[0], 4) + pow(x[1], 4))) *
                 (cos(M_PI * (2 * x[0] - x[1]) / scale) -
                  cos(M_PI * (2 * x[0] + x[1]) / scale)) +
             2 * M_PI * rhop *
                 pow(4 * u0 * x[1] * (scale - x[1]) +
                         up * pow(scale, 2) * pow(sin(M_PI * x[0] / scale), 2) *
                             sin(2 * M_PI * x[1] / scale),
                     2) *
                 sin(M_PI * x[0] / scale) * sin(M_PI * x[1] / scale) *
                 cos(M_PI * x[0] / scale)) /
            pow(scale, 5);

   src[2] =
       (2 * Tp * scale * x[1] *
            (rho0 + rhop * pow(sin(M_PI * x[0] / scale), 2) *
                        sin(M_PI * x[1] / scale)) *
            (pow(scale, 2) - 3 * scale * x[1] + 2 * pow(x[1], 2)) +
        M_PI * pow(up, 2) * pow(scale, 4) *
            (rhop * pow(sin(M_PI * x[0] / scale), 2) *
                 pow(sin(2 * M_PI * x[0] / scale), 2) *
                 sin(M_PI * x[1] / scale) * cos(M_PI * x[1] / scale) -
             2 *
                 (rho0 + rhop * pow(sin(M_PI * x[0] / scale), 2) *
                             sin(M_PI * x[1] / scale)) *
                 sin(M_PI * x[0] / scale) * sin(2 * M_PI * x[1] / scale) *
                 cos(M_PI * x[0] / scale) +
             2 *
                 (rho0 + rhop * pow(sin(M_PI * x[0] / scale), 2) *
                             sin(M_PI * x[1] / scale)) *
                 pow(sin(2 * M_PI * x[0] / scale), 2) *
                 cos(M_PI * x[1] / scale)) *
            pow(sin(2 * M_PI * x[0] / scale), 2) * sin(M_PI * x[1] / scale) -
        1.0 / 2.0 * M_PI * up * pow(scale, 2) *
            (4 * u0 * x[1] * (scale - x[1]) +
             up * pow(scale, 2) * pow(sin(M_PI * x[0] / scale), 2) *
                 sin(2 * M_PI * x[1] / scale)) *
            (rhop * (1 - cos(4 * M_PI * x[0] / scale)) *
                 sin(M_PI * x[1] / scale) +
             8 *
                 (rho0 + rhop * pow(sin(M_PI * x[0] / scale), 2) *
                             sin(M_PI * x[1] / scale)) *
                 cos(2 * M_PI * x[0] / scale)) *
            sin(2 * M_PI * x[0] / scale) * sin(M_PI * x[1] / scale) +
        M_PI * rhop *
            (T0 * pow(scale, 4) +
             Tp * (pow(scale, 2) * (pow(x[0], 2) + pow(x[1], 2)) -
                   2 * scale * (pow(x[0], 3) + pow(x[1], 3)) + pow(x[0], 4) +
                   pow(x[1], 4))) *
            pow(sin(M_PI * x[0] / scale), 2) * cos(M_PI * x[1] / scale)) /
       pow(scale, 5);

   src[3] =
       (1.0 / 2.0) *
       ((1.0 / 2.0) * M_PI * up * pow(scale, 2) *
            (rho0 + rhop * pow(sin(M_PI * x[0] / scale), 2) *
                        sin(M_PI * x[1] / scale)) *
            (-2 * pow(sin(2 * M_PI * x[0] / scale), 2) *
                 cos(M_PI * x[1] / scale) +
             cos(2 * M_PI * (x[0] - x[1]) / scale) -
             cos(2 * M_PI * (x[0] + x[1]) / scale)) *
            (2 * T0 * pow(scale, 4) +
             2 * Tp *
                 (pow(scale, 2) * (pow(x[0], 2) + pow(x[1], 2)) -
                  2 * scale * (pow(x[0], 3) + pow(x[1], 3)) + pow(x[0], 4) +
                  pow(x[1], 4)) +
             2 * (gamma - 1) *
                 (T0 * pow(scale, 4) +
                  Tp * (pow(scale, 2) * (pow(x[0], 2) + pow(x[1], 2)) -
                        2 * scale * (pow(x[0], 3) + pow(x[1], 3)) +
                        pow(x[0], 4) + pow(x[1], 4))) +
             (gamma - 1) * (pow(up, 2) * pow(scale, 4) *
                                pow(sin(2 * M_PI * x[0] / scale), 4) *
                                pow(sin(M_PI * x[1] / scale), 2) +
                            pow(4 * u0 * x[1] * (scale - x[1]) +
                                    up * pow(scale, 2) *
                                        pow(sin(M_PI * x[0] / scale), 2) *
                                        sin(2 * M_PI * x[1] / scale),
                                2))) -
        up * pow(scale, 2) *
            (4 * Tp * scale * x[1] * (gamma - 1) *
                 (rho0 + rhop * pow(sin(M_PI * x[0] / scale), 2) *
                             sin(M_PI * x[1] / scale)) *
                 (pow(scale, 2) - 3 * scale * x[1] + 2 * pow(x[1], 2)) +
             4 * Tp * scale * x[1] *
                 (rho0 + rhop * pow(sin(M_PI * x[0] / scale), 2) *
                             sin(M_PI * x[1] / scale)) *
                 (pow(scale, 2) - 3 * scale * x[1] + 2 * pow(x[1], 2)) +
             2 * M_PI * rhop * (gamma - 1) *
                 (T0 * pow(scale, 4) +
                  Tp * (pow(scale, 2) * (pow(x[0], 2) + pow(x[1], 2)) -
                        2 * scale * (pow(x[0], 3) + pow(x[1], 3)) +
                        pow(x[0], 4) + pow(x[1], 4))) *
                 pow(sin(M_PI * x[0] / scale), 2) * cos(M_PI * x[1] / scale) +
             M_PI * rhop * (gamma - 1) *
                 (pow(up, 2) * pow(scale, 4) *
                      pow(sin(2 * M_PI * x[0] / scale), 4) *
                      pow(sin(M_PI * x[1] / scale), 2) +
                  pow(4 * u0 * x[1] * (scale - x[1]) +
                          up * pow(scale, 2) *
                              pow(sin(M_PI * x[0] / scale), 2) *
                              sin(2 * M_PI * x[1] / scale),
                      2)) *
                 pow(sin(M_PI * x[0] / scale), 2) * cos(M_PI * x[1] / scale) +
             2 * M_PI * rhop *
                 (T0 * pow(scale, 4) +
                  Tp * (pow(scale, 2) * (pow(x[0], 2) + pow(x[1], 2)) -
                        2 * scale * (pow(x[0], 3) + pow(x[1], 3)) +
                        pow(x[0], 4) + pow(x[1], 4))) *
                 pow(sin(M_PI * x[0] / scale), 2) * cos(M_PI * x[1] / scale) +
             2 * scale * (gamma - 1) *
                 (rho0 + rhop * pow(sin(M_PI * x[0] / scale), 2) *
                             sin(M_PI * x[1] / scale)) *
                 (M_PI * pow(up, 2) * pow(scale, 3) *
                      pow(sin(2 * M_PI * x[0] / scale), 4) *
                      sin(M_PI * x[1] / scale) * cos(M_PI * x[1] / scale) +
                  2 * (2 * u0 * (scale - 2 * x[1]) + M_PI * up * scale * pow(sin(M_PI * x[0] / scale), 2) * cos(2 * M_PI * x[1] / scale)) *
                      (4 * u0 * x[1] * (scale - x[1]) +
                       up * pow(scale, 2) * pow(sin(M_PI * x[0] / scale), 2) *
                           sin(2 * M_PI * x[1] / scale)))) *
            pow(sin(2 * M_PI * x[0] / scale), 2) * sin(M_PI * x[1] / scale) +
        (4 * u0 * x[1] * (scale - x[1]) + up * pow(scale, 2) *
                                              pow(sin(M_PI * x[0] / scale), 2) *
                                              sin(2 * M_PI * x[1] / scale)) *
            (4 * Tp * scale * x[0] * (gamma - 1) *
                 (rho0 + rhop * pow(sin(M_PI * x[0] / scale), 2) *
                             sin(M_PI * x[1] / scale)) *
                 (pow(scale, 2) - 3 * scale * x[0] + 2 * pow(x[0], 2)) +
             4 * Tp * scale * x[0] *
                 (rho0 + rhop * pow(sin(M_PI * x[0] / scale), 2) *
                             sin(M_PI * x[1] / scale)) *
                 (pow(scale, 2) - 3 * scale * x[0] + 2 * pow(x[0], 2)) +
             4 * M_PI * up * pow(scale, 2) * (gamma - 1) *
                 (rho0 + rhop * pow(sin(M_PI * x[0] / scale), 2) *
                             sin(M_PI * x[1] / scale)) *
                 (2 * up * pow(scale, 2) *
                      pow(sin(2 * M_PI * x[0] / scale), 3) *
                      pow(sin(M_PI * x[1] / scale), 2) *
                      cos(2 * M_PI * x[0] / scale) +
                  (4 * u0 * x[1] * (scale - x[1]) +
                   up * pow(scale, 2) * pow(sin(M_PI * x[0] / scale), 2) *
                       sin(2 * M_PI * x[1] / scale)) *
                      sin(M_PI * x[0] / scale) * sin(2 * M_PI * x[1] / scale) *
                      cos(M_PI * x[0] / scale)) +
             M_PI * rhop * (gamma - 1) *
                 (T0 * pow(scale, 4) +
                  Tp * (pow(scale, 2) * (pow(x[0], 2) + pow(x[1], 2)) -
                        2 * scale * (pow(x[0], 3) + pow(x[1], 3)) +
                        pow(x[0], 4) + pow(x[1], 4))) *
                 (cos(M_PI * (2 * x[0] - x[1]) / scale) -
                  cos(M_PI * (2 * x[0] + x[1]) / scale)) +
             2 * M_PI * rhop * (gamma - 1) *
                 (pow(up, 2) * pow(scale, 4) *
                      pow(sin(2 * M_PI * x[0] / scale), 4) *
                      pow(sin(M_PI * x[1] / scale), 2) +
                  pow(4 * u0 * x[1] * (scale - x[1]) +
                          up * pow(scale, 2) *
                              pow(sin(M_PI * x[0] / scale), 2) *
                              sin(2 * M_PI * x[1] / scale),
                      2)) *
                 sin(M_PI * x[0] / scale) * sin(M_PI * x[1] / scale) *
                 cos(M_PI * x[0] / scale) +
             M_PI * rhop *
                 (T0 * pow(scale, 4) +
                  Tp * (pow(scale, 2) * (pow(x[0], 2) + pow(x[1], 2)) -
                        2 * scale * (pow(x[0], 3) + pow(x[1], 3)) +
                        pow(x[0], 4) + pow(x[1], 4))) *
                 (cos(M_PI * (2 * x[0] - x[1]) / scale) -
                  cos(M_PI * (2 * x[0] + x[1]) / scale)))) /
       (pow(scale, 7) * (gamma - 1));
}
#endif

}  // namespace mach

#endif
