#include "nlohmann/json.hpp"
#include "material_library.hpp"

namespace mach
{
/// Defines the material library for for mach

const auto hiperco50 = R"(
{
   "reluctivity": {
      "lognu": {
         "cps": [
            5.401,
            4.3732,
            3.9991,
            3.8783,
            3.8492,
            3.8882,
            4.3043,
            4.8375,
            10.251,
            13.5846,
            13.5848],
         "degree": 2,
         "knots": [
            0,
            0,
            0,
            0.6209,
            0.9571,
            1.1516,
            1.346,
            1.5888,
            1.8193,
            2.3539,
            2.7175,
            3.5,
            3.5,
            3.5]
      },
      "bh": {
         "cps": [
            0,
            15.2187154,
            24.4069859,
            28.7543851,
            32.857424,
            35.3931188,
            38.3409008,
            41.064278,
            43.8435871,
            46.4801737,
            49.6317182,
            52.9859037,
            56.6857479,
            61.5933673,
            66.4821406,
            77.5969027,
            97.5330122,
            121.434401,
            152.150611,
            194.912815,
            326.052424,
            316.221363,
            1174.92265,
            2375.59462,
            3701.26207,
            7186.75547,
            13535.5204,
            21724.8492,
            39788.736],
         "degree": 3,
         "knots": [
            0,
            0,
            0,
            0,
            0.2,
            0.3,
            0.4,
            0.5,
            0.6,
            0.7,
            0.8,
            0.9,
            1,
            1.1,
            1.1976,
            1.3,
            1.4,
            1.4775,
            1.6,
            1.7119,
            1.8,
            1.9,
            2,
            2.1,
            2.1868,
            2.25,
            2.3,
            2.36,
            2.4,
            2.5,
            2.5,
            2.5,
            2.5]
      }
   },
   "core_loss": {
      "steinmetz": {
         "ks": 0.0044,
         "alpha": 1.286,
         "beta": 1.76835
      },
      "CAL2": {
         "T0" : 293.15,
         "kh_T0": [
            0.0997091541786544,
            -0.129193571991623,
            0.0900090637806644,
            -0.0212834836667556],
         "ke_T0": [
            -5.93693970727006e-06,
            0.000117138629373709,
            -0.000130355460369590,
            4.10973552619398e-05],
         "T1" : 473.15,
         "kh_T1": [
            0.0895406177349016,
            -0.0810594723247055,
            0.0377588555136910,
            -0.00511339186996760],
         "ke_T1": [
            1.79301614571386e-05,
            7.45159671115992e-07,
            -1.19410662547280e-06,
            3.53133402660246e-07]			
      }
   },
   "cv": 0.42,
   "kappa": 20,
   "mu_r": 500,
   "rho": 8110
}
)"_json;

const auto team13 = R"(
{
   "reluctivity": {
      "bh": {
         "cps": [
            0.0,
            0.0,
            0.0,
            0.0,
            0.0979,
            1.3841,
            1.800000001641453,
            1.999999999999916,
            2.22,
            2.22,
            2.22,
            2.22
         ],
         "degree": 3,
         "knots": [
            0.0,
            164.46362274610001,
            301.3546318106,
            161.7681804384,
            961.7045565209,
            27015.6213073554,
            138424.0810362731,
            196780.8935033237
         ]
      }
   },
   "mu_r": 750
}
)"_json;

// Adding parameters needed for permanent magnet demagnetization model's
// constraint equation Demag parameters explained in
// mach/src/physics/electromagnetics/pm_demag_constraint_coeff.cpp file
// Manufacturer taken to be Arnold Magnetics. Data sheet:
// https://www.arnoldmagnetics.com/wp-content/uploads/2017/11/N48H-151021.pdf
const auto Nd2Fe14B = R"(
{
   "mu_r": 1.04,
   "B_r": 1.390,
   "alpha_B_r": -0.12,
   "T_ref": 293.15,
   "rho": 7500,
   "cv": 502.08,
   "kappa": 9,
   "max-temp": 583.15,
   "ks": 500,
   "beta": 0.0,
   "alpha": 0.0,
   "Demag": {
      "T0": 293.15,
      "alpha_B_r": -0.12,
      "B_r_T0": 1.39,
      "alpha_H_ci": -0.57,
      "H_ci_T0": -1273.0,
      "alpha_B_knee": 0.005522656,
      "beta_B_knee": -1.4442405898,
      "alpha_H_knee": 5.548346445,
      "beta_H_knee": -2571.4027913402
   }
}
)"_json;
// {"ks", 18.092347463670936,
// "max-temp": 310 + 273.15,  // Curie temp is 310 C

const auto air = R"(
{
   "rho": 1.225,
   "cv": 93,
   "kappa": 0.026
}
)"_json;

const auto moving_air = R"(
{
   "rho": 1.225,
   "cv": 93,
   "kappa": 100
}
)"_json;

// Copper Wire that accounts for temperature dependent conductivity/resistivity
// Current value is (=1 divided by resistivity value from
// https://www.britannica.com/science/resistivity)
/// NOTE: Sigma_T_ref value is slightly less than the 58.14e6 that was there
/// (wasn't sure about where that # came from)
// Current alpha value for resistivity is from
// https://www.engineeringtoolbox.com/resistivity-conductivity-d_418.html
/// TODO: Ultimately remove "sigma": 58.14e6 once sigma logic has moved away
/// from MeshDependentCoefficient & ConstantCoefficient and to StateCoefficient
/// & Conductivity Coefficient
///  TODO: since this is meant for windings, should have reduced conductivity...
// "kappa": 2.49,
const auto copper_wire = R"(
{
   "rho": {
      "materials": ["copper", "epoxy"],
      "weighted_by": "fill_factor",
      "weight": "average"
   },
   "drho_df": {
      "materials": ["copper", "epoxy"],
      "weighted_by": "fill_factor",
      "weight": "daverage"
   },
   "cv": 376,
   "kappa": {
      "materials": ["copper", "epoxy"],
      "weighted_by": "fill_factor",
      "weight": "Hashin-Shtrikman"
   },
   "dkappa_df": {
      "materials": ["copper", "epoxy"],
      "weighted_by": "fill_factor",
      "weight": "dHashin-Shtrikman"
   },
   "conductivity":{
      "linear":{
         "sigma_T_ref": 5.6497e7,
         "T_ref": 293.15,
         "alpha_resistivity": 3.8e-3
      }
   },
   "sigma": 58.14e6
}
)"_json;

// this is meant for the heatsink
const auto copper = R"(
{
   "rho": 8960,
   "drho_df": 8960,
   "cv": 376,
   "kappa": 400
}
)"_json;

const auto epoxy = R"(
{
   "rho": 1200,
   "drho_df": 1200,
   "kappa": 1
}
)"_json;

// motor heatsink from X-57 Reference Paper
const auto Al2024_T3 = R"(
{
   "rho": 2780,
   "cv": 875,
   "kappa": 120
}
)"_json;

const nlohmann::json material_library = {{"hiperco50", hiperco50},
                                         {"team13", team13},
                                         {"Nd2Fe14B", Nd2Fe14B},
                                         {"air", air},
                                         {"moving_air", moving_air},
                                         {"copperwire", copper_wire},
                                         {"copper", copper},
                                         {"2024_T3", Al2024_T3},
                                         {"epoxy", epoxy}};

// const nlohmann::json material_library
// {
//    {"box1",
//       {
//          {"rho", 1},
//          {"cv", 1},
//          {"kappa", 1},
//          {"kappae", 1},
//          {"sigma", -1},
//          {"kh", -0.00013333333333333},
//          {"ke", -3.55555555555e-8},
//          {"alpha", 1.0},
//          {"max-temp", 0.5},
//          // {"mu_r", 1.0} // for real scaled problem
//          {"mu_r", 1 / (4 * M_PI * 1e-7)}  // for unit scaled problem
//       }
//    },
//    {"box2",
//       {
//          {"rho", 1},
//          {"cv", 1},
//          {"kappa", 1},
//          {"kappae", 1},
//          {"sigma", -1},
//          {"kh", -0.00013333333333333},
//          {"ke", -3.55555555555e-8},
//          {"alpha", 1.0},
//          {"max-temp", 0.5},
//          // {"mu_r", 1.0} // for real scaled problem
//          {"mu_r", 1 / (4 * M_PI * 1e-7)}  // for unit scaled problem
//       }
//    },
//    {"testmat",
//       {
//          {"mu_r", 1},
//          {"B_r", 1.0},
//          {"rho", 1},
//          {"cv", 1},
//          {"kappa", 1},
//          {"kappae", 1},
//          {"sigma", -1},
//          {"kh", -0.00013333333333333},
//          {"ke", -3.55555555555e-8},
//          {"alpha", 1.0},
//       }
//    },
//    {"regtestmat1",
//       {
//          {"mu_r", 1},
//          {"rho", 1},
//          {"cv", 1},
//          {"kappa", 1},
//          {"kappae", 1},
//          {"sigma", -1},
//       }
//    },
//    {"regtestmat2",
//       {
//          {"mu_r", 1},
//          {"rho", 1},
//          {"cv", 1},
//          {"kappa", 1},
//          {"kappae", 1},
//          {"sigma", 1},
//       }
//    },
//    {"ideal",
//       {
//          {"mu_r", 1e8},
//          {"rho", 8120.0},
//          {"cv", 0.420},
//          {"kappa", 20},
//          // {"kh", 0.02},
//          // {"ke", 0.0001},
//          {"ks", 0.0044},
//          {"beta", 1.76835},
//          {"alpha", 1.286}
//       }
//    },
//    {"hiperco50",
//       {
//          {{"reluctivity",
//             {{"lognu",
//                {
//                   {"cps",
//                      {5.4010,
//                      4.3732,
//                      3.9991,
//                      3.8783,
//                      3.8492,
//                      3.8882,
//                      4.3043,
//                      4.8375,
//                      10.2510,
//                      13.5846,
//                      13.5848}
//                   },
//                   {"knots",
//                      {0.0,
//                      0.0,
//                      0.0,
//                      0.6209,
//                      0.9571,
//                      1.1516,
//                      1.3460,
//                      1.5888,
//                      1.8193,
//                      2.3539,
//                      2.7175,
//                      3.5000,
//                      3.5000,
//                      3.5000}
//                   },
//                   {"degree", 2},
//                }
//             }},
//             {{"bh",
//                {
//                   {"cps",
//                      {0.0,
//                      1.52187154e+01,
//                      2.44069859e+01,
//                      2.87543851e+01,
//                      3.28574240e+01,
//                      3.53931188e+01,
//                      3.83409008e+01,
//                      4.10642780e+01,
//                      4.38435871e+01,
//                      4.64801737e+01,
//                      4.96317182e+01,
//                      5.29859037e+01,
//                      5.66857479e+01,
//                      6.15933673e+01,
//                      6.64821406e+01,
//                      7.75969027e+01,
//                      9.75330122e+01,
//                      1.21434401e+02,
//                      1.52150611e+02,
//                      1.94912815e+02,
//                      3.26052424e+02,
//                      3.16221363e+02,
//                      1.17492265e+03,
//                      2.37559462e+03,
//                      3.70126207e+03,
//                      7.18675547e+03,
//                      1.35355204e+04,
//                      2.17248492e+04,
//                      3.97887360e+04}
//                   },
//                   {"knots",
//                      {0.0, 0.0, 0.0,    0.0,    0.2, 0.3,  0.4,    0.5, 0.6,
//                      0.7, 0.8,
//                      0.9, 1.0, 1.1,    1.1976, 1.3, 1.4,  1.4775, 1.6, 1.7119,
//                      1.8, 1.9,
//                      2.,  2.1, 2.1868, 2.25,   2.3, 2.36, 2.4,    2.5, 2.5,
//                      2.5, 2.5}
//                   },
//                   {"degree", 3}
//                }
//             }}
//          }},
//          {"mu_r", 500},
//          {"rho", 8110.0},
//          {"cv", 0.420},
//          {"kappa", 20},
//          {"ks", 0.0044},
//          {"beta", 1.76835},
//          {"alpha", 1.286}
//       }
//    },
//    {"hiperco50old",
//       {
//          {"B",
//          //  {0.0,
//          //   0.0,
//          //   0.0,
//          //   0.0,
//          //   1.424590497285806,
//          //   1.879802197738017,
//          //   2.089177970766961,
//          //   2.184851867536579,
//          //   2.229448683,
//          //   2.264761102805212,
//          //   2.302883806114454,
//          //   7.290145827,
//          //   7.290145827,
//          //   7.290145827,
//          //   7.290145827}},
//             {0.,  0.,  0.,     0.,     0.2, 0.3,  0.4,    0.5, 0.6,    0.7,
//             0.8,
//             0.9, 1.,  1.1,    1.1976, 1.3, 1.4,  1.4775, 1.6, 1.7119, 1.8, 1.9,
//             2.,  2.1, 2.1868, 2.25,   2.3, 2.36, 2.4,    2.5, 2.5,    2.5, 2.5}
//          },
//          {"H",
//          //  {0.0,
//          //   8.993187962999999,
//          //   10.978287105,
//          //   83.247282676,
//          //   211.230768762,
//          //   458.336047457,
//          //   1159.131573849,
//          //   2773.494054824,
//          //   1.342303920594017e6,
//          //   2.675328620250853e6,
//          //   3.9982409587815357e6}},
//             {-8.45567148e-16, 1.52187154e+01, 2.44069859e+01, 2.87543851e+01,
//             3.28574240e+01,  3.53931188e+01, 3.83409008e+01, 4.10642780e+01,
//             4.38435871e+01,  4.64801737e+01, 4.96317182e+01, 5.29859037e+01,
//             5.66857479e+01,  6.15933673e+01, 6.64821406e+01, 7.75969027e+01,
//             9.75330122e+01,  1.21434401e+02, 1.52150611e+02, 1.94912815e+02,
//             3.26052424e+02,  3.16221363e+02, 1.17492265e+03, 2.37559462e+03,
//             3.70126207e+03,  7.18675547e+03, 1.35355204e+04, 2.17248492e+04,
//             3.97887360e+04}
//          },
//          {"mu_r", 500},
//          {"rho", 8110.0},
//          {"cv", 0.420},
//          {"kappa", 20},
//          // {"kh", 0.02},
//          // {"ke", 0.0001},
//          {"ks", 0.0044},
//          {"beta", 1.76835},
//          {"alpha", 1.286}
//       }
//    },
//    {"ansys-steel",
//       {
//          {"mu_r", 10000},
//       }
//    },
//    {"steel",
//       {
//          {"B",
//             {0.0000000000000000, 0.0044450000000000, 0.0058190000000000,
//             0.0071930000000000, 0.0085680000000000, 0.0128980000000000,
//             0.0172280000000000, 0.0274700000000000, 0.0465790000000000,
//             0.0922889999999999, 0.2207600000000000, 0.4113000000000000,
//             0.6018500000000000, 0.7717000000000000, 0.9149500000000000,
//             1.0464000000000000, 1.1600999999999900, 1.2619000000000000,
//             1.3431000000000000, 1.3947000000000000, 1.4286000000000000,
//             1.4507000000000000, 1.4668000000000000, 1.4830000000000000,
//             1.4932000000000000, 1.5064000000000000, 1.5166999999999900,
//             1.5268999999999900, 1.5371999999999900, 1.5474000000000000,
//             1.5576000000000000, 1.5679000000000000, 1.5840000000000000,
//             1.5972000000000000, 1.6104000000000000, 1.6266000000000000,
//             1.6427000000000000, 1.6589000000000000, 1.6779999999999900,
//             1.6971000000000000, 1.7161999999999900, 1.7353000000000000,
//             1.7604000000000000, 1.7854000000000000, 1.8104000000000000,
//             1.8384000000000000, 1.8723000000000000, 1.9060999999999900,
//             1.9459000000000000, 1.9886999999999900, 2.0344000000000000,
//             2.0741999999999900, 2.1080999999999900, 2.1389999999999900,
//             2.1610999999999900, 2.1772000000000000, 2.1903999999999900}
//          },
//          {"H",
//             {0.0000000000000000,     9.5103069999999900,
//             11.2124700000000000,    13.2194140000000000,
//             15.5852530000000000,    18.3712620000000000,
//             21.6562209999999000,    25.5213000000000000,
//             30.0619920000000000,    35.3642410000000000,
//             41.4304339999999000,    48.3863029999999000,
//             56.5103700000000000,    66.0660359999999000,
//             77.3405759999999000,    90.5910259999999000,
//             106.2120890000000000,   124.5944920000000000,
//             146.3111910000000000,   172.0624699999990000,
//             202.5247369999990000,   238.5255980000000000,
//             281.0120259999990000,   331.0583149999990000,
//             390.1446090000000000,   459.6953439999990000,
//             541.7317890000000000,   638.4104939999990000,
//             752.3336430000000000,   886.5729270000000000,
//             1044.7729970000000000,  1231.2230800000000000,
//             1450.5386699999900000,  1709.1655450000000000,
//             2013.8677920000000000,  2372.5235849999900000,
//             2795.1596880000000000,  3292.9965269999900000,
//             3878.9256599999900000,  4569.1013169999900000,
//             5382.0650580000000000,  6339.7006929999900000,
//             7465.5631620000000000,  8791.7222000000000000,
//             10352.2369750000000000, 12188.8856750000000000,
//             14347.8232499999000000, 16887.9370500000000000,
//             19872.0933000000000000, 23380.6652749999000000,
//             27504.3713250000000000, 32364.9650250000000000,
//             38095.3407999999000000, 44847.4916749999000000,
//             52819.5656250000000000, 62227.2176750000000000,
//             73321.1169499999000000}
//          },
//          {"mu_r", 750},
//          {"rho", 7.750},
//          {"cv", 0.420},
//          {"kappa", 50},
//          // {"kh", 0.02},
//          // {"ke", 0.0001},
//          {"ks", 0.0044},
//          {"beta", 1.76835},
//          {"alpha", 1.286}
//       }
//    },
//    {"team13",
//       {
//          {"B",
//             {// 0.0, 0.0, 0.0, 0.0, 0.1,
//                // 0.4672, 1.1862, 1.4124, 1.6386, 4.3078, 15.8322, 15.8322,
//                // 15.8322, 15.8322 0, 0.025, 0.3,
//                // 0.7, 1.1, 1.5, 1.7, 1.8
//                0.0,
//                0.0,
//                0.0,
//                0.0,
//                0.0979,
//                1.3841,
//                1.800000001641453,
//                1.999999999999916,
//                2.22,
//                2.22,
//                2.22,
//                2.22}
//             },
//          {"H",
//             {// 8.768456660138328,   7.962589775712834,   6.836102567668674,
//                // 5.849726164223915,
//                // 5.817004866729157,   5.925084803960186,  12.847141274392365,
//                // 13.168983405092462,
//                // 13.587072660625301,  13.587072660625301 0.0, 93,
//                // 222, 272, 377, 933, 4993, 9423
//                0.0,
//                164.46362274610001,
//                301.3546318106,
//                161.7681804384,
//                961.7045565209,
//                27015.6213073554,
//                138424.0810362731,
//                196780.8935033237}
//             },
//             {"mu_r", 750}
//          {"B",
//          {
//             0,
//             0.0025,
//             0.005,
//             0.0125,
//             0.025,
//             0.05,
//             0.1,
//             0.2,
//             0.3,
//             0.4,
//             0.5,
//             0.6,
//             0.7,
//             0.8,
//             0.9,
//             1.0,
//             1.1,
//             1.2,
//             1.3,
//             1.4,
//             1.5,
//             1.55,
//             1.6,
//             1.65,
//             1.7,
//             1.75,
//             1.8,
//             1.8435526351982585,
//             1.9179901552741234,
//             1.984275166575515,
//             2.042407669102433,
//             2.092387662854877,
//             2.1342151478328484,
//             2.1678901240363464,
//             2.1934125914653704,
//             2.210782550119921,
//             2.219999999999999,
//             2.222831853071796,
//             2.2605309649148735,
//             2.298230076757951,
//             2.3359291886010287,
//             2.373628300444106
//          }},
//          {"H",
//          {
//             0.0,
//             16,
//             30,
//             54,
//             93,
//             143,
//             191,
//             210,
//             222,
//             233,
//             247,
//             258,
//             272,
//             289,
//             313,
//             342,
//             377,
//             433,
//             509,
//             648,
//             933,
//             1228,
//             1934,
//             2913,
//             4993,
//             7189,
//             9423,
//             11657.0,
//             15794.62323416157,
//             19932.24646832314,
//             24069.869702484713,
//             28207.492936646286,
//             32345.116170807854,
//             36482.73940496943,
//             40620.362639130995,
//             44757.98587329257,
//             48895.60910745414,
//             50000.0,
//             80000.0,
//             110000.0,
//             140000.0,
//             170000.0
//          }}
//       }
//    },
//    {"Nd2Fe14B",
//       {
//          {"mu_r", 1.04},
//          {"B_r", 1.390},
//          // {"B_r", 0.0},
//          {"rho", 7500},
//          {"cv", 502.08},
//          {"kappa", 9},
//          {"max-temp", 310 + 273.15},  // Curie temp is 310 C
//          // {"ks", 18.092347463670936},
//          {"ks", 500},
//          {"beta", 0.0},
//          {"alpha", 0.0}
//       }
//    },
//    {"air",
//       {
//          {"mu_r", 1},
//          {"rho", 1.225},
//          {"cv", 93},
//          {"kappa", 0.026},
//          {"max-temp", 500}
//       }
//    },
//    {"copper",  // this is meant for the heatsink
//       {
//          {"mu_r", 1},
//          {"rho", 8960},
//          {"cv", 376},
//          {"kappa", 400}
//          // {"sigma", 58.14e6},
//       }
//    },
//    {"2024-T3",  // motor heatsink from Reference Paper
//       {
//          {"mu_r", 1},
//          {"rho", 2780},
//          {"cv", 875},
//          {"kappa", 120}
//       }
//    },
//    {"copperwire",  // meant for windings - should have reduced
//                      // conductivity
//       {
//          {"mu_r", 1},
//          {"rho", 8960},
//          {"cv", 376},
//          // {"kappa", 237.6},
//          {"kappa", 2.49},
//          {"sigma", 58.14e6},
//          {"max-temp", 400}
//       }
//    }
// };

}  // namespace mach
