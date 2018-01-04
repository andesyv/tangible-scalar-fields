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
#include <iostream>

namespace molumes
{
	class Molume
	{

	public:
		Molume();

		void setBounds(const glm::vec3& minimumBounds, const glm::vec3& maximumBounds);
		const glm::vec3& minimumBounds() const;
		const glm::vec3& maximumBounds() const;
		const glm::mat4& transform() const;

		void insert(const std::vector<glm::vec3>& positions);
		void setValue(const glm::vec3& pos, float val);
		glm::vec4 value(const glm::vec3& pos) const;

		const std::vector<glm::vec3>& pagePositions() const;
		size_t pageIndex(const glm::ivec3& pagePosition) const;
		const glm::u8vec4 * pageData(size_t index) const;
		

		glm::ivec3 pageCount() const;
		glm::ivec3 cellCount() const;
		size_t numberOfPages() const;


		static glm::ivec3 pageSize();


	private:

		std::vector< std::unique_ptr<glm::u8vec4[]> > m_pages;
		std::vector< glm::vec3 > m_pagePositions;
		glm::vec3 m_minimumBounds = glm::vec3(0.0);
		glm::vec3 m_maximumBounds = glm::vec3(0.0);
		glm::ivec3 m_pageCount = glm::ivec3(0);
		glm::ivec3 m_cellCount = glm::ivec3(0);
		glm::mat4 m_transform = glm::mat4(1.0);
		const float m_resolutionScale = 2.0f;
	};

}
