#pragma once

#include <vector>
#include <glm/glm.hpp>

namespace molumes
{
	class Protein
	{
	public:

		Protein();
		Protein(const std::string& filename);
		void load(const std::string& filename);
		const std::vector<glm::vec4> & atoms() const;
		glm::vec3 minimumBounds() const;
		glm::vec3 maximumBounds() const;

	private:
		std::vector<glm::vec4> m_atoms;
		glm::vec3 m_minimumBounds = glm::vec3(0.0);
		glm::vec3 m_maximumBounds = glm::vec3(0.0);
	};
}