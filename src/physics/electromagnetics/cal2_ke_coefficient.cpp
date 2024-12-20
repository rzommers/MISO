#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include "mfem.hpp"
#include "nlohmann/json.hpp"
#include "tinysplinecxx.h"

#include "cal2_ke_coefficient.hpp"
#include "utils.hpp"

#include "coefficient.hpp"
#include "miso_input.hpp"

namespace
{
class PolyVarEddyCurrentLossCoeff : public miso::ThreeStateCoefficient
{
public:
   /// \brief Define a model to represent the polynomial fit for the variable
   /// eddy current coefficient
   ///      empirically derived from a data source (NASA, Carpenter, ADA, etc.)
   /// \param[in] T0 - the lower temperature used for curve fitting kh(f,B) and
   /// ke(f,B) \param[in] ke_T0 - vector of variable eddy current loss
   /// coefficients at temperature T0, empirically found.
   ///                     ke_T0=[ke0_T0, ke1_T0, ke2_T0, ...],
   ///                     ke_T0(B)=ke0_T0+ke1_T0*B+ke2_T0*B^2...
   /// \param[in] T1 - the upper temperature used for curve fitting ke(f,B) and
   /// ke(f,B) \param[in] ke_T1 - vector of variable eddy current loss
   /// coefficients at temperature T1, empirically found.
   ///                     ke_T1=[ke0_T1, ke1_T1, ke2_T1, ...],
   ///                     ke_T1(B)=ke0_T1+ke1_T1*B+ke2_T1*B^2...
   PolyVarEddyCurrentLossCoeff(const double &T0,
                               const std::vector<double> &ke_T0,
                               const double &T1,
                               const std::vector<double> &ke_T1);

   /// \brief Evaluate the variable eddy current coefficient in the element
   /// described by trans at the point ip. \note When this method is called, the
   /// caller must make sure that the IntegrationPoint associated with trans is
   /// the same as ip. This can be achieved by calling trans.SetIntPoint(&ip).
   double Eval(mfem::ElementTransformation &trans,
               const mfem::IntegrationPoint &ip,
               double state1,
               double state2,
               double state3) override;

   /// \brief Evaluate the derivative of the variable eddy current coefficient
   /// in the element with respect to the 1st state variable
   ///   described by trans at the point ip.
   /// \note When this method is called, the caller must make sure that the
   /// IntegrationPoint associated with trans is the same as ip. This can be
   /// achieved by calling trans.SetIntPoint(&ip).
   double EvalDerivS1(mfem::ElementTransformation &trans,
                      const mfem::IntegrationPoint &ip,
                      double state1,
                      double state2,
                      double state3) override;

   /// \brief Evaluate the derivative of the variable eddy current coefficient
   /// in the element with respect to the 2nd state variable
   ///   described by trans at the point ip.
   /// \note When this method is called, the caller must make sure that the
   /// IntegrationPoint associated with trans is the same as ip. This can be
   /// achieved by calling trans.SetIntPoint(&ip).
   double EvalDerivS2(mfem::ElementTransformation &trans,
                      const mfem::IntegrationPoint &ip,
                      double state1,
                      double state2,
                      double state3) override;

   /// \brief Evaluate the derivative of the variable eddy current coefficient
   /// in the element with respect to the 3rd state variable
   ///   described by trans at the point ip.
   /// \note When this method is called, the caller must make sure that the
   /// IntegrationPoint associated with trans is the same as ip. This can be
   /// achieved by calling trans.SetIntPoint(&ip).
   double EvalDerivS3(mfem::ElementTransformation &trans,
                      const mfem::IntegrationPoint &ip,
                      double state1,
                      double state2,
                      double state3) override;

   /// \brief Evaluate the second derivative of the variable eddy current
   /// coefficient in the element with respect to the 1st state variable
   ///   described by trans at the point ip.
   /// \note When this method is called, the caller must make sure that the
   /// IntegrationPoint associated with trans is the same as ip. This can be
   /// achieved by calling trans.SetIntPoint(&ip).
   double Eval2ndDerivS1(mfem::ElementTransformation &trans,
                         const mfem::IntegrationPoint &ip,
                         double state1,
                         double state2,
                         double state3) override;

   /// \brief Evaluate the derivative of the variable eddy current coefficient
   /// in the element with respect to the 2nd state variable
   ///   described by trans at the point ip.
   /// \note When this method is called, the caller must make sure that the
   /// IntegrationPoint associated with trans is the same as ip. This can be
   /// achieved by calling trans.SetIntPoint(&ip).
   double Eval2ndDerivS2(mfem::ElementTransformation &trans,
                         const mfem::IntegrationPoint &ip,
                         double state1,
                         double state2,
                         double state3) override;

   /// \brief Evaluate the derivative of the variable eddy current coefficient
   /// in the element with respect to the 3rd state variable
   ///   described by trans at the point ip.
   /// \note When this method is called, the caller must make sure that the
   /// IntegrationPoint associated with trans is the same as ip. This can be
   /// achieved by calling trans.SetIntPoint(&ip).
   double Eval2ndDerivS3(mfem::ElementTransformation &trans,
                         const mfem::IntegrationPoint &ip,
                         double state1,
                         double state2,
                         double state3) override;

   /// \brief Evaluate the second derivative of the variable eddy current
   /// coefficient in the element with respect to the 1st then 2nd state
   /// variable
   ///   described by trans at the point ip.
   /// \note When this method is called, the caller must make sure that the
   /// IntegrationPoint associated with trans is the same as ip. This can be
   /// achieved by calling trans.SetIntPoint(&ip).
   double Eval2ndDerivS1S2(mfem::ElementTransformation &trans,
                           const mfem::IntegrationPoint &ip,
                           double state1,
                           double state2,
                           double state3) override;

   /// \brief Evaluate the second derivative of the variable eddy current
   /// coefficient in the element with respect to the 1st then 3rd state
   /// variable
   ///   described by trans at the point ip.
   /// \note When this method is called, the caller must make sure that the
   /// IntegrationPoint associated with trans is the same as ip. This can be
   /// achieved by calling trans.SetIntPoint(&ip).
   double Eval2ndDerivS1S3(mfem::ElementTransformation &trans,
                           const mfem::IntegrationPoint &ip,
                           double state1,
                           double state2,
                           double state3) override;

   /// \brief Evaluate the second derivative of the variable eddy current
   /// coefficient in the element with respect to the 2nd then 3rd state
   /// variable
   ///   described by trans at the point ip.
   /// \note When this method is called, the caller must make sure that the
   /// IntegrationPoint associated with trans is the same as ip. This can be
   /// achieved by calling trans.SetIntPoint(&ip).
   double Eval2ndDerivS2S3(mfem::ElementTransformation &trans,
                           const mfem::IntegrationPoint &ip,
                           double state1,
                           double state2,
                           double state3) override;

   /// TODO: Likely not necessary because of Eval2ndDerivS1S2
   ///  \brief Evaluate the derivative of the variable eddy current coefficient
   ///  in the element with respect to the 2nd then 1st state variable
   ///    described by trans at the point ip.
   ///  \note When this method is called, the caller must make sure that the
   ///  IntegrationPoint associated with trans is the same as ip. This can be
   ///  achieved by calling trans.SetIntPoint(&ip).
   double Eval2ndDerivS2S1(mfem::ElementTransformation &trans,
                           const mfem::IntegrationPoint &ip,
                           double state1,
                           double state2,
                           double state3) override;

   /// TODO: Likely not necessary because of Eval2ndDerivS1S3
   ///  \brief Evaluate the derivative of the variable eddy current coefficient
   ///  in the element with respect to the 3rd then 1st state variable
   ///    described by trans at the point ip.
   ///  \note When this method is called, the caller must make sure that the
   ///  IntegrationPoint associated with trans is the same as ip. This can be
   ///  achieved by calling trans.SetIntPoint(&ip).
   double Eval2ndDerivS3S1(mfem::ElementTransformation &trans,
                           const mfem::IntegrationPoint &ip,
                           double state1,
                           double state2,
                           double state3) override;

   /// TODO: Likely not necessary because of Eval2ndDerivS2S3
   ///  \brief Evaluate the derivative of the variable eddy current coefficient
   ///  in the element with respect to the 3rd then 2nd state variable
   ///    described by trans at the point ip.
   ///  \note When this method is called, the caller must make sure that the
   ///  IntegrationPoint associated with trans is the same as ip. This can be
   ///  achieved by calling trans.SetIntPoint(&ip).
   double Eval2ndDerivS3S2(mfem::ElementTransformation &trans,
                           const mfem::IntegrationPoint &ip,
                           double state1,
                           double state2,
                           double state3) override;

   /// TODO: Adapt EvalRevDiff if needed for variable eddy current coefficient
   void EvalRevDiff(const double Q_bar,
                    mfem::ElementTransformation &trans,
                    const mfem::IntegrationPoint &ip,
                    mfem::DenseMatrix &PointMat_bar) override
   { }

protected:
   double T0;
   std::vector<double> ke_T0;
   double T1;
   std::vector<double> ke_T1;
};

/// TODO: this default variable eddy current loss coefficient will need to be
/// altered
/// TODO: this will need some work -- COME BACK TO THIS (possibly even remove)
std::unique_ptr<mfem::Coefficient> constructDefaultCAL2_keCoeff(
    const std::string &material_name,
    const nlohmann::json &materials)
{
   auto CAL2_ke = materials[material_name].value("ke", 1.0);
   return std::make_unique<mfem::ConstantCoefficient>(CAL2_ke);
}

// Get T0, ke_T0, T1, and ke_T1 from JSON
/// TODO: Add in argument of const std::string &model if decide to combine
/// Steinmetz and CAL2 into one
void getTsAndKes(const nlohmann::json &material,
                 const nlohmann::json &materials,
                 double &T0,
                 std::vector<double> &ke_T0,
                 double &T1,
                 std::vector<double> &ke_T1)
{
   const auto &material_name = material["name"].get<std::string>();

   // Assign T0 based on material options, else refer to material library
   if (material["core_loss"].contains("T0"))
   {
      T0 = material["core_loss"]["T0"].get<double>();
   }
   else
   {
      T0 = materials[material_name]["core_loss"]["CAL2"]["T0"].get<double>();
   }

   // Assign ke_T0 based on material options, else refer to material library
   if (material["core_loss"].contains("ke_T0"))
   {
      ke_T0 = material["core_loss"]["ke_T0"].get<std::vector<double>>();
   }
   else
   {
      ke_T0 = materials[material_name]["core_loss"]["CAL2"]["ke_T0"]
                  .get<std::vector<double>>();
   }

   // Assign T1 based on material options, else refer to material library
   if (material["core_loss"].contains("T1"))
   {
      T1 = material["core_loss"]["T1"].get<double>();
   }
   else
   {
      T1 = materials[material_name]["core_loss"]["CAL2"]["T1"].get<double>();
   }

   // Assign ke_T1 based on material options, else refer to material library
   if (material["core_loss"].contains("ke_T1"))
   {
      ke_T1 = material["core_loss"]["ke_T1"].get<std::vector<double>>();
   }
   else
   {
      ke_T1 = materials[material_name]["core_loss"]["CAL2"]["ke_T1"]
                  .get<std::vector<double>>();
   }
}

// Construct the ke coefficient
std::unique_ptr<mfem::Coefficient> constructCAL2_keCoeff(
    const nlohmann::json &component,
    const nlohmann::json &materials)
{
   std::unique_ptr<mfem::Coefficient>
       temp_coeff;  // temp=temporary, not temperature
   const auto &material = component["material"];

   /// If "material" is a string, it is interpreted to be the name of a
   /// material. We default to a CAL2_ke coeff of 1 ///TODO: (change this value
   /// as needed) unless there is a different value in the material library
   if (material.is_string())
   {
      const auto &material_name = material.get<std::string>();
      /// TODO: Ensure this Default CAL2_ke coefficient is in order
      temp_coeff = constructDefaultCAL2_keCoeff(material_name, materials);
   }
   else
   {
      const auto &material_name = material["name"].get<std::string>();

      if (material.contains("core_loss"))
      {
         // Declare variables
         double T0;
         std::vector<double> ke_T0;
         double T1;
         std::vector<double> ke_T1;

         // Obtain the necessary parameters from the JSON
         getTsAndKes(material, materials, T0, ke_T0, T1, ke_T1);

         // Can now construct the coefficient accordingly
         temp_coeff = std::make_unique<PolyVarEddyCurrentLossCoeff>(
             T0, ke_T0, T1, ke_T1);

         /// TODO: Add this error handing in if add in multiple models or
         /// combine Steinmetz and CAL2 into one
         // else
         // {
         //    std::string error_msg =
         //          "Insufficient information to compute CAL2 variable eddy
         //          current loss coefficient for material \"";
         //    error_msg += material_name;
         //    error_msg += "\"!\n";
         //    throw miso::MISOException(error_msg);
         // }
      }
      else
      {
         // Doesn't have the core loss JSON structure; assign it default
         // coefficient
         temp_coeff = constructDefaultCAL2_keCoeff(material_name, materials);
      }
   }
   return temp_coeff;
}

}  // anonymous namespace

namespace miso
{
double CAL2keCoefficient::Eval(mfem::ElementTransformation &trans,
                               const mfem::IntegrationPoint &ip)
{
   return CAL2_ke.Eval(trans, ip);
}

double CAL2keCoefficient::Eval(mfem::ElementTransformation &trans,
                               const mfem::IntegrationPoint &ip,
                               double state1,
                               double state2,
                               double state3)
{
   return CAL2_ke.Eval(trans, ip, state1, state2, state3);
}

double CAL2keCoefficient::EvalDerivS1(mfem::ElementTransformation &trans,
                                      const mfem::IntegrationPoint &ip,
                                      double state1,
                                      double state2,
                                      double state3)
{
   return CAL2_ke.EvalDerivS1(trans, ip, state1, state2, state3);
}

double CAL2keCoefficient::EvalDerivS2(mfem::ElementTransformation &trans,
                                      const mfem::IntegrationPoint &ip,
                                      double state1,
                                      double state2,
                                      double state3)
{
   return CAL2_ke.EvalDerivS2(trans, ip, state1, state2, state3);
}

double CAL2keCoefficient::EvalDerivS3(mfem::ElementTransformation &trans,
                                      const mfem::IntegrationPoint &ip,
                                      double state1,
                                      double state2,
                                      double state3)
{
   return CAL2_ke.EvalDerivS3(trans, ip, state1, state2, state3);
}

double CAL2keCoefficient::Eval2ndDerivS1(mfem::ElementTransformation &trans,
                                         const mfem::IntegrationPoint &ip,
                                         double state1,
                                         double state2,
                                         double state3)
{
   return CAL2_ke.Eval2ndDerivS1(trans, ip, state1, state2, state3);
}

double CAL2keCoefficient::Eval2ndDerivS2(mfem::ElementTransformation &trans,
                                         const mfem::IntegrationPoint &ip,
                                         double state1,
                                         double state2,
                                         double state3)
{
   return CAL2_ke.Eval2ndDerivS2(trans, ip, state1, state2, state3);
}

double CAL2keCoefficient::Eval2ndDerivS3(mfem::ElementTransformation &trans,
                                         const mfem::IntegrationPoint &ip,
                                         double state1,
                                         double state2,
                                         double state3)
{
   return CAL2_ke.Eval2ndDerivS3(trans, ip, state1, state2, state3);
}

double CAL2keCoefficient::Eval2ndDerivS1S2(mfem::ElementTransformation &trans,
                                           const mfem::IntegrationPoint &ip,
                                           double state1,
                                           double state2,
                                           double state3)
{
   return CAL2_ke.Eval2ndDerivS1S2(trans, ip, state1, state2, state3);
}

double CAL2keCoefficient::Eval2ndDerivS1S3(mfem::ElementTransformation &trans,
                                           const mfem::IntegrationPoint &ip,
                                           double state1,
                                           double state2,
                                           double state3)
{
   return CAL2_ke.Eval2ndDerivS1S3(trans, ip, state1, state2, state3);
}

double CAL2keCoefficient::Eval2ndDerivS2S3(mfem::ElementTransformation &trans,
                                           const mfem::IntegrationPoint &ip,
                                           double state1,
                                           double state2,
                                           double state3)
{
   return CAL2_ke.Eval2ndDerivS2S3(trans, ip, state1, state2, state3);
}

/// TODO: Likely not necessary because of Eval2ndDerivS1S2
double CAL2keCoefficient::Eval2ndDerivS2S1(mfem::ElementTransformation &trans,
                                           const mfem::IntegrationPoint &ip,
                                           double state1,
                                           double state2,
                                           double state3)
{
   return CAL2_ke.Eval2ndDerivS2S1(trans, ip, state1, state2, state3);
}

/// TODO: Likely not necessary because of Eval2ndDerivS1S3
double CAL2keCoefficient::Eval2ndDerivS3S1(mfem::ElementTransformation &trans,
                                           const mfem::IntegrationPoint &ip,
                                           double state1,
                                           double state2,
                                           double state3)
{
   return CAL2_ke.Eval2ndDerivS3S1(trans, ip, state1, state2, state3);
}

/// TODO: Likely not necessary because of Eval2ndDerivS2S3
double CAL2keCoefficient::Eval2ndDerivS3S2(mfem::ElementTransformation &trans,
                                           const mfem::IntegrationPoint &ip,
                                           double state1,
                                           double state2,
                                           double state3)
{
   return CAL2_ke.Eval2ndDerivS3S2(trans, ip, state1, state2, state3);
}

/// TODO: Adapt if needed
void CAL2keCoefficient::EvalRevDiff(const double Q_bar,
                                    mfem::ElementTransformation &trans,
                                    const mfem::IntegrationPoint &ip,
                                    mfem::DenseMatrix &PointMat_bar)
{
   CAL2_ke.EvalRevDiff(Q_bar, trans, ip, PointMat_bar);
}

/// TODO: Change CAL2_ke(std::make_unique<mfem::ConstantCoefficient>(1.0)) line
/// IF the equivalent lines... std::unique_ptr<mfem::Coefficient>
/// constructDefaultCAL2_keCoeff( from earlier change
CAL2keCoefficient::CAL2keCoefficient(const nlohmann::json &CAL2_ke_options,
                                     const nlohmann::json &materials)
 : CAL2_ke(std::make_unique<mfem::ConstantCoefficient>(1.0))
{
   if (CAL2_ke_options.contains("components"))
   {
      /// Options are being passed in. Loop over the components within and
      /// construct a CAL2_ke loss coefficient for each
      for (const auto &component : CAL2_ke_options["components"])
      {
         int attr = component.value("attr", -1);
         if (-1 != attr)
         {
            CAL2_ke.addCoefficient(
                attr, constructDefaultCAL2_keCoeff(component, materials));
         }
         else
         {
            for (const auto &attribute : component["attrs"])
            {
               CAL2_ke.addCoefficient(
                   attribute, constructCAL2_keCoeff(component, materials));
            }
         }
      }
   }
   else
   {
      /// Components themselves are being passed in. Loop over the components
      /// and construct a CAL2_ke loss coefficient for each
      auto components = CAL2_ke_options;
      for (const auto &component : components)
      {
         int attr = component.value("attr", -1);
         if (-1 != attr)
         {
            CAL2_ke.addCoefficient(
                attr, constructDefaultCAL2_keCoeff(component, materials));
         }
         else
         {
            for (const auto &attribute : component["attrs"])
            {
               CAL2_ke.addCoefficient(
                   attribute, constructCAL2_keCoeff(component, materials));
            }
         }
      }
   }
}

}  // namespace miso

namespace
{
PolyVarEddyCurrentLossCoeff::PolyVarEddyCurrentLossCoeff(
    const double &T0,
    const std::vector<double> &ke_T0,
    const double &T1,
    const std::vector<double> &ke_T1)
 : T0(T0), ke_T0(ke_T0), T1(T1), ke_T1(ke_T1)

{ }

double PolyVarEddyCurrentLossCoeff::Eval(mfem::ElementTransformation &trans,
                                         const mfem::IntegrationPoint &ip,
                                         const double state1,
                                         const double state2,
                                         const double state3)
{
   // Assuming state1=temperature, state2=frequency, state3=max alternating flux
   // density
   auto T = state1;
   // auto f = state2;
   auto B_m = state3;

   double ke_T0_f_B = 0.0;
   for (int i = 0; i < static_cast<int>(ke_T0.size()); ++i)
   {
      ke_T0_f_B += ke_T0[i] * std::pow(B_m, i);
   }
   double ke_T1_f_B = 0.0;
   for (int i = 0; i < static_cast<int>(ke_T1.size()); ++i)
   {
      ke_T1_f_B += ke_T1[i] * std::pow(B_m, i);
   }
   double D_eddy = (ke_T1_f_B - ke_T0_f_B) / ((T1 - T0) * ke_T0_f_B);
   double kte = 1 + (T - T0) * D_eddy;

   double CAL2_ke = kte * ke_T0_f_B;

   return CAL2_ke;
}

double PolyVarEddyCurrentLossCoeff::EvalDerivS1(
    mfem::ElementTransformation &trans,
    const mfem::IntegrationPoint &ip,
    const double state1,
    const double state2,
    const double state3)
{
   // Assuming state1=temperature, state2=frequency, state3=max alternating flux
   // density First derivative with respect to temperature

   auto T = state1;
   // auto f = state2;
   auto B_m = state3;

   double ke_T0_f_B = 0.0;
   for (int i = 0; i < static_cast<int>(ke_T0.size()); ++i)
   {
      ke_T0_f_B += ke_T0[i] * std::pow(B_m, i);
   }
   double ke_T1_f_B = 0.0;
   for (int i = 0; i < static_cast<int>(ke_T1.size()); ++i)
   {
      ke_T1_f_B += ke_T1[i] * std::pow(B_m, i);
   }
   double D_eddy = (ke_T1_f_B - ke_T0_f_B) / ((T1 - T0) * ke_T0_f_B);
   // double kte = 1+(T-T0)*D_eddy;
   double dktedT = D_eddy;

   // double CAL2_ke = kte*ke_T0_f_B;

   double dCAL2_kedT = dktedT * ke_T0_f_B;
   return dCAL2_kedT;
}

double PolyVarEddyCurrentLossCoeff::EvalDerivS2(
    mfem::ElementTransformation &trans,
    const mfem::IntegrationPoint &ip,
    const double state1,
    const double state2,
    const double state3)
{
   // Assuming state1=temperature, state2=frequency, state3=max alternating flux
   // density First derivative with respect to frequency

   // Frequency is not explicitly used to calculate the coefficient itself.
   // Frequency is used to explicitly calculate the specific core loss (next
   // level up) Therefore, all CAL2 coefficient derivatives with respect to
   // frequency are 0
   double dCAL2_kedf = 0;
   return dCAL2_kedf;
}

double PolyVarEddyCurrentLossCoeff::EvalDerivS3(
    mfem::ElementTransformation &trans,
    const mfem::IntegrationPoint &ip,
    const double state1,
    const double state2,
    const double state3)
{
   // Assuming state1=temperature, state2=frequency, state3=max alternating flux
   // density First derivative with respect to max alternating flux density

   auto T = state1;
   // auto f = state2;
   auto B_m = state3;

   // double ke_T0_f_B = 0.0;
   double dke_T0_f_BdB_m = 0.0;
   for (int i = 0; i < static_cast<int>(ke_T0.size()); ++i)
   {
      // ke_T0_f_B += ke_T0[i]*std::pow(B_m,i);
      dke_T0_f_BdB_m += i * ke_T0[i] * std::pow(B_m, i - 1);
   }
   // double ke_T1_f_B = 0.0;
   double dke_T1_f_BdB_m = 0.0;
   for (int i = 0; i < static_cast<int>(ke_T1.size()); ++i)
   {
      // ke_T1_f_B += ke_T1[i]*std::pow(B_m,i);
      dke_T1_f_BdB_m += i * ke_T1[i] * std::pow(B_m, i - 1);
   }
   // double D_eddy = (ke_T1_f_B-ke_T0_f_B)/((T1-T0)*ke_T0_f_B);
   // double kte = 1+(T-T0)*D_eddy;

   // double CAL2_ke = kte*ke_T0_f_B;

   double dCAL2_kedB_m = dke_T0_f_BdB_m + ((T - T0) / (T1 - T0)) *
                                              (dke_T1_f_BdB_m - dke_T0_f_BdB_m);
   return dCAL2_kedB_m;
}

double PolyVarEddyCurrentLossCoeff::Eval2ndDerivS1(
    mfem::ElementTransformation &trans,
    const mfem::IntegrationPoint &ip,
    const double state1,
    const double state2,
    const double state3)
{
   // Assuming state1=temperature, state2=frequency, state3=max alternating flux
   // density Second derivative with respect to temperature

   // double D_eddy = (ke_T1_f_B-ke_T0_f_B)/((T1-T0)*ke_T0_f_B);
   // double kte = 1+(T-T0)*D_eddy;
   // double dktedT = D_eddy;

   // double CAL2_ke = kte*ke_T0_f_B;
   // double dCAL2_kedT = dktedT*ke_T0_f_B;

   // As seen, CAL2 coefficient merely linear in temperature
   // Thus, 2nd and higher order derivatives w/r/t temperature will be 0
   double d2CAL2_kedT2 = 0;
   return d2CAL2_kedT2;
}

double PolyVarEddyCurrentLossCoeff::Eval2ndDerivS2(
    mfem::ElementTransformation &trans,
    const mfem::IntegrationPoint &ip,
    const double state1,
    const double state2,
    const double state3)
{
   // Assuming state1=temperature, state2=frequency, state3=max alternating flux
   // density Second derivative with respect to frequency

   // Frequency is not explicitly used to calculate the coefficient itself.
   // Frequency is used to explicitly calculate the specific core loss (next
   // level up) Therefore, all CAL2 coefficient derivatives with respect to
   // frequency are 0
   double d2CAL2_kedf2 = 0;
   return d2CAL2_kedf2;
}

double PolyVarEddyCurrentLossCoeff::Eval2ndDerivS3(
    mfem::ElementTransformation &trans,
    const mfem::IntegrationPoint &ip,
    const double state1,
    const double state2,
    const double state3)
{
   // Assuming state1=temperature, state2=frequency, state3=max alternating flux
   // density Second derivative with respect to max alternating flux density

   auto T = state1;
   // auto f = state2;
   auto B_m = state3;

   // double dke_T0_f_BdB_m = 0.0;
   double d2ke_T0_f_BdB_m2 = 0.0;
   for (int i = 0; i < static_cast<int>(ke_T0.size()); ++i)
   {
      // dke_T0_f_BdB_m += i*ke_T0[i]*std::pow(B_m,i-1);
      d2ke_T0_f_BdB_m2 += (i - 1) * i * ke_T0[i] * std::pow(B_m, i - 2);
   }
   double dke_T1_f_BdB_m = 0.0;
   double d2ke_T1_f_BdB_m2 = 0.0;
   for (int i = 0; i < static_cast<int>(ke_T1.size()); ++i)
   {
      // dke_T1_f_BdB_m += i*ke_T1[i]*std::pow(B_m,i-1);
      d2ke_T1_f_BdB_m2 += (i - 1) * i * ke_T1[i] * std::pow(B_m, i - 2);
   }
   // double D_eddy = (ke_T1_f_B-ke_T0_f_B)/((T1-T0)*ke_T0_f_B);
   // double kte = 1+(T-T0)*D_eddy;

   // double CAL2_ke = kte*ke_T0_f_B;

   double d2CAL2_kedB_m2 =
       d2ke_T0_f_BdB_m2 +
       ((T - T0) / (T1 - T0)) * (d2ke_T1_f_BdB_m2 - d2ke_T0_f_BdB_m2);
   return d2CAL2_kedB_m2;
}

double PolyVarEddyCurrentLossCoeff::Eval2ndDerivS1S2(
    mfem::ElementTransformation &trans,
    const mfem::IntegrationPoint &ip,
    const double state1,
    const double state2,
    const double state3)
{
   // Assuming state1=temperature, state2=frequency, state3=max alternating flux
   // density Derivative with respect to temperature then frequency

   // Frequency is not explicitly used to calculate the coefficient itself.
   // Frequency is used to explicitly calculate the specific core loss (next
   // level up) Therefore, all CAL2 coefficient derivatives with respect to
   // frequency are 0
   double d2CAL2_kedTdf = 0;
   return d2CAL2_kedTdf;
}

double PolyVarEddyCurrentLossCoeff::Eval2ndDerivS1S3(
    mfem::ElementTransformation &trans,
    const mfem::IntegrationPoint &ip,
    const double state1,
    const double state2,
    const double state3)
{
   // Assuming state1=temperature, state2=frequency, state3=max alternating flux
   // density Derivative with respect to temperature then max alternating flux
   // density

   auto T = state1;
   // auto f = state2;
   auto B_m = state3;

   // double ke_T0_f_B = 0.0;
   double dke_T0_f_BdB_m = 0.0;
   for (int i = 0; i < static_cast<int>(ke_T0.size()); ++i)
   {
      // ke_T0_f_B += ke_T0[i]*std::pow(B_m,i);
      dke_T0_f_BdB_m += i * ke_T0[i] * std::pow(B_m, i - 1);
   }
   // double ke_T1_f_B = 0.0;
   double dke_T1_f_BdB_m = 0.0;
   for (int i = 0; i < static_cast<int>(ke_T1.size()); ++i)
   {
      // ke_T1_f_B += ke_T1[i]*std::pow(B_m,i);
      dke_T1_f_BdB_m += i * ke_T1[i] * std::pow(B_m, i - 1);
   }
   // double D_eddy = (ke_T1_f_B-ke_T0_f_B)/((T1-T0)*ke_T0_f_B);
   // double kte = 1+(T-T0)*D_eddy;
   // double dktedT = D_eddy;

   // double CAL2_ke = kte*ke_T0_f_B;
   // double dCAL2_kedT = dktedT*ke_T0_f_B = (ke_T1_f_B-ke_T0_f_B)/(T1-T0);

   double d2CAL2_kedTdB_m = (dke_T1_f_BdB_m - dke_T0_f_BdB_m) / (T1 - T0);
   return d2CAL2_kedTdB_m;
}

double PolyVarEddyCurrentLossCoeff::Eval2ndDerivS2S3(
    mfem::ElementTransformation &trans,
    const mfem::IntegrationPoint &ip,
    const double state1,
    const double state2,
    const double state3)
{
   // Assuming state1=temperature, state2=frequency, state3=max alternating flux
   // density Derivative with respect to frequency then max alternating flux
   // density

   // Frequency is not explicitly used to calculate the coefficient itself.
   // Frequency is used to explicitly calculate the specific core loss (next
   // level up) Therefore, all CAL2 coefficient derivatives with respect to
   // frequency are 0
   double d2CAL2_kedfdB_m = 0;
   return d2CAL2_kedfdB_m;
}

/// TODO: Likely not necessary because of Eval2ndDerivS1S2
double PolyVarEddyCurrentLossCoeff::Eval2ndDerivS2S1(
    mfem::ElementTransformation &trans,
    const mfem::IntegrationPoint &ip,
    const double state1,
    const double state2,
    const double state3)
{
   // Assuming state1=temperature, state2=frequency, state3=max alternating flux
   // density Derivative with respect to frequency then temperature

   // Frequency is not explicitly used to calculate the coefficient itself.
   // Frequency is used to explicitly calculate the specific core loss (next
   // level up) Therefore, all CAL2 coefficient derivatives with respect to
   // frequency are 0
   double d2CAL2_kedfdT = 0;
   return d2CAL2_kedfdT;
}

/// TODO: Likely not necessary because of Eval2ndDerivS1S3
double PolyVarEddyCurrentLossCoeff::Eval2ndDerivS3S1(
    mfem::ElementTransformation &trans,
    const mfem::IntegrationPoint &ip,
    const double state1,
    const double state2,
    const double state3)
{
   // Assuming state1=temperature, state2=frequency, state3=max alternating flux
   // density Derivative with respect to max alternating flux density then
   // temperature

   auto T = state1;
   // auto f = state2;
   auto B_m = state3;

   // double ke_T0_f_B = 0.0;
   double dke_T0_f_BdB_m = 0.0;
   for (int i = 0; i < static_cast<int>(ke_T0.size()); ++i)
   {
      // ke_T0_f_B += ke_T0[i]*std::pow(B_m,i);
      dke_T0_f_BdB_m += i * ke_T0[i] * std::pow(B_m, i - 1);
   }
   // double ke_T1_f_B = 0.0;
   double dke_T1_f_BdB_m = 0.0;
   for (int i = 0; i < static_cast<int>(ke_T1.size()); ++i)
   {
      // ke_T1_f_B += ke_T1[i]*std::pow(B_m,i);
      dke_T1_f_BdB_m += i * ke_T1[i] * std::pow(B_m, i - 1);
   }
   // double D_eddy = (ke_T1_f_B-ke_T0_f_B)/((T1-T0)*ke_T0_f_B);
   // double kte = 1+(T-T0)*D_eddy;
   // double dktedT = D_eddy;

   // double CAL2_ke = kte*ke_T0_f_B;
   // double dCAL2_kedB_m =
   // dke_T0_f_BdB_m+((T-T0)/(T1-T0))*(dke_T1_f_BdB_m-dke_T0_f_BdB_m);

   double d2CAL2_kedB_mdT = (dke_T1_f_BdB_m - dke_T0_f_BdB_m) / (T1 - T0);
   return d2CAL2_kedB_mdT;
}

/// TODO: Likely not necessary because of Eval2ndDerivS2S3
double PolyVarEddyCurrentLossCoeff::Eval2ndDerivS3S2(
    mfem::ElementTransformation &trans,
    const mfem::IntegrationPoint &ip,
    const double state1,
    const double state2,
    const double state3)
{
   // Assuming state1=temperature, state2=frequency, state3=max alternating flux
   // density Derivative with respect to max alternating flux density then
   // frequency

   // Frequency is not explicitly used to calculate the coefficient itself.
   // Frequency is used to explicitly calculate the specific core loss (next
   // level up) Therefore, all CAL2 coefficient derivatives with respect to
   // frequency are 0
   double d2CAL2_kedB_mdf = 0;
   return d2CAL2_kedB_mdf;
}

/// TODO: is there a need to code EvalRevDiff for variable eddy current
/// coefficient method here? I'm thinking not

}  // namespace
