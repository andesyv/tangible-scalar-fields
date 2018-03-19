#pragma once

#include <vector>
#include <glm/glm.hpp>

namespace molumes
{
	class Protein
	{
		struct Element
		{
			glm::vec3 color = glm::vec3(1.0, 1.0, 1.0);
			float radius = 1.0;
		};

	public:

		Protein();
		Protein(const std::string& filename);
		void load(const std::string& filename);
		const std::vector<glm::vec4> & atoms() const;
		const std::vector<Element> & elements() const;
		glm::vec3 minimumBounds() const;
		glm::vec3 maximumBounds() const;

	private:
		std::vector<glm::vec4> m_atoms;
		std::vector<Element> m_elements;
		glm::vec3 m_minimumBounds = glm::vec3(0.0);
		glm::vec3 m_maximumBounds = glm::vec3(0.0);
	};
}