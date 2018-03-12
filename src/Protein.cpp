#include "Protein.h"

#include <fstream>
#include <string>
#include <sstream>
#include <cstdlib>
#include <iterator>
#include <iostream>
#include <limits>

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
	std::cout << "Loading file " << filename << " ..." << std::endl;
	std::ifstream file(filename);

	if (!file.is_open())
	{
		std::cerr << "Could not open file " << filename << "!" << std::endl << std::endl;
	}

	m_atoms.clear();
	m_minimumBounds = vec3(std::numeric_limits<float>::max());
	m_maximumBounds = vec3(-std::numeric_limits<float>::max());

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
			vec3 pos(x, y, z);
			m_atoms.push_back(pos);

			m_minimumBounds = min(m_minimumBounds, pos);
			m_maximumBounds = max(m_maximumBounds, pos);
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

const std::vector<glm::vec3> & Protein::atoms() const
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
