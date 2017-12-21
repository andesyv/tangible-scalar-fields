#pragma once

#include <glm/glm.hpp>
#include <glbinding/gl/gl.h>
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>
#include <glm/glm.hpp>
#include <globjects/globjects.h>
#include <globjects/logging.h>

#include <memory>
#include <vector>

namespace molumes
{
	class Molume
	{

	public:
		Molume();

		void setBounds(const glm::vec3& minimumBounds, const glm::vec3& maximumBounds);
		const glm::vec3& minimumBounds() const;
		const glm::vec3& maximumBounds() const;

		void insert(const std::vector<glm::vec3>& positions);
		void setValue(const glm::vec3& pos, float val);
		glm::vec4 value(const glm::vec3& pos) const;

		static glm::ivec3 pageSize();

	private:

		std::vector< std::unique_ptr<glm::u8vec4[]> > m_pages;
		glm::vec3 m_minimumBounds = glm::vec3(0.0);
		glm::vec3 m_maximumBounds = glm::vec3(0.0);
		glm::ivec3 m_size = glm::ivec3(0);
		const float m_resolutionScale = 2.0f;
	};

}
