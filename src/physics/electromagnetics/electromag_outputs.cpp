#include <cmath>
#include <string>

#include "miso_residual.hpp"
#include "mfem.hpp"
#include "nlohmann/json.hpp"

#include "cal2_kh_coefficient.hpp"
#include "cal2_ke_coefficient.hpp"
#include "coefficient.hpp"
#include "common_outputs.hpp"
#include "data_logging.hpp"
#include "electromag_integ.hpp"
#include "functional_output.hpp"
#include "miso_input.hpp"
#include "miso_output.hpp"

#include "pm_demag_constraint_coeff.hpp"

#include "electromag_outputs.hpp"

namespace miso
{

void setOptions(ForceFunctional &output, const nlohmann::json &options)
{
   auto &&attrs = options["attributes"].get<std::unordered_set<int>>();
   auto &&axis = options["axis"].get<std::vector<double>>();

   auto space_dim = output.fields.at("vforce").mesh().SpaceDimension();
   mfem::VectorConstantCoefficient axis_vector(
       mfem::Vector(axis.data(), space_dim));

   auto &v = output.fields.at("vforce").gridFunc();
   v = 0.0;
   for (const auto &attr : attrs)
   {
      v.ProjectCoefficient(axis_vector, attr);
   }
}

void setOptions(TorqueFunctional &output, const nlohmann::json &options)
{
   auto &&attrs = options["attributes"].get<std::unordered_set<int>>();
   auto &&axis = options["axis"].get<std::vector<double>>();
   auto &&about = options["about"].get<std::vector<double>>();
   mfem::Vector axis_vector(axis.data(), axis.size());
   axis_vector /= axis_vector.Norml2();

   auto space_dim = output.fields.at("vtorque").mesh().SpaceDimension();
   mfem::Vector about_vector(about.data(), space_dim);
   double r_data[3];
   mfem::Vector r(r_data, space_dim);

   mfem::VectorFunctionCoefficient v_vector(
       space_dim,
       [&axis_vector, &about_vector, &r, space_dim](const mfem::Vector &x,
                                                    mfem::Vector &v)
       {
          subtract(x, about_vector, r);
          if (space_dim == 3)
          {
             // r /= r.Norml2();
             v(0) = axis_vector(1) * r(2) - axis_vector(2) * r(1);
             v(1) = axis_vector(2) * r(0) - axis_vector(0) * r(2);
             v(2) = axis_vector(0) * r(1) - axis_vector(1) * r(0);
          }
          else
          {
             v(0) = -axis_vector(2) * r(1);
             v(1) = axis_vector(2) * r(0);
          }
          // if (v.Norml2() > 1e-12)
          //    v /= v.Norml2();
       });

   auto &v = output.fields.at("vtorque").gridFunc();
   v = 0.0;
   for (const auto &attr : attrs)
   {
      v.ProjectCoefficient(v_vector, attr);
   }
}

double calcOutput(DCLossFunctional &output, const MISOInputs &inputs)
{
   setInputs(output, inputs);

   double rho = calcOutput(output.resistivity, inputs);

   double strand_area = M_PI * pow(output.strand_radius, 2);
   double R = output.wire_length * rho / (strand_area * output.strands_in_hand);

   double loss = pow(output.rms_current, 2) * R * sqrt(2);

   double volume = calcOutput(output.volume, inputs);

   return loss / volume;
}

double jacobianVectorProduct(DCLossFunctional &output,
                             const mfem::Vector &wrt_dot,
                             const std::string &wrt)
{
   const MISOInputs &inputs = *output.inputs;
   if (wrt.rfind("wire_length", 0) == 0)
   {
      double rho = calcOutput(output.resistivity, inputs);

      double strand_area = M_PI * pow(output.strand_radius, 2);
      // double R =
      //     output.wire_length * rho / (strand_area * output.strands_in_hand);
      double R_dot = rho / (strand_area * output.strands_in_hand) * wrt_dot(0);

      // double loss = pow(output.rms_current, 2) * R * sqrt(2);
      double loss_dot = pow(output.rms_current, 2) * sqrt(2) * R_dot;

      double volume = calcOutput(output.volume, inputs);

      // double dc_loss = loss / volume;
      double dc_loss_dot = 1 / volume * loss_dot;

      return dc_loss_dot;
   }
   else if (wrt.rfind("rms_current", 0) == 0)
   {
      double rho = calcOutput(output.resistivity, inputs);

      double strand_area = M_PI * pow(output.strand_radius, 2);
      double R =
          output.wire_length * rho / (strand_area * output.strands_in_hand);

      // double loss = pow(output.rms_current, 2) * R * sqrt(2);
      double loss_dot = 2 * output.rms_current * R * sqrt(2) * wrt_dot(0);

      double volume = calcOutput(output.volume, inputs);

      // double dc_loss = loss / volume;
      double dc_loss_dot = 1 / volume * loss_dot;

      return dc_loss_dot;
   }
   else if (wrt.rfind("strand_radius", 0) == 0)
   {
      double rho = calcOutput(output.resistivity, inputs);

      double strand_area = M_PI * pow(output.strand_radius, 2);
      double strand_area_dot = M_PI * 2 * output.strand_radius * wrt_dot(0);

      // double R =
      //     output.wire_length * rho / (strand_area * output.strands_in_hand);
      double R_dot = -output.wire_length * rho /
                     (pow(strand_area, 2) * output.strands_in_hand) *
                     strand_area_dot;

      // double loss = pow(output.rms_current, 2) * R * sqrt(2);
      double loss_dot = pow(output.rms_current, 2) * sqrt(2) * R_dot;

      double volume = calcOutput(output.volume, inputs);

      // double dc_loss = loss / volume;
      double dc_loss_dot = 1 / volume * loss_dot;

      return dc_loss_dot;
   }
   else if (wrt.rfind("strands_in_hand", 0) == 0)
   {
      double rho = calcOutput(output.resistivity, inputs);

      double strand_area = M_PI * pow(output.strand_radius, 2);

      // double R =
      //     output.wire_length * rho / (strand_area * output.strands_in_hand);
      double R_dot = -output.wire_length * rho /
                     (strand_area * pow(output.strands_in_hand, 2)) *
                     wrt_dot(0);

      // double loss = pow(output.rms_current, 2) * R * sqrt(2);
      double loss_dot = pow(output.rms_current, 2) * sqrt(2) * R_dot;

      double volume = calcOutput(output.volume, inputs);

      // double dc_loss = loss / volume;
      double dc_loss_dot = 1 / volume * loss_dot;

      return dc_loss_dot;
   }
   else if (wrt.rfind("mesh_coords", 0) == 0)
   {
      double rho = calcOutput(output.resistivity, inputs);
      double rho_dot = jacobianVectorProduct(output.resistivity, wrt_dot, wrt);

      double strand_area = M_PI * pow(output.strand_radius, 2);

      double R =
          output.wire_length * rho / (strand_area * output.strands_in_hand);
      double R_dot =
          output.wire_length / (strand_area * output.strands_in_hand) * rho_dot;

      double loss = pow(output.rms_current, 2) * R * sqrt(2);
      double loss_dot = pow(output.rms_current, 2) * sqrt(2) * R_dot;

      double volume = calcOutput(output.volume, inputs);
      double volume_dot = jacobianVectorProduct(output.volume, wrt_dot, wrt);

      // double dc_loss = loss / volume;
      double dc_loss_dot =
          loss_dot / volume - loss / pow(volume, 2) * volume_dot;

      return dc_loss_dot;
   }
   else if (wrt.rfind("temperature", 0) == 0)
   {
      // double rho = calcOutput(output.resistivity, inputs);
      double rho_dot = jacobianVectorProduct(output.resistivity, wrt_dot, wrt);

      double strand_area = M_PI * pow(output.strand_radius, 2);

      // double R =
      //     output.wire_length * rho / (strand_area * output.strands_in_hand);
      double R_dot =
          output.wire_length / (strand_area * output.strands_in_hand) * rho_dot;

      // double loss = pow(output.rms_current, 2) * R * sqrt(2);
      double loss_dot = pow(output.rms_current, 2) * sqrt(2) * R_dot;

      double volume = calcOutput(output.volume, inputs);
      // double volume_dot = jacobianVectorProduct(output.volume, wrt_dot, wrt);

      // double dc_loss = loss / volume;
      double dc_loss_dot =
          loss_dot / volume;  // - loss / pow(volume, 2) * volume_dot;

      return dc_loss_dot;
   }
   else
   {
      return 0.0;
   }
}

// void jacobianVectorProduct(DCLossFunctional &output,
//                            const mfem::Vector &wrt_dot,
//                            const std::string &wrt,
//                            mfem::Vector &out_dot)
// { }

double vectorJacobianProduct(DCLossFunctional &output,
                             const mfem::Vector &out_bar,
                             const std::string &wrt)
{
   const MISOInputs &inputs = *output.inputs;
   if (wrt.rfind("wire_length", 0) == 0)
   {
      double rho = calcOutput(output.resistivity, inputs);

      double strand_area = M_PI * pow(output.strand_radius, 2);
      // double R = output.wire_length * rho / (strand_area *
      // output.strands_in_hand);

      // double loss = pow(output.rms_current, 2) * R * sqrt(2);

      double volume = calcOutput(output.volume, inputs);
      // double dc_loss = loss / volume;

      /// Start reverse pass...
      double dc_loss_bar = out_bar(0);

      /// double dc_loss = loss / volume;
      double loss_bar = dc_loss_bar / volume;
      // double volume_bar = -dc_loss_bar * loss / pow(volume, 2);

      /// double volume = calcOutput(output.volume, inputs);
      // volume does not depend on any of the inputs except mesh coords

      /// double loss = pow(output.rms_current, 2) * R * sqrt(2);
      // double rms_current_bar = loss_bar * 2 * output.rms_current * R *
      // sqrt(2);
      double R_bar = loss_bar * pow(output.rms_current, 2) * sqrt(2);

      /// double R = output.wire_length * rho / (strand_area *
      /// output.strands_in_hand);
      double wire_length_bar =
          R_bar * rho / (strand_area * output.strands_in_hand);
      // double rho_bar = R_bar * output.wire_length / (strand_area *
      // output.strands_in_hand); double strand_area_bar = -R_bar *
      // output.wire_length * rho / (pow(strand_area,2) *
      // output.strands_in_hand); double strands_in_hand_bar = -R_bar *
      // output.wire_length * rho / (strand_area * pow(output.strands_in_hand,
      // 2));

      /// double strand_area = M_PI * pow(output.strand_radius, 2);
      // double strand_radius_bar = strand_area_bar * M_PI * 2 *
      // output.strand_radius;

      /// double rho = output.output.GetEnergy(output.scratch);
      // rho does not depend on any of the inputs except mesh coords

      return wire_length_bar;
   }
   else if (wrt.rfind("rms_current", 0) == 0)
   {
      double rho = calcOutput(output.resistivity, inputs);

      double strand_area = M_PI * pow(output.strand_radius, 2);
      double R =
          output.wire_length * rho / (strand_area * output.strands_in_hand);

      // double loss = pow(output.rms_current, 2) * R * sqrt(2);

      double volume = calcOutput(output.volume, inputs);
      // double dc_loss = loss / volume;

      /// Start reverse pass...
      double dc_loss_bar = out_bar(0);

      /// double dc_loss = loss / volume;
      double loss_bar = dc_loss_bar / volume;
      // double volume_bar = -dc_loss_bar * loss / pow(volume, 2);

      /// double volume = calcOutput(output.volume, inputs);
      // volume does not depend on any of the inputs except mesh coords

      /// double loss = pow(output.rms_current, 2) * R * sqrt(2);
      double rms_current_bar = loss_bar * 2 * output.rms_current * R * sqrt(2);
      // double R_bar = loss_bar * pow(output.rms_current, 2) * sqrt(2);

      /// double R = output.wire_length * rho / (strand_area *
      /// output.strands_in_hand);
      // double wire_length_bar = R_bar * rho / (strand_area *
      // output.strands_in_hand); double rho_bar = R_bar * output.wire_length /
      // (strand_area * output.strands_in_hand); double strand_area_bar = -R_bar
      // * output.wire_length * rho / (pow(strand_area,2) *
      // output.strands_in_hand); double strands_in_hand_bar = -R_bar *
      // output.wire_length * rho / (strand_area * pow(output.strands_in_hand,
      // 2));

      /// double strand_area = M_PI * pow(output.strand_radius, 2);
      // double strand_radius_bar = strand_area_bar * M_PI * 2 *
      // output.strand_radius;

      /// double rho = output.output.GetEnergy(output.scratch);
      // rho does not depend on any of the inputs except mesh coords

      return rms_current_bar;
   }
   else if (wrt.rfind("strand_radius", 0) == 0)
   {
      double rho = calcOutput(output.resistivity, inputs);

      double strand_area = M_PI * pow(output.strand_radius, 2);
      // double R = output.wire_length * rho / (strand_area *
      // output.strands_in_hand);

      // double loss = pow(output.rms_current, 2) * R * sqrt(2);

      double volume = calcOutput(output.volume, inputs);
      // double dc_loss = loss / volume;

      /// Start reverse pass...
      double dc_loss_bar = out_bar(0);

      /// double dc_loss = loss / volume;
      double loss_bar = dc_loss_bar / volume;
      // double volume_bar = -dc_loss_bar * loss / pow(volume, 2);

      /// double volume = calcOutput(output.volume, inputs);
      // volume does not depend on any of the inputs except mesh coords

      /// double loss = pow(output.rms_current, 2) * R * sqrt(2);
      // double rms_current_bar = loss_bar * 2 * output.rms_current * R *
      // sqrt(2);
      double R_bar = loss_bar * pow(output.rms_current, 2) * sqrt(2);

      /// double R = output.wire_length * rho / (strand_area *
      /// output.strands_in_hand);
      // double wire_length_bar = R_bar * rho / (strand_area *
      // output.strands_in_hand); double rho_bar = R_bar * output.wire_length /
      // (strand_area * output.strands_in_hand);
      double strand_area_bar = -R_bar * output.wire_length * rho /
                               (pow(strand_area, 2) * output.strands_in_hand);
      // double strands_in_hand_bar = -R_bar * output.wire_length * rho /
      // (strand_area * pow(output.strands_in_hand, 2));

      /// double strand_area = M_PI * pow(output.strand_radius, 2);
      double strand_radius_bar =
          strand_area_bar * M_PI * 2 * output.strand_radius;

      /// double rho = output.output.GetEnergy(output.scratch);
      // rho does not depend on any of the inputs except mesh coords

      return strand_radius_bar;
   }
   else if (wrt.rfind("strands_in_hand", 0) == 0)
   {
      double rho = calcOutput(output.resistivity, inputs);

      double strand_area = M_PI * pow(output.strand_radius, 2);
      // double R = output.wire_length * rho / (strand_area *
      // output.strands_in_hand);

      // double loss = pow(output.rms_current, 2) * R * sqrt(2);

      double volume = calcOutput(output.volume, inputs);
      // double dc_loss = loss / volume;

      /// Start reverse pass...
      double dc_loss_bar = out_bar(0);

      /// double dc_loss = loss / volume;
      double loss_bar = dc_loss_bar / volume;
      // double volume_bar = -dc_loss_bar * loss / pow(volume, 2);

      /// double volume = calcOutput(output.volume, inputs);
      // volume does not depend on any of the inputs except mesh coords

      /// double loss = pow(output.rms_current, 2) * R * sqrt(2);
      // double rms_current_bar = loss_bar * 2 * output.rms_current * R *
      // sqrt(2);
      double R_bar = loss_bar * pow(output.rms_current, 2) * sqrt(2);

      /// double R = output.wire_length * rho / (strand_area *
      /// output.strands_in_hand);
      // double wire_length_bar = R_bar * rho / (strand_area *
      // output.strands_in_hand); double rho_bar = R_bar * output.wire_length /
      // (strand_area * output.strands_in_hand); double strand_area_bar = -R_bar
      // * output.wire_length * rho / (pow(strand_area,2) *
      // output.strands_in_hand);
      double strands_in_hand_bar =
          -R_bar * output.wire_length * rho /
          (strand_area * pow(output.strands_in_hand, 2));

      /// double strand_area = M_PI * pow(output.strand_radius, 2);
      // double strand_radius_bar = strand_area_bar * M_PI * 2 *
      // output.strand_radius;

      /// double rho = output.output.GetEnergy(output.scratch);
      // rho does not depend on any of the inputs except mesh coords

      return strands_in_hand_bar;
   }
   else
   {
      return 0.0;
   }
}

void vectorJacobianProduct(DCLossFunctional &output,
                           const mfem::Vector &out_bar,
                           const std::string &wrt,
                           mfem::Vector &wrt_bar)
{
   const MISOInputs &inputs = *output.inputs;
   if (wrt.rfind("mesh_coords", 0) == 0)
   {
      double rho = calcOutput(output.resistivity, inputs);

      double strand_area = M_PI * pow(output.strand_radius, 2);
      double R =
          output.wire_length * rho / (strand_area * output.strands_in_hand);

      double loss = pow(output.rms_current, 2) * R * sqrt(2);

      double volume = calcOutput(output.volume, inputs);
      // double dc_loss = loss / volume;

      /// Start reverse pass...
      double dc_loss_bar = out_bar(0);

      /// double dc_loss = loss / volume;
      double loss_bar = dc_loss_bar / volume;
      double volume_bar = -dc_loss_bar * loss / pow(volume, 2);

      /// double volume = calcOutput(output.volume, inputs);
      mfem::Vector vol_bar_vec(&volume_bar, 1);
      vectorJacobianProduct(output.volume, vol_bar_vec, wrt, wrt_bar);

      /// double loss = pow(output.rms_current, 2) * R * sqrt(2);
      // double rms_current_bar =
      //     loss_bar * 2 * output.rms_current * R * sqrt(2);
      double R_bar = loss_bar * pow(output.rms_current, 2) * sqrt(2);

      /// double R =
      ///     output.wire_length * rho / (strand_area * output.strands_in_hand);
      // double wire_length_bar =
      //     R_bar * rho / (strand_area * output.strands_in_hand);
      double rho_bar =
          R_bar * output.wire_length / (strand_area * output.strands_in_hand);
      // double strand_area_bar = -R_bar * output.wire_length * rho /
      //                          (pow(strand_area, 2) *
      //                          output.strands_in_hand);
      // double strands_in_hand_bar =
      //     -R_bar * output.wire_length * rho /
      //     (strand_area * pow(output.strands_in_hand, 2));

      /// double strand_area = M_PI * pow(output.strand_radius, 2);
      // double strand_radius_bar =
      //     strand_area_bar * M_PI * 2 * output.strand_radius;

      /// double rho = calcOutput(output.resistivity, inputs);
      mfem::Vector rho_bar_vec(&rho_bar, 1);
      vectorJacobianProduct(output.resistivity, rho_bar_vec, wrt, wrt_bar);
   }
   else if (wrt.rfind("temperature", 0) == 0)
   {
      // double rho = calcOutput(output.resistivity, inputs);

      double strand_area = M_PI * pow(output.strand_radius, 2);
      // double R =
      //     output.wire_length * rho / (strand_area * output.strands_in_hand);

      // double loss = pow(output.rms_current, 2) * R * sqrt(2);

      double volume = calcOutput(output.volume, inputs);
      // double dc_loss = loss / volume;

      /// Start reverse pass...
      double dc_loss_bar = out_bar(0);

      /// double dc_loss = loss / volume;
      double loss_bar = dc_loss_bar / volume;
      // double volume_bar = -dc_loss_bar * loss / pow(volume, 2);

      /// double volume = calcOutput(output.volume, inputs);
      // mfem::Vector vol_bar_vec(&volume_bar, 1);
      // vectorJacobianProduct(output.volume, vol_bar_vec, wrt, wrt_bar);

      /// double loss = pow(output.rms_current, 2) * R * sqrt(2);
      // double rms_current_bar =
      //     loss_bar * 2 * output.rms_current * R * sqrt(2);
      double R_bar = loss_bar * pow(output.rms_current, 2) * sqrt(2);

      /// double R =
      ///     output.wire_length * rho / (strand_area * output.strands_in_hand);
      // double wire_length_bar =
      //     R_bar * rho / (strand_area * output.strands_in_hand);
      double rho_bar =
          R_bar * output.wire_length / (strand_area * output.strands_in_hand);
      // double strand_area_bar = -R_bar * output.wire_length * rho /
      //                          (pow(strand_area, 2) *
      //                          output.strands_in_hand);
      // double strands_in_hand_bar =
      //     -R_bar * output.wire_length * rho /
      //     (strand_area * pow(output.strands_in_hand, 2));

      /// double strand_area = M_PI * pow(output.strand_radius, 2);
      // double strand_radius_bar =
      //     strand_area_bar * M_PI * 2 * output.strand_radius;

      /// double rho = calcOutput(output.resistivity, inputs);
      mfem::Vector rho_bar_vec(&rho_bar, 1);
      vectorJacobianProduct(output.resistivity, rho_bar_vec, wrt, wrt_bar);
   }
}

DCLossFunctional::DCLossFunctional(
    std::map<std::string, FiniteElementState> &fields,
    StateCoefficient &sigma,
    const nlohmann::json &options)
 : resistivity(fields.at("state").space(), fields), volume(fields, options)
{
   auto &temp = fields.at("temperature");

   // Assign the integrator used to compute the DC losses
   if (options.contains("attributes"))
   {
      auto attributes = options["attributes"].get<std::vector<int>>();
      resistivity.addOutputDomainIntegrator(
          new DCLossFunctionalIntegrator(sigma, temp.gridFunc()), attributes);
   }
   else
   {
      resistivity.addOutputDomainIntegrator(
          new DCLossFunctionalIntegrator(sigma, temp.gridFunc()));
   }
}

void calcOutput(DCLossDistribution &output,
                const MISOInputs &inputs,
                mfem::Vector &out_vec)
{
   auto winding_volume = calcOutput(output.volume, inputs);
   setInputs(output.output, {{"winding_volume", winding_volume}});
   setInputs(output.output, inputs);
   out_vec = 0.0;
   addLoad(output.output, out_vec);
}

void jacobianVectorProduct(DCLossDistribution &output,
                           const mfem::Vector &wrt_dot,
                           const std::string &wrt,
                           mfem::Vector &out_dot)
{
   jacobianVectorProduct(output.output, wrt_dot, wrt, out_dot);
}

double vectorJacobianProduct(DCLossDistribution &output,
                             const mfem::Vector &out_bar,
                             const std::string &wrt)
{
   return vectorJacobianProduct(output.output, out_bar, wrt);
}

void vectorJacobianProduct(DCLossDistribution &output,
                           const mfem::Vector &out_bar,
                           const std::string &wrt,
                           mfem::Vector &wrt_bar)
{
   if (wrt.rfind("mesh_coords", 0) == 0)
   {
      // auto winding_volume = calcOutput(output.volume, inputs);
      // setInputs(output.output, {{"volume", winding_volume}});

      vectorJacobianProduct(output.output, out_bar, wrt, wrt_bar);

      double volume_bar =
          vectorJacobianProduct(output.output, out_bar, "winding_volume");
      mfem::Vector vol_bar_vec(&volume_bar, 1);
      vectorJacobianProduct(output.volume, vol_bar_vec, wrt, wrt_bar);
   }
   else
   {
      vectorJacobianProduct(output.output, out_bar, wrt, wrt_bar);
   }
}

DCLossDistribution::DCLossDistribution(
    std::map<std::string, FiniteElementState> &fields,
    StateCoefficient &sigma,
    const nlohmann::json &options)
 : output(fields.at("temperature").space(), fields, "thermal_adjoint"),
   volume(fields, options)
{
   auto &temp = fields.at("temperature");

   // std::vector<int> current_attributes;
   // for (const auto &group : options["current"])
   // {
   //    for (const auto &source : group)
   //    {
   //       auto attrs = source.get<std::vector<int>>();
   //       current_attributes.insert(
   //           current_attributes.end(), attrs.begin(), attrs.end());
   //    }
   // }

   if (options.contains("attributes"))
   {
      auto current_attributes = options["attributes"].get<std::vector<int>>();
      output.addDomainIntegrator(
          new DCLossDistributionIntegrator(sigma, temp.gridFunc()),
          current_attributes);
   }
   else
   {
      output.addDomainIntegrator(
          new DCLossDistributionIntegrator(sigma, temp.gridFunc()));
   }
}

void setOptions(ACLossFunctional &output, const nlohmann::json &options)
{
   // output.num_strands = options.value("num_strands", output.num_strands);
   setOptions(output.output, options);
}

void setInputs(ACLossFunctional &output, const MISOInputs &inputs)
{
   output.inputs = inputs;
   // output.inputs["state"] = inputs.at("peak_flux");

   setValueFromInputs(inputs, "strand_radius", output.radius);
   setValueFromInputs(inputs, "frequency", output.freq);
   setValueFromInputs(inputs, "stack_length", output.stack_length);
   setValueFromInputs(inputs, "strands_in_hand", output.strands_in_hand);
   setValueFromInputs(inputs, "num_turns", output.num_turns);
   setValueFromInputs(inputs, "num_slots", output.num_slots);

   setInputs(output.output, inputs);
}

double calcOutput(ACLossFunctional &output, const MISOInputs &inputs)
{
   setInputs(output, inputs);

   // mfem::Vector flux_state;
   // setVectorFromInputs(inputs, "peak_flux", flux_state, false, true);
   /// TODO: Remove once done debugging
   // std::cout << "flux_state.Size() = " << flux_state.Size() << "\n";
   // std::cout << "flux_state=np.array([";
   // for (int j = 0; j < flux_state.Size(); j++) {std::cout <<
   // flux_state.Elem(j) << ", ";} std::cout << "])\n";
   // auto &flux_mag = output.fields.at("peak_flux");
   // flux_mag.distributeSharedDofs(flux_state);
   // mfem::ParaViewDataCollection pv("FluxMag", &flux_mag.mesh());
   // pv.SetPrefixPath("ParaView");
   // pv.SetLevelsOfDetail(3);
   // pv.SetDataFormat(mfem::VTKFormat::BINARY);
   // pv.SetHighOrderOutput(true);
   // pv.RegisterField("FluxMag", &flux_mag.gridFunc());
   // pv.Save();

   double sigma_b2 = calcOutput(output.output, output.inputs);

   double strand_loss = sigma_b2 * output.stack_length * M_PI *
                        pow(output.radius, 4) * pow(2 * M_PI * output.freq, 2) /
                        8.0;

   double num_strands =
       2 * output.strands_in_hand * output.num_turns * output.num_slots;

   double loss = num_strands * strand_loss;

   double volume = calcOutput(output.volume, output.inputs);

   return loss / volume;
}

double jacobianVectorProduct(ACLossFunctional &output,
                             const mfem::Vector &wrt_dot,
                             const std::string &wrt)
{
   if (wrt.rfind("strand_radius", 0) == 0)
   {
      double sigma_b2 = calcOutput(output.output, output.inputs);

      // double strand_loss = sigma_b2 * output.stack_length * M_PI *
      //                      pow(output.radius, 4) *
      //                      pow(2 * M_PI * output.freq, 2) / 8.0;
      double strand_loss_dot =
          4 * sigma_b2 * output.stack_length * M_PI * pow(output.radius, 3) *
          pow(2 * M_PI * output.freq, 2) / 8.0 * wrt_dot(0);

      double num_strands =
          2 * output.strands_in_hand * output.num_turns * output.num_slots;

      // double loss = num_strands * strand_loss;
      double loss_dot = num_strands * strand_loss_dot;

      double volume = calcOutput(output.volume, output.inputs);

      return loss_dot / volume;
   }
   else if (wrt.rfind("frequency", 0) == 0)
   {
      double sigma_b2 = calcOutput(output.output, output.inputs);

      // double strand_loss = sigma_b2 * output.stack_length * M_PI *
      //                      pow(output.radius, 4) *
      //                      pow(2 * M_PI * output.freq, 2) / 8.0;
      double strand_loss_dot = 2 * sigma_b2 * output.stack_length * M_PI *
                               pow(output.radius, 3) * output.freq *
                               pow(2 * M_PI, 2) / 8.0 * wrt_dot(0);

      double num_strands =
          2 * output.strands_in_hand * output.num_turns * output.num_slots;

      // double loss = num_strands * strand_loss;
      double loss_dot = num_strands * strand_loss_dot;

      double volume = calcOutput(output.volume, output.inputs);

      return loss_dot / volume;
   }
   else if (wrt.rfind("stack_length", 0) == 0)
   {
      double sigma_b2 = calcOutput(output.output, output.inputs);

      // double strand_loss = sigma_b2 * output.stack_length * M_PI *
      //                      pow(output.radius, 4) *
      //                      pow(2 * M_PI * output.freq, 2) / 8.0;
      double strand_loss_dot = sigma_b2 * M_PI * pow(output.radius, 4) *
                               pow(2 * M_PI * output.freq, 2) / 8.0 *
                               wrt_dot(0);

      double num_strands =
          2 * output.strands_in_hand * output.num_turns * output.num_slots;

      // double loss = num_strands * strand_loss;
      double loss_dot = num_strands * strand_loss_dot;

      double volume = calcOutput(output.volume, output.inputs);

      return loss_dot / volume;
   }
   else if (wrt.rfind("strands_in_hand", 0) == 0)
   {
      double sigma_b2 = calcOutput(output.output, output.inputs);

      double strand_loss = sigma_b2 * output.stack_length * M_PI *
                           pow(output.radius, 4) *
                           pow(2 * M_PI * output.freq, 2) / 8.0;

      // double num_strands =
      //     2 * output.strands_in_hand * output.num_turns * output.num_slots;

      double num_strands_dot =
          2 * output.num_turns * output.num_slots * wrt_dot(0);

      // double loss = num_strands * strand_loss;
      double loss_dot = strand_loss * num_strands_dot;

      double volume = calcOutput(output.volume, output.inputs);

      return loss_dot / volume;
   }
   else if (wrt.rfind("num_turns", 0) == 0)
   {
      double sigma_b2 = calcOutput(output.output, output.inputs);

      double strand_loss = sigma_b2 * output.stack_length * M_PI *
                           pow(output.radius, 4) *
                           pow(2 * M_PI * output.freq, 2) / 8.0;

      // double num_strands =
      //     2 * output.strands_in_hand * output.num_turns * output.num_slots;

      double num_strands_dot =
          2 * output.strands_in_hand * output.num_slots * wrt_dot(0);

      // double loss = num_strands * strand_loss;
      double loss_dot = strand_loss * num_strands_dot;

      double volume = calcOutput(output.volume, output.inputs);

      return loss_dot / volume;
   }
   else if (wrt.rfind("num_slots", 0) == 0)
   {
      double sigma_b2 = calcOutput(output.output, output.inputs);

      double strand_loss = sigma_b2 * output.stack_length * M_PI *
                           pow(output.radius, 4) *
                           pow(2 * M_PI * output.freq, 2) / 8.0;

      // double num_strands =
      //     2 * output.strands_in_hand * output.num_turns * output.num_slots;

      double num_strands_dot =
          2 * output.strands_in_hand * output.num_turns * wrt_dot(0);

      // double loss = num_strands * strand_loss;
      double loss_dot = strand_loss * num_strands_dot;

      double volume = calcOutput(output.volume, output.inputs);

      return loss_dot / volume;
   }
   else if (wrt.rfind("mesh_coords", 0) == 0)
   {
      double sigma_b2 = calcOutput(output.output, output.inputs);
      double sigma_b2_dot = jacobianVectorProduct(output.output, wrt_dot, wrt);

      double strand_loss = sigma_b2 * output.stack_length * M_PI *
                           pow(output.radius, 4) *
                           pow(2 * M_PI * output.freq, 2) / 8.0;

      double strand_loss_dot =
          output.stack_length * M_PI * pow(output.radius, 4) *
          pow(2 * M_PI * output.freq, 2) / 8.0 * sigma_b2_dot;

      double num_strands =
          2 * output.strands_in_hand * output.num_turns * output.num_slots;

      double loss = num_strands * strand_loss;
      double loss_dot = num_strands * strand_loss_dot;

      double volume = calcOutput(output.volume, output.inputs);
      double volume_dot = jacobianVectorProduct(output.volume, wrt_dot, wrt);

      return loss_dot / volume - loss / pow(volume, 2) * volume_dot;
   }
   else if (wrt.rfind("peak_flux", 0) == 0)
   {
      // double sigma_b2 = calcOutput(output.output, output.inputs);
      double sigma_b2_dot = jacobianVectorProduct(output.output, wrt_dot, wrt);

      // double strand_loss = sigma_b2 * output.stack_length * M_PI *
      //                      pow(output.radius, 4) *
      //                      pow(2 * M_PI * output.freq, 2) / 8.0;

      double strand_loss_dot =
          output.stack_length * M_PI * pow(output.radius, 4) *
          pow(2 * M_PI * output.freq, 2) / 8.0 * sigma_b2_dot;

      double num_strands =
          2 * output.strands_in_hand * output.num_turns * output.num_slots;

      // double loss = num_strands * strand_loss;
      double loss_dot = num_strands * strand_loss_dot;

      double volume = calcOutput(output.volume, output.inputs);
      // double volume_dot = jacobianVectorProduct(output.volume, wrt_dot, wrt);

      return loss_dot / volume;  // - loss / pow(volume, 2) * volume_dot;
   }
   // Adding in w/r/t temperature for the future. Untested.
   else if (wrt.rfind("temperature", 0) == 0)
   {
      // double sigma_b2 = calcOutput(output.output, output.inputs);
      double sigma_b2_dot = jacobianVectorProduct(output.output, wrt_dot, wrt);

      // double strand_loss = sigma_b2 * output.stack_length * M_PI *
      //                      pow(output.radius, 4) *
      //                      pow(2 * M_PI * output.freq, 2) / 8.0;

      double strand_loss_dot =
          output.stack_length * M_PI * pow(output.radius, 4) *
          pow(2 * M_PI * output.freq, 2) / 8.0 * sigma_b2_dot;

      double num_strands =
          2 * output.strands_in_hand * output.num_turns * output.num_slots;

      // double loss = num_strands * strand_loss;
      double loss_dot = num_strands * strand_loss_dot;

      double volume = calcOutput(output.volume, output.inputs);
      // double volume_dot = jacobianVectorProduct(output.volume, wrt_dot, wrt);
      // // volume is independent of temperature

      return loss_dot / volume;
   }
   else
   {
      return 0.0;
   }
}

double vectorJacobianProduct(ACLossFunctional &output,
                             const mfem::Vector &out_bar,
                             const std::string &wrt)
{
   if (wrt.rfind("strand_radius", 0) == 0)
   {
      double sigma_b2 = calcOutput(output.output, output.inputs);

      // double strand_loss = sigma_b2 * output.stack_length * M_PI *
      //                      pow(output.radius, 4) *
      //                      pow(2 * M_PI * output.freq, 2) / 8.0;
      double num_strands =
          2 * output.strands_in_hand * output.num_turns * output.num_slots;

      // double loss = num_strands * strand_loss;

      double volume = calcOutput(output.volume, output.inputs);

      // double ac_loss = loss / volume;

      /// Start reverse pass...
      double ac_loss_bar = out_bar(0);

      /// double ac_loss = loss / volume;
      double loss_bar = ac_loss_bar / volume;
      // double volume_bar = -ac_loss_bar * loss / pow(volume, 2);

      /// double volume = calcOutput(output.volume, inputs);
      // volume does not depend on any of the inputs except mesh coords

      /// double loss = num_strands * strand_loss;
      // double num_strands_bar = loss_bar * strand_loss;
      double strand_loss_bar = loss_bar * num_strands;

      /// double num_strands =
      ///     2 * output.strands_in_hand * output.num_turns * output.num_slots;
      // double strands_in_hand_bar =
      //     num_strands_bar * 2 * output.num_turns * output.num_slots;
      // double num_turns_bar =
      //     num_strands_bar * 2 * output.strands_in_hand * output.num_slots;
      // double num_slots_bar =
      //     num_strands_bar * 2 * output.strands_in_hand * output.num_turns;

      /// double strand_loss = sigma_b2 * output.stack_length * M_PI *
      ///                      pow(output.radius, 4) *
      ///                      pow(2 * M_PI * output.freq, 2) / 8.0;
      // double sigma_b2_bar = strand_loss_bar * output.stack_length * M_PI *
      //                       pow(output.radius, 4) *
      //                       pow(2 * M_PI * output.freq, 2) / 8.0;
      // double stack_length_bar = strand_loss_bar * sigma_b2 * M_PI *
      //                           pow(output.radius, 4) *
      //                           pow(2 * M_PI * output.freq, 2) / 8.0;
      double strand_radius_bar =
          strand_loss_bar * sigma_b2 * output.stack_length * M_PI * 4 *
          pow(output.radius, 3) * pow(2 * M_PI * output.freq, 2) / 8.0;
      // double frequency_bar = strand_loss_bar * sigma_b2 * output.stack_length
      // *
      //                        M_PI * pow(output.radius, 4) * 2 * output.freq *
      //                        pow(2 * M_PI, 2) / 8.0;

      return strand_radius_bar;
   }
   else if (wrt.rfind("frequency", 0) == 0)
   {
      double sigma_b2 = calcOutput(output.output, output.inputs);

      // double strand_loss = sigma_b2 * output.stack_length * M_PI *
      //                      pow(output.radius, 4) *
      //                      pow(2 * M_PI * output.freq, 2) / 8.0;
      double num_strands =
          2 * output.strands_in_hand * output.num_turns * output.num_slots;

      // double loss = num_strands * strand_loss;

      double volume = calcOutput(output.volume, output.inputs);

      // double ac_loss = loss / volume;

      /// Start reverse pass...
      double ac_loss_bar = out_bar(0);

      /// double ac_loss = loss / volume;
      double loss_bar = ac_loss_bar / volume;
      // double volume_bar = -ac_loss_bar * loss / pow(volume, 2);

      /// double volume = calcOutput(output.volume, inputs);
      // volume does not depend on any of the inputs except mesh coords

      /// double loss = num_strands * strand_loss;
      // double num_strands_bar = loss_bar * strand_loss;
      double strand_loss_bar = loss_bar * num_strands;

      /// double num_strands =
      ///     2 * output.strands_in_hand * output.num_turns * output.num_slots;
      // double strands_in_hand_bar =
      //     num_strands_bar * 2 * output.num_turns * output.num_slots;
      // double num_turns_bar =
      //     num_strands_bar * 2 * output.strands_in_hand * output.num_slots;
      // double num_slots_bar =
      //     num_strands_bar * 2 * output.strands_in_hand * output.num_turns;

      /// double strand_loss = sigma_b2 * output.stack_length * M_PI *
      ///                      pow(output.radius, 4) *
      ///                      pow(2 * M_PI * output.freq, 2) / 8.0;
      // double sigma_b2_bar = strand_loss_bar * output.stack_length * M_PI *
      //                       pow(output.radius, 4) *
      //                       pow(2 * M_PI * output.freq, 2) / 8.0;
      // double stack_length_bar = strand_loss_bar * sigma_b2 * M_PI *
      //                           pow(output.radius, 4) *
      //                           pow(2 * M_PI * output.freq, 2) / 8.0;
      // double strand_radius_bar =
      //     strand_loss_bar * sigma_b2 * output.stack_length * M_PI * 4 *
      //     pow(output.radius, 3) * pow(2 * M_PI * output.freq, 2) / 8.0;
      double frequency_bar = strand_loss_bar * sigma_b2 * output.stack_length *
                             M_PI * pow(output.radius, 4) * 2 * output.freq *
                             pow(2 * M_PI, 2) / 8.0;

      return frequency_bar;
   }
   else if (wrt.rfind("stack_length", 0) == 0)
   {
      double sigma_b2 = calcOutput(output.output, output.inputs);

      // double strand_loss = sigma_b2 * output.stack_length * M_PI *
      //                      pow(output.radius, 4) *
      //                      pow(2 * M_PI * output.freq, 2) / 8.0;
      double num_strands =
          2 * output.strands_in_hand * output.num_turns * output.num_slots;

      // double loss = num_strands * strand_loss;

      double volume = calcOutput(output.volume, output.inputs);

      // double ac_loss = loss / volume;

      /// Start reverse pass...
      double ac_loss_bar = out_bar(0);

      /// double ac_loss = loss / volume;
      double loss_bar = ac_loss_bar / volume;
      // double volume_bar = -ac_loss_bar * loss / pow(volume, 2);

      /// double volume = calcOutput(output.volume, inputs);
      // volume does not depend on any of the inputs except mesh coords

      /// double loss = num_strands * strand_loss;
      // double num_strands_bar = loss_bar * strand_loss;
      double strand_loss_bar = loss_bar * num_strands;

      /// double num_strands =
      ///     2 * output.strands_in_hand * output.num_turns * output.num_slots;
      // double strands_in_hand_bar =
      //     num_strands_bar * 2 * output.num_turns * output.num_slots;
      // double num_turns_bar =
      //     num_strands_bar * 2 * output.strands_in_hand * output.num_slots;
      // double num_slots_bar =
      //     num_strands_bar * 2 * output.strands_in_hand * output.num_turns;

      /// double strand_loss = sigma_b2 * output.stack_length * M_PI *
      ///                      pow(output.radius, 4) *
      ///                      pow(2 * M_PI * output.freq, 2) / 8.0;
      // double sigma_b2_bar = strand_loss_bar * output.stack_length * M_PI *
      //                       pow(output.radius, 4) *
      //                       pow(2 * M_PI * output.freq, 2) / 8.0;
      double stack_length_bar = strand_loss_bar * sigma_b2 * M_PI *
                                pow(output.radius, 4) *
                                pow(2 * M_PI * output.freq, 2) / 8.0;
      // double strand_radius_bar =
      //     strand_loss_bar * sigma_b2 * output.stack_length * M_PI * 4 *
      //     pow(output.radius, 3) * pow(2 * M_PI * output.freq, 2) / 8.0;
      // double frequency_bar = strand_loss_bar * sigma_b2 * output.stack_length
      // *
      //                        M_PI * pow(output.radius, 4) * 2 * output.freq *
      //                        pow(2 * M_PI, 2) / 8.0;

      return stack_length_bar;
   }
   else if (wrt.rfind("strands_in_hand", 0) == 0)
   {
      double sigma_b2 = calcOutput(output.output, output.inputs);

      double strand_loss = sigma_b2 * output.stack_length * M_PI *
                           pow(output.radius, 4) *
                           pow(2 * M_PI * output.freq, 2) / 8.0;
      // double num_strands =
      //     2 * output.strands_in_hand * output.num_turns * output.num_slots;

      // double loss = num_strands * strand_loss;

      double volume = calcOutput(output.volume, output.inputs);

      // double ac_loss = loss / volume;

      /// Start reverse pass...
      double ac_loss_bar = out_bar(0);

      /// double ac_loss = loss / volume;
      double loss_bar = ac_loss_bar / volume;
      // double volume_bar = -ac_loss_bar * loss / pow(volume, 2);

      /// double volume = calcOutput(output.volume, inputs);
      // volume does not depend on any of the inputs except mesh coords

      /// double loss = num_strands * strand_loss;
      double num_strands_bar = loss_bar * strand_loss;
      // double strand_loss_bar = loss_bar * num_strands;

      /// double num_strands =
      ///     2 * output.strands_in_hand * output.num_turns * output.num_slots;
      double strands_in_hand_bar =
          num_strands_bar * 2 * output.num_turns * output.num_slots;
      // double num_turns_bar =
      //     num_strands_bar * 2 * output.strands_in_hand * output.num_slots;
      // double num_slots_bar =
      //     num_strands_bar * 2 * output.strands_in_hand * output.num_turns;

      /// double strand_loss = sigma_b2 * output.stack_length * M_PI *
      ///                      pow(output.radius, 4) *
      ///                      pow(2 * M_PI * output.freq, 2) / 8.0;
      // double sigma_b2_bar = strand_loss_bar * output.stack_length * M_PI *
      //                       pow(output.radius, 4) *
      //                       pow(2 * M_PI * output.freq, 2) / 8.0;
      // double stack_length_bar = strand_loss_bar * sigma_b2 * M_PI *
      //                           pow(output.radius, 4) *
      //                           pow(2 * M_PI * output.freq, 2) / 8.0;
      // double strand_radius_bar =
      //     strand_loss_bar * sigma_b2 * output.stack_length * M_PI * 4 *
      //     pow(output.radius, 3) * pow(2 * M_PI * output.freq, 2) / 8.0;
      // double frequency_bar = strand_loss_bar * sigma_b2 * output.stack_length
      // *
      //                        M_PI * pow(output.radius, 4) * 2 * output.freq *
      //                        pow(2 * M_PI, 2) / 8.0;
      return strands_in_hand_bar;
   }
   else if (wrt.rfind("num_turns", 0) == 0)
   {
      double sigma_b2 = calcOutput(output.output, output.inputs);

      double strand_loss = sigma_b2 * output.stack_length * M_PI *
                           pow(output.radius, 4) *
                           pow(2 * M_PI * output.freq, 2) / 8.0;
      // double num_strands =
      //     2 * output.strands_in_hand * output.num_turns * output.num_slots;

      // double loss = num_strands * strand_loss;

      double volume = calcOutput(output.volume, output.inputs);

      // double ac_loss = loss / volume;

      /// Start reverse pass...
      double ac_loss_bar = out_bar(0);

      /// double ac_loss = loss / volume;
      double loss_bar = ac_loss_bar / volume;
      // double volume_bar = -ac_loss_bar * loss / pow(volume, 2);

      /// double volume = calcOutput(output.volume, inputs);
      // volume does not depend on any of the inputs except mesh coords

      /// double loss = num_strands * strand_loss;
      double num_strands_bar = loss_bar * strand_loss;
      // double strand_loss_bar = loss_bar * num_strands;

      /// double num_strands =
      ///     2 * output.strands_in_hand * output.num_turns * output.num_slots;
      // double strands_in_hand_bar =
      //     num_strands_bar * 2 * output.num_turns * output.num_slots;
      double num_turns_bar =
          num_strands_bar * 2 * output.strands_in_hand * output.num_slots;
      // double num_slots_bar =
      //     num_strands_bar * 2 * output.strands_in_hand * output.num_turns;

      /// double strand_loss = sigma_b2 * output.stack_length * M_PI *
      ///                      pow(output.radius, 4) *
      ///                      pow(2 * M_PI * output.freq, 2) / 8.0;
      // double sigma_b2_bar = strand_loss_bar * output.stack_length * M_PI *
      //                       pow(output.radius, 4) *
      //                       pow(2 * M_PI * output.freq, 2) / 8.0;
      // double stack_length_bar = strand_loss_bar * sigma_b2 * M_PI *
      //                           pow(output.radius, 4) *
      //                           pow(2 * M_PI * output.freq, 2) / 8.0;
      // double strand_radius_bar =
      //     strand_loss_bar * sigma_b2 * output.stack_length * M_PI * 4 *
      //     pow(output.radius, 3) * pow(2 * M_PI * output.freq, 2) / 8.0;
      // double frequency_bar = strand_loss_bar * sigma_b2 * output.stack_length
      // *
      //                        M_PI * pow(output.radius, 4) * 2 * output.freq *
      //                        pow(2 * M_PI, 2) / 8.0;
      return num_turns_bar;
   }
   else if (wrt.rfind("num_slots", 0) == 0)
   {
      double sigma_b2 = calcOutput(output.output, output.inputs);

      double strand_loss = sigma_b2 * output.stack_length * M_PI *
                           pow(output.radius, 4) *
                           pow(2 * M_PI * output.freq, 2) / 8.0;
      // double num_strands =
      //     2 * output.strands_in_hand * output.num_turns * output.num_slots;

      // double loss = num_strands * strand_loss;

      double volume = calcOutput(output.volume, output.inputs);

      // double ac_loss = loss / volume;

      /// Start reverse pass...
      double ac_loss_bar = out_bar(0);

      /// double ac_loss = loss / volume;
      double loss_bar = ac_loss_bar / volume;
      // double volume_bar = -ac_loss_bar * loss / pow(volume, 2);

      /// double volume = calcOutput(output.volume, inputs);
      // volume does not depend on any of the inputs except mesh coords

      /// double loss = num_strands * strand_loss;
      double num_strands_bar = loss_bar * strand_loss;
      // double strand_loss_bar = loss_bar * num_strands;

      /// double num_strands =
      ///     2 * output.strands_in_hand * output.num_turns * output.num_slots;
      // double strands_in_hand_bar =
      //     num_strands_bar * 2 * output.num_turns * output.num_slots;
      // double num_turns_bar =
      //     num_strands_bar * 2 * output.strands_in_hand * output.num_slots;
      double num_slots_bar =
          num_strands_bar * 2 * output.strands_in_hand * output.num_turns;

      /// double strand_loss = sigma_b2 * output.stack_length * M_PI *
      ///                      pow(output.radius, 4) *
      ///                      pow(2 * M_PI * output.freq, 2) / 8.0;
      // double sigma_b2_bar = strand_loss_bar * output.stack_length * M_PI *
      //                       pow(output.radius, 4) *
      //                       pow(2 * M_PI * output.freq, 2) / 8.0;
      // double stack_length_bar = strand_loss_bar * sigma_b2 * M_PI *
      //                           pow(output.radius, 4) *
      //                           pow(2 * M_PI * output.freq, 2) / 8.0;
      // double strand_radius_bar =
      //     strand_loss_bar * sigma_b2 * output.stack_length * M_PI * 4 *
      //     pow(output.radius, 3) * pow(2 * M_PI * output.freq, 2) / 8.0;
      // double frequency_bar = strand_loss_bar * sigma_b2 * output.stack_length
      // *
      //                        M_PI * pow(output.radius, 4) * 2 * output.freq *
      //                        pow(2 * M_PI, 2) / 8.0;
      return num_slots_bar;
   }
   else
   {
      return 0.0;
   }
}

void vectorJacobianProduct(ACLossFunctional &output,
                           const mfem::Vector &out_bar,
                           const std::string &wrt,
                           mfem::Vector &wrt_bar)
{
   if (wrt.rfind("mesh_coords", 0) == 0)
   {
      double sigma_b2 = calcOutput(output.output, output.inputs);

      double strand_loss = sigma_b2 * output.stack_length * M_PI *
                           pow(output.radius, 4) *
                           pow(2 * M_PI * output.freq, 2) / 8.0;
      double num_strands =
          2 * output.strands_in_hand * output.num_turns * output.num_slots;

      double loss = num_strands * strand_loss;

      double volume = calcOutput(output.volume, output.inputs);

      // double ac_loss = loss / volume;

      /// Start reverse pass...
      double ac_loss_bar = out_bar(0);

      /// double ac_loss = loss / volume;
      double loss_bar = ac_loss_bar / volume;
      double volume_bar = -ac_loss_bar * loss / pow(volume, 2);

      /// double volume = calcOutput(output.volume, inputs);
      mfem::Vector vol_bar_vec(&volume_bar, 1);
      vectorJacobianProduct(output.volume, vol_bar_vec, wrt, wrt_bar);

      /// double loss = num_strands * strand_loss;
      // double num_strands_bar = loss_bar * strand_loss;
      double strand_loss_bar = loss_bar * num_strands;

      /// double num_strands =
      ///     2 * output.strands_in_hand * output.num_turns * output.num_slots;
      // double strands_in_hand_bar =
      //     num_strands_bar * 2 * output.num_turns * output.num_slots;
      // double num_turns_bar =
      //     num_strands_bar * 2 * output.strands_in_hand * output.num_slots;
      // double num_slots_bar =
      //     num_strands_bar * 2 * output.strands_in_hand * output.num_turns;

      /// double strand_loss = sigma_b2 * output.stack_length * M_PI *
      ///                      pow(output.radius, 4) *
      ///                      pow(2 * M_PI * output.freq, 2) / 8.0;
      double sigma_b2_bar = strand_loss_bar * output.stack_length * M_PI *
                            pow(output.radius, 4) *
                            pow(2 * M_PI * output.freq, 2) / 8.0;
      // double stack_length_bar = strand_loss_bar * sigma_b2 * M_PI *
      //                           pow(output.radius, 4) *
      //                           pow(2 * M_PI * output.freq, 2) / 8.0;
      // double strand_radius_bar =
      //     strand_loss_bar * sigma_b2 * output.stack_length * M_PI * 4 *
      //     pow(output.radius, 3) * pow(2 * M_PI * output.freq, 2) / 8.0;
      // double frequency_bar = strand_loss_bar * sigma_b2 * output.stack_length
      // *
      //                        M_PI * pow(output.radius, 4) * 2 * output.freq *
      //                        pow(2 * M_PI, 2) / 8.0;

      /// double sigma_b2 = calcOutput(output.output, output.inputs);
      mfem::Vector sigma_b2_bar_vec(&sigma_b2_bar, 1);
      vectorJacobianProduct(output.output, sigma_b2_bar_vec, wrt, wrt_bar);
   }
   else if (wrt.rfind("peak_flux", 0) == 0)
   {
      // double sigma_b2 = calcOutput(output.output, output.inputs);

      // double strand_loss = sigma_b2 * output.stack_length * M_PI *
      //                      pow(output.radius, 4) *
      //                      pow(2 * M_PI * output.freq, 2) / 8.0;
      double num_strands =
          2 * output.strands_in_hand * output.num_turns * output.num_slots;

      // double loss = num_strands * strand_loss;

      double volume = calcOutput(output.volume, output.inputs);

      // double ac_loss = loss / volume;

      /// Start reverse pass...
      double ac_loss_bar = out_bar(0);

      /// double ac_loss = loss / volume;
      double loss_bar = ac_loss_bar / volume;
      // double volume_bar = -ac_loss_bar * loss / pow(volume, 2);

      /// double volume = calcOutput(output.volume, inputs);
      // mfem::Vector vol_bar_vec(&volume_bar, 1);
      // vectorJacobianProduct(output.volume, vol_bar_vec, "state", wrt_bar);

      /// double loss = num_strands * strand_loss;
      // double num_strands_bar = loss_bar * strand_loss;
      double strand_loss_bar = loss_bar * num_strands;

      /// double num_strands =
      ///     2 * output.strands_in_hand * output.num_turns * output.num_slots;
      // double strands_in_hand_bar =
      //     num_strands_bar * 2 * output.num_turns * output.num_slots;
      // double num_turns_bar =
      //     num_strands_bar * 2 * output.strands_in_hand * output.num_slots;
      // double num_slots_bar =
      //     num_strands_bar * 2 * output.strands_in_hand * output.num_turns;

      /// double strand_loss = sigma_b2 * output.stack_length * M_PI *
      ///                      pow(output.radius, 4) *
      ///                      pow(2 * M_PI * output.freq, 2) / 8.0;
      double sigma_b2_bar = strand_loss_bar * output.stack_length * M_PI *
                            pow(output.radius, 4) *
                            pow(2 * M_PI * output.freq, 2) / 8.0;
      // double stack_length_bar = strand_loss_bar * sigma_b2 * M_PI *
      //                           pow(output.radius, 4) *
      //                           pow(2 * M_PI * output.freq, 2) / 8.0;
      // double strand_radius_bar =
      //     strand_loss_bar * sigma_b2 * output.stack_length * M_PI * 4 *
      //     pow(output.radius, 3) * pow(2 * M_PI * output.freq, 2) / 8.0;
      // double frequency_bar = strand_loss_bar * sigma_b2 * output.stack_length
      // *
      //                        M_PI * pow(output.radius, 4) * 2 * output.freq *
      //                        pow(2 * M_PI, 2) / 8.0;

      /// double sigma_b2 = calcOutput(output.output, output.inputs);
      mfem::Vector sigma_b2_bar_vec(&sigma_b2_bar, 1);
      vectorJacobianProduct(output.output, sigma_b2_bar_vec, wrt, wrt_bar);
   }
   else if (wrt.rfind("temperature", 0) == 0)
   {
      // double sigma_b2 = calcOutput(output.output, output.inputs);

      // double strand_loss = sigma_b2 * output.stack_length * M_PI *
      //                      pow(output.radius, 4) *
      //                      pow(2 * M_PI * output.freq, 2) / 8.0;
      double num_strands =
          2 * output.strands_in_hand * output.num_turns * output.num_slots;

      // double loss = num_strands * strand_loss;

      double volume = calcOutput(output.volume, output.inputs);

      // double ac_loss = loss / volume;

      /// Start reverse pass...
      double ac_loss_bar = out_bar(0);

      /// double ac_loss = loss / volume;
      double loss_bar = ac_loss_bar / volume;
      // double volume_bar = -ac_loss_bar * loss / pow(volume, 2);

      /// double volume = calcOutput(output.volume, inputs);
      // mfem::Vector vol_bar_vec(&volume_bar, 1);
      // vectorJacobianProduct(output.volume, vol_bar_vec, "state", wrt_bar);

      /// double loss = num_strands * strand_loss;
      // double num_strands_bar = loss_bar * strand_loss;
      double strand_loss_bar = loss_bar * num_strands;

      /// double num_strands =
      ///     2 * output.strands_in_hand * output.num_turns * output.num_slots;
      // double strands_in_hand_bar =
      //     num_strands_bar * 2 * output.num_turns * output.num_slots;
      // double num_turns_bar =
      //     num_strands_bar * 2 * output.strands_in_hand * output.num_slots;
      // double num_slots_bar =
      //     num_strands_bar * 2 * output.strands_in_hand * output.num_turns;

      /// double strand_loss = sigma_b2 * output.stack_length * M_PI *
      ///                      pow(output.radius, 4) *
      ///                      pow(2 * M_PI * output.freq, 2) / 8.0;
      double sigma_b2_bar = strand_loss_bar * output.stack_length * M_PI *
                            pow(output.radius, 4) *
                            pow(2 * M_PI * output.freq, 2) / 8.0;
      // double stack_length_bar = strand_loss_bar * sigma_b2 * M_PI *
      //                           pow(output.radius, 4) *
      //                           pow(2 * M_PI * output.freq, 2) / 8.0;
      // double strand_radius_bar =
      //     strand_loss_bar * sigma_b2 * output.stack_length * M_PI * 4 *
      //     pow(output.radius, 3) * pow(2 * M_PI * output.freq, 2) / 8.0;
      // double frequency_bar = strand_loss_bar * sigma_b2 * output.stack_length
      // *
      //                        M_PI * pow(output.radius, 4) * 2 * output.freq *
      //                        pow(2 * M_PI, 2) / 8.0;

      ///  double sigma_b2 = calcOutput(output.output, output.inputs);
      mfem::Vector sigma_b2_bar_vec(&sigma_b2_bar, 1);
      vectorJacobianProduct(output.output, sigma_b2_bar_vec, wrt, wrt_bar);
   }
}

ACLossFunctional::ACLossFunctional(
    std::map<std::string, FiniteElementState> &fields,
    StateCoefficient &sigma,
    const nlohmann::json &options)
 : output(fields.at("peak_flux").space(), fields, "peak_flux"),
   volume(fields, options)
{
   auto &temp = fields.at("temperature");

   // Assign the integrator used to compute the AC losses
   if (options.contains("attributes"))
   {
      auto attributes = options["attributes"].get<std::vector<int>>();
      output.addOutputDomainIntegrator(
          new ACLossFunctionalIntegrator(sigma, temp.gridFunc()), attributes);
   }
   else
   {
      output.addOutputDomainIntegrator(
          new ACLossFunctionalIntegrator(sigma, temp.gridFunc()));
   }
   setOptions(*this, options);
}

void calcOutput(ACLossDistribution &output,
                const MISOInputs &inputs,
                mfem::Vector &out_vec)
{
   auto winding_volume = calcOutput(output.volume, inputs);
   setInputs(output.output, {{"winding_volume", winding_volume}});
   setInputs(output.output, inputs);
   out_vec = 0.0;
   addLoad(output.output, out_vec);
}

void jacobianVectorProduct(ACLossDistribution &output,
                           const mfem::Vector &wrt_dot,
                           const std::string &wrt,
                           mfem::Vector &out_dot)
{
   jacobianVectorProduct(output.output, wrt_dot, wrt, out_dot);
}

double vectorJacobianProduct(ACLossDistribution &output,
                             const mfem::Vector &out_bar,
                             const std::string &wrt)
{
   return vectorJacobianProduct(output.output, out_bar, wrt);
}

void vectorJacobianProduct(ACLossDistribution &output,
                           const mfem::Vector &out_bar,
                           const std::string &wrt,
                           mfem::Vector &wrt_bar)
{
   if (wrt.rfind("mesh_coords", 0) == 0)
   {
      // auto winding_volume = calcOutput(output.volume, inputs);
      // setInputs(output.output, {{"volume", winding_volume}});

      vectorJacobianProduct(output.output, out_bar, wrt, wrt_bar);

      double volume_bar =
          vectorJacobianProduct(output.output, out_bar, "winding_volume");
      mfem::Vector vol_bar_vec(&volume_bar, 1);
      vectorJacobianProduct(output.volume, vol_bar_vec, wrt, wrt_bar);
   }
   else
   {
      vectorJacobianProduct(output.output, out_bar, wrt, wrt_bar);
   }
}

ACLossDistribution::ACLossDistribution(
    std::map<std::string, FiniteElementState> &fields,
    StateCoefficient &sigma,
    const nlohmann::json &options)
 : output(fields.at("temperature").space(), fields, "thermal_adjoint"),
   volume(fields, options)
{
   auto &peak_flux = fields.at("peak_flux");
   auto &temp = fields.at("temperature");

   // std::vector<int> current_attributes;
   // for (const auto &group : options["current"])
   // {
   //    for (const auto &source : group)
   //    {
   //       auto attrs = source.get<std::vector<int>>();
   //       current_attributes.insert(
   //           current_attributes.end(), attrs.begin(), attrs.end());
   //    }
   // }

   if (options.contains("attributes"))
   {
      auto current_attributes = options["attributes"].get<std::vector<int>>();
      output.addDomainIntegrator(
          new ACLossDistributionIntegrator(
              peak_flux.gridFunc(), temp.gridFunc(), sigma),
          current_attributes);
   }
   else
   {
      output.addDomainIntegrator(new ACLossDistributionIntegrator(
          peak_flux.gridFunc(), temp.gridFunc(), sigma));
   }
}

void setOptions(CoreLossFunctional &output, const nlohmann::json &options)
{
   setOptions(output.output, options);
}

void setInputs(CoreLossFunctional &output, const MISOInputs &inputs)
{
   setInputs(output.output, inputs);
}

double calcOutput(CoreLossFunctional &output, const MISOInputs &inputs)
{
   return calcOutput(output.output, inputs);
}

double jacobianVectorProduct(CoreLossFunctional &output,
                             const mfem::Vector &wrt_dot,
                             const std::string &wrt)
{
   return jacobianVectorProduct(output.output, wrt_dot, wrt);
}

double vectorJacobianProduct(CoreLossFunctional &output,
                             const mfem::Vector &out_bar,
                             const std::string &wrt)
{
   return vectorJacobianProduct(output.output, out_bar, wrt);
}

void vectorJacobianProduct(CoreLossFunctional &output,
                           const mfem::Vector &out_bar,
                           const std::string &wrt,
                           mfem::Vector &wrt_bar)
{
   vectorJacobianProduct(output.output, out_bar, wrt, wrt_bar);
}

CoreLossFunctional::CoreLossFunctional(
    std::map<std::string, FiniteElementState> &fields,
    const nlohmann::json &components,
    const nlohmann::json &materials,
    const nlohmann::json &options)
 : output(fields.at("state").space(), fields),
   rho(constructMaterialCoefficient("rho", components, materials)),
   // k_s(constructMaterialCoefficient("ks", components, materials)),
   // alpha(constructMaterialCoefficient("alpha", components, materials)),
   // beta(constructMaterialCoefficient("beta", components, materials)),
   CAL2_kh(std::make_unique<CAL2khCoefficient>(components, materials)),
   CAL2_ke(std::make_unique<CAL2keCoefficient>(components, materials))
{
   // Making the integrator see the peak flux field
   const auto &peak_flux_iter =
       fields.find("peak_flux");  // find where peak flux field is
   mfem::GridFunction *peak_flux =
       nullptr;  // default peak flux field to null pointer
   if (peak_flux_iter != fields.end())
   {
      // If peak flux field exists, turn it into a grid function
      /// TODO: Ultimately handle the case where there is no peak flux field
      auto &flux_field = peak_flux_iter->second;
      peak_flux = &flux_field.gridFunc();
   }

   // Making the integrator see the temperature field
   const auto &temp_field_iter =
       fields.find("temperature");  // find where temperature field is
   mfem::GridFunction *temperature_field =
       nullptr;  // default temperature field to null pointer
   if (temp_field_iter != fields.end())
   {
      // If temperature field exists, turn it into a grid function
      auto &temp_field = temp_field_iter->second;
      temperature_field = &temp_field.gridFunc();
   }

   if (options.contains("attributes"))
   {
      // if (options.contains("UseCAL2forCoreLoss") &&
      //     options["UseCAL2forCoreLoss"].get<bool>())
      // {
      auto attributes = options["attributes"].get<std::vector<int>>();
      output.addOutputDomainIntegrator(
          new CAL2CoreLossIntegrator(
              *rho, *CAL2_kh, *CAL2_ke, *peak_flux, *temperature_field),
          attributes);
      //    std::cout << "CoreLossFunctional using CAL2\n";
      // }
      // else
      // {
      //    auto attributes = options["attributes"].get<std::vector<int>>();
      //    output.addOutputDomainIntegrator(
      //        new SteinmetzLossIntegrator(*rho, *k_s, *alpha, *beta,
      //        "stator"), attributes);
      //    std::cout << "CoreLossFunctional using Steinmetz\n";
      // }
   }
   else
   {
      // if (options.contains("UseCAL2forCoreLoss") &&
      //     options["UseCAL2forCoreLoss"].get<bool>())
      // {
      output.addOutputDomainIntegrator(new CAL2CoreLossIntegrator(
          *rho, *CAL2_kh, *CAL2_ke, *peak_flux, *temperature_field));
      //    std::cout << "CoreLossFunctional using CAL2\n";
      // }
      // else
      // {
      //    output.addOutputDomainIntegrator(
      //        new SteinmetzLossIntegrator(*rho, *k_s, *alpha, *beta));
      //    std::cout << "CoreLossFunctional using Steinmetz\n";
      // }
   }
}

void calcOutput(CAL2CoreLossDistribution &output,
                const MISOInputs &inputs,
                mfem::Vector &out_vec)
{
   setInputs(output.output, inputs);
   out_vec = 0.0;
   addLoad(output.output, out_vec);
}

void jacobianVectorProduct(CAL2CoreLossDistribution &output,
                           const mfem::Vector &wrt_dot,
                           const std::string &wrt,
                           mfem::Vector &out_dot)
{
   jacobianVectorProduct(output.output, wrt_dot, wrt, out_dot);
}

double vectorJacobianProduct(CAL2CoreLossDistribution &output,
                             const mfem::Vector &out_bar,
                             const std::string &wrt)
{
   return vectorJacobianProduct(output.output, out_bar, wrt);
}

void vectorJacobianProduct(CAL2CoreLossDistribution &output,
                           const mfem::Vector &out_bar,
                           const std::string &wrt,
                           mfem::Vector &wrt_bar)
{
   vectorJacobianProduct(output.output, out_bar, wrt, wrt_bar);
}

CAL2CoreLossDistribution::CAL2CoreLossDistribution(
    std::map<std::string, FiniteElementState> &fields,
    const nlohmann::json &components,
    const nlohmann::json &materials,
    const nlohmann::json &options)
 : output(fields.at("temperature").space(), fields, "thermal_adjoint"),
   rho(constructMaterialCoefficient("rho", components, materials)),
   CAL2_kh(std::make_unique<CAL2khCoefficient>(components, materials)),
   CAL2_ke(std::make_unique<CAL2keCoefficient>(components, materials))
{
   auto &peak_flux = fields.at("peak_flux");
   auto &temp = fields.at("temperature");

   if (options.contains("attributes"))
   {
      auto current_attributes = options["attributes"].get<std::vector<int>>();
      output.addDomainIntegrator(
          new CAL2CoreLossDistributionIntegrator(
              *rho, *CAL2_kh, *CAL2_ke, peak_flux.gridFunc(), temp.gridFunc()),
          current_attributes);
   }
   else
   {
      output.addDomainIntegrator(new CAL2CoreLossDistributionIntegrator(
          *rho, *CAL2_kh, *CAL2_ke, peak_flux.gridFunc(), temp.gridFunc()));
   }
}

void calcOutput(EMHeatSourceOutput &output,
                const MISOInputs &inputs,
                mfem::Vector &out_vec)
{
   setInputs(output, inputs);
   out_vec = 0.0;
   calcOutput(output.dc_loss, inputs, out_vec);
   output.scratch = 0.0;
   calcOutput(output.ac_loss, inputs, output.scratch);
   out_vec += output.scratch;
   output.scratch = 0.0;
   calcOutput(output.core_loss, inputs, output.scratch);
   // std::cout << "output.scratch norml2 " << output.scratch.Norml2() << "\n";
   out_vec += output.scratch;
   // std::cout << "out_vec norml2 " << out_vec.Norml2() << "\n";

   auto &peak_flux = output.fields.at("peak_flux");
   miso::ParaViewLogger paraview("peak_flux", &peak_flux.mesh());
   paraview.registerField("peak_flux", peak_flux.gridFunc());
   paraview.saveState(peak_flux.gridFunc(), "peak_flux", 0, 0, 0);
}

void jacobianVectorProduct(EMHeatSourceOutput &output,
                           const mfem::Vector &wrt_dot,
                           const std::string &wrt,
                           mfem::Vector &out_dot)
{
   jacobianVectorProduct(output.dc_loss, wrt_dot, wrt, out_dot);
   jacobianVectorProduct(output.ac_loss, wrt_dot, wrt, out_dot);
   jacobianVectorProduct(output.core_loss, wrt_dot, wrt, out_dot);
}

double vectorJacobianProduct(EMHeatSourceOutput &output,
                             const mfem::Vector &out_bar,
                             const std::string &wrt)
{
   auto wrt_bar = vectorJacobianProduct(output.dc_loss, out_bar, wrt);
   wrt_bar += vectorJacobianProduct(output.ac_loss, out_bar, wrt);
   wrt_bar += vectorJacobianProduct(output.core_loss, out_bar, wrt);
   return wrt_bar;
}

void vectorJacobianProduct(EMHeatSourceOutput &output,
                           const mfem::Vector &out_bar,
                           const std::string &wrt,
                           mfem::Vector &wrt_bar)
{
   vectorJacobianProduct(output.dc_loss, out_bar, wrt, wrt_bar);
   vectorJacobianProduct(output.ac_loss, out_bar, wrt, wrt_bar);
   vectorJacobianProduct(output.core_loss, out_bar, wrt, wrt_bar);
}

EMHeatSourceOutput::EMHeatSourceOutput(
    std::map<std::string, FiniteElementState> &fields,
    StateCoefficient &sigma,
    const nlohmann::json &components,
    const nlohmann::json &materials,
    const nlohmann::json &options)
 : dc_loss(fields, sigma, options["dc_loss"]),
   ac_loss(fields, sigma, options["ac_loss"]),
   core_loss(fields, components, materials, options["core_loss"]),
   fields(fields),
   scratch(getSize(dc_loss))
{ }

void setOptions(PMDemagOutput &output, const nlohmann::json &options)
{
   // setOptions(output.lf, options);
   setOptions(output.output, options);
}

void setInputs(PMDemagOutput &output, const MISOInputs &inputs)
{
   // setInputs(output.lf, inputs);

   output.inputs = inputs;
   output.inputs["state"] = inputs.at("peak_flux");
   // output.inputs["state"] = inputs.at("pm_demag_field"); // causes
   // temperature to be 1 exclusively

   setInputs(output.output, inputs);
}

double calcOutput(PMDemagOutput &output, const MISOInputs &inputs)
{
   setInputs(output, inputs);

   // mfem::Vector flux_state;
   // setVectorFromInputs(inputs, "peak_flux", flux_state, false, true);
   // ///TODO: Remove once done debugging
   // std::cout << "flux_state.Size() = " << flux_state.Size() << "\n";
   // std::cout << "flux_state.Min() = " << flux_state.Min() << "\n";
   // std::cout << "flux_state.Max() = " << flux_state.Max() << "\n";
   // std::cout << "flux_state.Sum() = " << flux_state.Sum() << "\n";
   // std::cout << "flux_state=np.array([";
   // for (int j = 0; j < flux_state.Size(); j++) {std::cout <<
   // flux_state.Elem(j) << ", ";} std::cout << "])\n";

   // mfem::Vector temperature_vector;
   // setVectorFromInputs(inputs, "temperature", temperature_vector, false,
   // true);
   // ///TODO: Remove once done debugging
   // std::cout << "temperature_vector.Size() = " << temperature_vector.Size()
   // << "\n"; std::cout << "temperature_vector.Min() = " <<
   // temperature_vector.Min() << "\n"; std::cout << "temperature_vector.Max() =
   // " << temperature_vector.Max() << "\n"; std::cout <<
   // "temperature_vector.Sum() = " << temperature_vector.Sum() << "\n";

   return calcOutput(output.output, output.inputs);
}

/// TODO: Implement this method for the AssembleElementVector (or distribution
/// case) for demag rather than singular value
// void calcOutput(PMDemagOutput &output,
//                 const MISOInputs &inputs,
//                 mfem::Vector &out_vec)
// {
//    setInputs(output, inputs);

//    out_vec = 0.0;
//    addLoad(output.lf, out_vec);
// }

PMDemagOutput::PMDemagOutput(std::map<std::string, FiniteElementState> &fields,
                             const nlohmann::json &components,
                             const nlohmann::json &materials,
                             const nlohmann::json &options)
 : output(fields.at("peak_flux").space(), fields),
   PMDemagConstraint(
       std::make_unique<PMDemagConstraintCoefficient>(components, materials))
{
   // /*
   // Making the integrator see the temperature field
   const auto &temp_field_iter =
       fields.find("temperature");  // find where temperature field is
   mfem::GridFunction *temperature_field =
       nullptr;  // default temperature field to null pointer
   if (temp_field_iter != fields.end())
   {
      // If temperature field exists, turn it into a grid function
      auto &temp_field = temp_field_iter->second;
      temperature_field = &temp_field.gridFunc();

      // std::cout << "temperature_field.Size() = " << temperature_field->Size()
      // << "\n"; std::cout << "temperature_field.Min() = " <<
      // temperature_field->Min() << "\n"; std::cout << "temperature_field.Max()
      // = " << temperature_field->Max() << "\n"; std::cout <<
      // "temperature_field.Sum() = " << temperature_field->Sum() << "\n";

      // std::cout << "PMDemagOutput, electromag_outputs.cpp, temperature field
      // seen\n";
   }

   // Assign the integrator used to compute the singular value for the PMDM
   // constraint coefficient
   if (options.contains("attributes"))
   {
      auto attributes = options["attributes"].get<std::vector<int>>();
      output.addOutputDomainIntegrator(
          new PMDemagIntegrator(*PMDemagConstraint, temperature_field),
          attributes);
   }
   else
   {
      output.addOutputDomainIntegrator(
          new PMDemagIntegrator(*PMDemagConstraint, temperature_field));
   }
   // */
}

}  // namespace miso
