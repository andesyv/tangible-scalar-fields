#include "Protein.h"

#include <fstream>
#include <string>
#include <sstream>
#include <cstdlib>
#include <iterator>
#include <iostream>
#include <limits>
#include <unordered_map>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

using namespace molumes;
using namespace glm;

Protein::Protein()
{

}

Protein::Protein(const std::string& filename)
{
	load(filename);
}

void Protein::load(const std::string& filename)
{
	std::unordered_map<std::string, uint> elementTypes;
	elementTypes["H"] = 1;
	elementTypes["He"] = 2;
	elementTypes["Li"] = 3;
	elementTypes["Be"] = 4;
	elementTypes["B"] = 5;
	elementTypes["C"] = 6;
	elementTypes["N"] = 7;
	elementTypes["O"] = 8;
	elementTypes["F"] = 9;
	elementTypes["Ne"] = 10;
	elementTypes["Na"] = 11;
	elementTypes["Mg"] = 12;
	elementTypes["Al"] = 13;
	elementTypes["Si"] = 14;
	elementTypes["P"] = 15;
	elementTypes["S"] = 16;
	elementTypes["Cl"] = 17;
	elementTypes["Ar"] = 18;
	elementTypes["K"] = 19;
	elementTypes["Ca"] = 20;
	elementTypes["Sc"] = 21;
	elementTypes["Ti"] = 22;
	elementTypes["V"] = 23;
	elementTypes["Cr"] = 24;
	elementTypes["Mn"] = 25;
	elementTypes["Fe"] = 26;
	elementTypes["Co"] = 27;
	elementTypes["Ni"] = 28;
	elementTypes["Cu"] = 29;
	elementTypes["Zn"] = 30;
	elementTypes["Ga"] = 31;
	elementTypes["Ge"] = 32;
	elementTypes["As"] = 33;
	elementTypes["Se"] = 34;
	elementTypes["Br"] = 35;
	elementTypes["Kr"] = 36;
	elementTypes["Rb"] = 37;
	elementTypes["Sr"] = 38;
	elementTypes["Y"] = 39;
	elementTypes["Zr"] = 40;
	elementTypes["Nb"] = 41;
	elementTypes["Mo"] = 42;
	elementTypes["Tc"] = 43;
	elementTypes["Ru"] = 44;
	elementTypes["Rh"] = 45;
	elementTypes["Pd"] = 46;
	elementTypes["Ag"] = 47;
	elementTypes["Cd"] = 48;
	elementTypes["In"] = 49;
	elementTypes["Sn"] = 50;
	elementTypes["Sb"] = 51;
	elementTypes["Te"] = 52;
	elementTypes["I"] = 53;
	elementTypes["Xe"] = 54;
	elementTypes["Cs"] = 55;
	elementTypes["Ba"] = 56;
	elementTypes["La"] = 57;
	elementTypes["Ce"] = 58;
	elementTypes["Pr"] = 59;
	elementTypes["Nd"] = 60;
	elementTypes["Pm"] = 61;
	elementTypes["Sm"] = 62;
	elementTypes["Eu"] = 63;
	elementTypes["Gd"] = 64;
	elementTypes["Tb"] = 65;
	elementTypes["Dy"] = 66;
	elementTypes["Ho"] = 67;
	elementTypes["Er"] = 68;
	elementTypes["Tm"] = 69;
	elementTypes["Yb"] = 70;
	elementTypes["Lu"] = 71;
	elementTypes["Hf"] = 72;
	elementTypes["Ta"] = 73;
	elementTypes["W"] = 74;
	elementTypes["Re"] = 75;
	elementTypes["Os"] = 76;
	elementTypes["Ir"] = 77;
	elementTypes["Pt"] = 78;
	elementTypes["Au"] = 79;
	elementTypes["Hg"] = 80;
	elementTypes["Tl"] = 81;
	elementTypes["Pb"] = 82;
	elementTypes["Bi"] = 83;
	elementTypes["Po"] = 84;
	elementTypes["At"] = 85;
	elementTypes["Rn"] = 86;
	elementTypes["Fr"] = 87;
	elementTypes["Ra"] = 88;
	elementTypes["Ac"] = 89;
	elementTypes["Th"] = 90;
	elementTypes["Pa"] = 91;
	elementTypes["U"] = 92;
	elementTypes["Np"] = 93;
	elementTypes["Pu"] = 94;
	elementTypes["Am"] = 95;
	elementTypes["Cm"] = 96;
	elementTypes["Bk"] = 97;
	elementTypes["Cf"] = 98;
	elementTypes["Es"] = 99;
	elementTypes["Fm"] = 100;
	elementTypes["Md"] = 101;
	elementTypes["No"] = 102;
	elementTypes["Lr"] = 103;
	elementTypes["Rf"] = 104;
	elementTypes["Db"] = 105;
	elementTypes["Sg"] = 106;
	elementTypes["Bh"] = 107;
	elementTypes["Hs"] = 108;
	elementTypes["Mt"] = 109;
	elementTypes["Ds"] = 110;
	elementTypes["Rg"] = 111;
	elementTypes["Cn"] = 112;
	elementTypes["H (WAT)"] = 113;
	elementTypes["O (WAT)"] = 114;
	elementTypes["D"] = 115;

	static const float radii[] = {
		1.0f, // 0 - default
		/*<vdw id = "1" radius = "*/1.200f,
		/*<vdw id = "2" radius = "*/1.400f,
		/*<vdw id = "3" radius = "*/1.820f,
		/*<vdw id = "4" radius = "*/1.530f,
		/*<vdw id = "5" radius = "*/1.920f,
		/*<vdw id = "6" radius = "*/1.700f,
		/*<vdw id = "7" radius = "*/1.550f,
		/*<vdw id = "8" radius = "*/1.520f,
		/*<vdw id = "9" radius = "*/1.470f,
		/*<vdw id = "10" radius = "*/1.540f,
		/*<vdw id = "11" radius = "*/2.270f,
		/*<vdw id = "12" radius = "*/1.730f,
		/*<vdw id = "13" radius = "*/1.840f,
		/*<vdw id = "14" radius = "*/2.100f,
		/*<vdw id = "15" radius = "*/1.800f,
		/*<vdw id = "16" radius = "*/1.800f,
		/*<vdw id = "17" radius = "*/1.750f,
		/*<vdw id = "18" radius = "*/1.880f,
		/*<vdw id = "19" radius = "*/2.750f,
		/*<vdw id = "20" radius = "*/2.310f,
		/*<vdw id = "21" radius = "*/2.110f,
		/*<vdw id = "22" radius = "*/2.000f,
		/*<vdw id = "23" radius = "*/2.000f,
		/*<vdw id = "24" radius = "*/2.000f,
		/*<vdw id = "25" radius = "*/2.000f,
		/*<vdw id = "26" radius = "*/2.000f,
		/*<vdw id = "27" radius = "*/2.000f,
		/*<vdw id = "28" radius = "*/1.630f,
		/*<vdw id = "29" radius = "*/1.400f,
		/*<vdw id = "30" radius = "*/1.390f,
		/*<vdw id = "31" radius = "*/1.870f,
		/*<vdw id = "32" radius = "*/2.110f,
		/*<vdw id = "33" radius = "*/1.850f,
		/*<vdw id = "34" radius = "*/1.900f,
		/*<vdw id = "35" radius = "*/1.850f,
		/*<vdw id = "36" radius = "*/2.020f,
		/*<vdw id = "37" radius = "*/3.030f,
		/*<vdw id = "38" radius = "*/2.490f,
		/*<vdw id = "39" radius = "*/2.000f,
		/*<vdw id = "40" radius = "*/2.000f,
		/*<vdw id = "41" radius = "*/2.000f,
		/*<vdw id = "42" radius = "*/2.000f,
		/*<vdw id = "43" radius = "*/2.000f,
		/*<vdw id = "44" radius = "*/2.000f,
		/*<vdw id = "45" radius = "*/2.000f,
		/*<vdw id = "46" radius = "*/1.630f,
		/*<vdw id = "47" radius = "*/1.720f,
		/*<vdw id = "48" radius = "*/1.580f,
		/*<vdw id = "49" radius = "*/1.930f,
		/*<vdw id = "50" radius = "*/2.170f,
		/*<vdw id = "51" radius = "*/2.060f,
		/*<vdw id = "52" radius = "*/2.060f,
		/*<vdw id = "53" radius = "*/1.980f,
		/*<vdw id = "54" radius = "*/2.160f,
		/*<vdw id = "55" radius = "*/3.430f,
		/*<vdw id = "56" radius = "*/2.680f,
		/*<vdw id = "57" radius = "*/2.000f,
		/*<vdw id = "58" radius = "*/2.000f,
		/*<vdw id = "59" radius = "*/2.000f,
		/*<vdw id = "60" radius = "*/2.000f,
		/*<vdw id = "61" radius = "*/2.000f,
		/*<vdw id = "62" radius = "*/2.000f,
		/*<vdw id = "63" radius = "*/2.000f,
		/*<vdw id = "64" radius = "*/2.000f,
		/*<vdw id = "65" radius = "*/2.000f,
		/*<vdw id = "66" radius = "*/2.000f,
		/*<vdw id = "67" radius = "*/2.000f,
		/*<vdw id = "68" radius = "*/2.000f,
		/*<vdw id = "69" radius = "*/2.000f,
		/*<vdw id = "70" radius = "*/2.000f,
		/*<vdw id = "71" radius = "*/2.000f,
		/*<vdw id = "72" radius = "*/2.000f,
		/*<vdw id = "73" radius = "*/2.000f,
		/*<vdw id = "74" radius = "*/2.000f,
		/*<vdw id = "75" radius = "*/2.000f,
		/*<vdw id = "76" radius = "*/2.000f,
		/*<vdw id = "77" radius = "*/2.000f,
		/*<vdw id = "78" radius = "*/1.750f,
		/*<vdw id = "79" radius = "*/1.660f,
		/*<vdw id = "80" radius = "*/1.550f,
		/*<vdw id = "81" radius = "*/1.960f,
		/*<vdw id = "82" radius = "*/2.020f,
		/*<vdw id = "83" radius = "*/2.070f,
		/*<vdw id = "84" radius = "*/1.970f,
		/*<vdw id = "85" radius = "*/2.020f,
		/*<vdw id = "86" radius = "*/2.200f,
		/*<vdw id = "87" radius = "*/3.480f,
		/*<vdw id = "88" radius = "*/2.830f,
		/*<vdw id = "89" radius = "*/2.000f,
		/*<vdw id = "90" radius = "*/2.000f,
		/*<vdw id = "91" radius = "*/2.000f,
		/*<vdw id = "92" radius = "*/1.860f,
		/*<vdw id = "93" radius = "*/2.000f,
		/*<vdw id = "94" radius = "*/2.000f,
		/*<vdw id = "95" radius = "*/2.000f,
		/*<vdw id = "96" radius = "*/2.000f,
		/*<vdw id = "97" radius = "*/2.000f,
		/*<vdw id = "98" radius = "*/2.000f,
		/*<vdw id = "99" radius = "*/2.000f,
		/*<vdw id = "100" radius = "*/2.000f,
		/*<vdw id = "101" radius = "*/2.000f,
		/*<vdw id = "102" radius = "*/2.000f,
		/*<vdw id = "103" radius = "*/2.000f,
		/*<vdw id = "104" radius = "*/2.000f,
		/*<vdw id = "105" radius = "*/2.000f,
		/*<vdw id = "106" radius = "*/2.000f,
		/*<vdw id = "107" radius = "*/2.000f,
		/*<vdw id = "108" radius = "*/2.000f,
		/*<vdw id = "109" radius = "*/2.000f,
		/*<vdw id = "110" radius = "*/2.000f,
		/*<vdw id = "111" radius = "*/2.000f,
		/*<vdw id = "112" radius = "*/2.000f,
		/*<vdw id = "113" radius = "*/1.200f,
		/*<vdw id = "114" radius = "*/1.520f,
		/*<vdw id = "115" radius = "*/1.200f
	};

	std::cout << "Loading file " << filename << " ..." << std::endl;
	std::ifstream file(filename);

	if (!file.is_open())
	{
		std::cerr << "Could not open file " << filename << "!" << std::endl << std::endl;
	}

	m_atoms.clear();
	m_minimumBounds = vec3(std::numeric_limits<float>::max());
	m_maximumBounds = vec3(-std::numeric_limits<float>::max());

	uint j = 0;
	std::string str;
	while (std::getline(file, str))
	{
		std::vector<std::string> tokens;
		std::istringstream buffer(str);

		std::copy(std::istream_iterator<std::string>(buffer), std::istream_iterator<std::string>(), std::back_inserter(tokens));

		if (tokens[0] == "ATOM" || tokens[0] == "HETATM")
		{
			float x = float(std::atof(tokens[6].c_str()));
			float y = float(std::atof(tokens[7].c_str()));
			float z = float(std::atof(tokens[8].c_str()));

			vec4 pos(x, y, z, j > 20000 ? 1.0f : 0.0f);
			m_atoms.push_back(pos);

			j++;

			m_minimumBounds = min(m_minimumBounds, vec3(pos));
			m_maximumBounds = max(m_maximumBounds, vec3(pos));
		}
	}

	std::cout << m_atoms.size() << " atoms loaded from file " << filename << "." << std::endl;
	
	/*
	m_atoms.clear();
	m_atoms.push_back(vec3(-2.0, 3.0, 0.0));//2
	m_atoms.push_back(vec3(-1.3, 0.0, 0.0));
	m_atoms.push_back(vec3(0.0, 2.0, 0.0));
	*/
	
/*	
	m_atoms.clear();
	m_atoms.push_back(vec3(0.0, -2.0, -2.0));

	m_atoms.push_back(vec3(-1.3, 0.0, 0.0));
	m_atoms.push_back(vec3(1.3, 0.0, 0.0));

	m_atoms.push_back(vec3(0.0, 2.0, 0.0));

	m_atoms.push_back(vec3(-2.0, 3.0, 0.0));
	m_atoms.push_back(vec3(2.0, 3.0, 0.0));
	m_atoms.push_back(vec3(0.0, 4.0, 0.0));

	m_atoms.push_back(vec3(0.0, 3.0, 3.0));

	m_atoms.push_back(vec3(0.0, 5.0, 1.0));

	m_atoms.push_back(vec3(0.0, 0.0, 4.0));
	
	
	m_minimumBounds = vec3(-5.0, -5.0, -5.0);
	m_maximumBounds = vec3(5.0, 5.0, 5.0);
*/	
	/*
	// does not produce patch
	m_atoms.push_back(vec3(-2.0, 3.0, 0.0));//2
	m_atoms.push_back(vec3(-1.3, 0.0, 0.0));
	m_atoms.push_back(vec3(0.0, 2.0, 0.0));
	*/

	// Incorrect test case
	/*
	m_atoms.push_back(vec3(0.0, 1.3, 0.0));
	m_atoms.push_back(vec3(-1.5, 0.0, 0.0));
	m_atoms.push_back(vec3(1.5, 0.0, 0.0));
	*/

//	m_atoms.push_back(vec3(1.3, 0.0, 0.0));
//	m_atoms.push_back(vec3(2.0, 3.0, 0.0);
//	m_atoms.push_back(vec3(0.0, 2.0, 0.0));


	// correct patch
/*	m_atoms.push_back(vec3(-1.3, 0.0, 0.0));
	m_atoms.push_back(vec3(1.3, 0.0, 0.0));
	m_atoms.push_back(vec3(0.0, 2.0, 0.0));
	
	m_atoms.push_back(vec3(-2.0, 3.0, 0.0));
	m_atoms.push_back(vec3(2.0, 3.0, 0.0));
*/	

/*
	float minDist = std::numeric_limits<float>::max();

	for (std::size_t i = 0; i < m_atoms.size(); i++)
	{
		const vec3 & posi = m_atoms[i];

		for (std::size_t j = i+1; j < m_atoms.size(); j++)
		{
			const vec3 & posj = m_atoms[j];

			vec3 d = posi - posj;
			float l = length(d);

			minDist = min(minDist, l);
		}
	}
*/
/*
	vec3 boundsSize = (m_maximumBounds - m_minimumBounds);
	ivec3 gridSize(boundsSize*2.0f + vec3(1.0f));

	std::cout << m_atoms.size() << " atoms loaded from file " << filename << "." << std::endl;
	std::cout << "Minimum bounds: " << to_string(m_minimumBounds) << std::endl;
	std::cout << "Maximum bounds: " << to_string(m_maximumBounds) << std::endl;
	//std::cout << "Minimum atom distance: " << minDist << std::endl;
	std::cout << "Grid size: " << to_string(gridSize) << std::endl;
	std::cout << std::endl;


	std::vector<int> molume(gridSize.x*gridSize.y*gridSize.z,0);
	int occupancy = 0;

	for (const auto& a : m_atoms)
	{
		vec3 ap = vec3(a - m_minimumBounds)/ boundsSize;
		vec3 bp = ap*(vec3(gridSize)-vec3(1.0));
		ivec3 gridPos = ivec3(bp);

		std::size_t index = gridPos.x + gridPos.y*gridSize.x + gridPos.z*gridSize.x*gridSize.y;
		molume[index]++;

		if (molume[index] > occupancy)
			occupancy = molume[index];
	}

	std::cout << "Max occupancy: " << occupancy << std::endl;
*/
}

const std::vector<glm::vec4> & Protein::atoms() const
{
	return m_atoms;
}

vec3 Protein::minimumBounds() const
{
	return m_minimumBounds;
}

vec3 Protein::maximumBounds() const
{
	return m_maximumBounds;
}
