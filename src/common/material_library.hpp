#ifndef MACH_MATERIAL_LIBRARY
#define MACH_MATERIAL_LIBRARY

#include "json.hpp"

namespace mach
{

/// Defines the material library for for mach
///
/// This is placed in a hpp file instead of a json file so that it can be
/// compiled in and doesn't require a path to it. This also allows comments.
nlohmann::json material_library
{
	{"box1",
	{
		{"mu_r", 1.0}
	}},
	{"box2",
	{
		{"mu_r", 1.0}
	}},
	{"testmat",
	{
      {"mu_r", 1},
      {"rho", 1},
      {"cv", 1},
      {"kappa", 1},
      {"kappae", 1},
      {"sigma", -1},
      {"kh", -0.00013333333333333},
      {"ke", -3.55555555555e-8},
      {"alpha", 1.0},
	}},
	{"steel", {
		{"B",
		{
			0.0000000000000000,
			0.0044450000000000,
			0.0058190000000000,
			0.0071930000000000,
			0.0085680000000000,
			0.0128980000000000,
			0.0172280000000000,
			0.0274700000000000,
			0.0465790000000000,
			0.0922889999999999,
			0.2207600000000000,
			0.4113000000000000,
			0.6018500000000000,
			0.7717000000000000,
			0.9149500000000000,
			1.0464000000000000,
			1.1600999999999900,
			1.2619000000000000,
			1.3431000000000000,
			1.3947000000000000,
			1.4286000000000000,
			1.4507000000000000,
			1.4668000000000000,
			1.4830000000000000,
			1.4932000000000000,
			1.5064000000000000,
			1.5166999999999900,
			1.5268999999999900,
			1.5371999999999900,
			1.5474000000000000,
			1.5576000000000000,
			1.5679000000000000,
			1.5840000000000000,
			1.5972000000000000,
			1.6104000000000000,
			1.6266000000000000,
			1.6427000000000000,
			1.6589000000000000,
			1.6779999999999900,
			1.6971000000000000,
			1.7161999999999900,
			1.7353000000000000,
			1.7604000000000000,
			1.7854000000000000,
			1.8104000000000000,
			1.8384000000000000,
			1.8723000000000000,
			1.9060999999999900,
			1.9459000000000000,
			1.9886999999999900,
			2.0344000000000000,
			2.0741999999999900,
			2.1080999999999900,
			2.1389999999999900,
			2.1610999999999900,
			2.1772000000000000,
			2.1903999999999900
		}},
		{"H",
		{
			0.0000000000000000,
			9.5103069999999900,
			11.2124700000000000,
			13.2194140000000000,
			15.5852530000000000,
			18.3712620000000000,
			21.6562209999999000,
			25.5213000000000000,
			30.0619920000000000,
			35.3642410000000000,
			41.4304339999999000,
			48.3863029999999000,
			56.5103700000000000,
			66.0660359999999000,
			77.3405759999999000,
			90.5910259999999000,
			106.2120890000000000,
			124.5944920000000000,
			146.3111910000000000,
			172.0624699999990000,
			202.5247369999990000,
			238.5255980000000000,
			281.0120259999990000,
			331.0583149999990000,
			390.1446090000000000,
			459.6953439999990000,
			541.7317890000000000,
			638.4104939999990000,
			752.3336430000000000,
			886.5729270000000000,
			1044.7729970000000000,
			1231.2230800000000000,
			1450.5386699999900000,
			1709.1655450000000000,
			2013.8677920000000000,
			2372.5235849999900000,
			2795.1596880000000000,
			3292.9965269999900000,
			3878.9256599999900000,
			4569.1013169999900000,
			5382.0650580000000000,
			6339.7006929999900000,
			7465.5631620000000000,
			8791.7222000000000000,
			10352.2369750000000000,
			12188.8856750000000000,
			14347.8232499999000000,
			16887.9370500000000000,
			19872.0933000000000000,
			23380.6652749999000000,
			27504.3713250000000000,
			32364.9650250000000000,
			38095.3407999999000000,
			44847.4916749999000000,
			52819.5656250000000000,
			62227.2176750000000000,
			73321.1169499999000000
		}},
		{"mu_r", 2000},
		{"rho", 7.750},
		{"cv", 0.420}, 
		{"kappa", 50},
		{"kh", 0.02},
		{"ke", 0.0001},
		{"alpha", 1.0}
	}},
	{"NdFeB",
	{
		{"mu_r", 1.0},
		{"B_r", 1},
		{"rho", 8500},
		{"cv", 420},
	}},
	{"air",
	{
		{"mu_r", 1},
		{"rho", 1.225},
		{"cv", 93},
		{"kappa", 0.026},
	}},
	{"copper", // this is meant for the heatsink
	{
		{"mu_r", 1},
		{"rho", 5.000},
		{"cv", 0.376},
		{"kappa", 23},
		{"kappae", 10},
		{"sigma", 0.04},
	}},
	{"copperwire", // meant for windings - should have reduced conductivity
	{
		{"mu_r", 1},
		{"rho", 5.000},
		{"cv", 0.376},
		{"kappa", 23},
		{"kappae", 10},
		{"sigma", 0.04},
	}}
};

} // namespace mach

#endif
