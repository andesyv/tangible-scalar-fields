#include "Molume.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

using namespace molumes;
using namespace gl;
using namespace glm;

Molume::Molume()
{

}

void Molume::setBounds(const vec3& minimumBounds, const vec3& maximumBounds)
{
	static ivec3 pageSize = Molume::pageSize();
	vec3 size = (maximumBounds - minimumBounds)*m_resolutionScale;

	m_minimumBounds = minimumBounds;
	m_maximumBounds = maximumBounds;
	m_size = ivec3(ceil(size / vec3(pageSize)));

	m_pages.reserve(m_size.x*m_size.y*m_size.z);
}

const vec3& Molume::minimumBounds() const
{
	return m_minimumBounds;
}

const vec3& Molume::maximumBounds() const
{
	return m_maximumBounds;
}

void Molume::insert(const std::vector<vec3>& positions)
{
	vec3 minimumBounds = vec3(std::numeric_limits<float>::max());
	vec3 maximumBounds = vec3(-std::numeric_limits<float>::max());
	
	for (auto& p : positions)
	{
		minimumBounds = min(minimumBounds, p);
		maximumBounds = max(maximumBounds, p);
	}

	setBounds(minimumBounds, maximumBounds);

	for (auto& p : positions)
	{
		setValue(p, 1.0f);
	}
}

void Molume::setValue(const vec3 & pos, float val)
{
	static ivec3 pageSize = Molume::pageSize();
	vec3 position = (pos - m_minimumBounds)*m_resolutionScale;

	ivec3 pagePosition = ivec3(position) / pageSize;
	vec3 pageOffset = position - vec3(pagePosition*pageSize);
	ivec3 cellPosition = ivec3(pageOffset);
	vec3 cellOffset = fract(pageOffset);

	int pageIndex = pagePosition.x + pagePosition.y*m_size.x + pagePosition.z*m_size.x*m_size.y;

	if (pageIndex >= 0)
	{
		if (m_pages.size() <= pageIndex)
			m_pages.resize(pageIndex + 1);

		if (m_pages[pageIndex] == nullptr)
		{
			m_pages[pageIndex] = std::make_unique<u8vec4[]>(pageSize.x*pageSize.y*pageSize.z);
		}
		
		int cellIndex = cellPosition.x + cellPosition.y*pageSize.x + cellPosition.z*pageSize.x*pageSize.y;

		if (m_pages[pageIndex][cellIndex].w > 0)
			globjects::warning() << "Page " << pageIndex << ", cell " << cellIndex << " for positon " << to_string(pos) << " already set -- overwriting!";

		m_pages[pageIndex][cellIndex] = u8vec4((uint8)255.0f*cellOffset.x, (uint8)255.0f*cellOffset.y, (uint8)255.0f*cellOffset.z, (uint8)255.0f*val);		
	}
}

vec4 Molume::value(const vec3& pos) const
{
	return vec4(0.0);
}

ivec3 Molume::pageSize()
{
	static ivec3 size(0);

	if (size.x == 0 && size.y == 0 && size.z == 0)
	{
		int numPageSizes;

		GLenum target = GL_TEXTURE_3D;
		GLenum format = GL_RGBA8;

		glGetInternalformativ(target, format, GL_NUM_VIRTUAL_PAGE_SIZES_ARB, sizeof(int), &numPageSizes);
		globjects::info("GL_NUM_VIRTUAL_PAGE_SIZES_ARB = %d;", numPageSizes);

		if (numPageSizes == 0)
		{
			globjects::fatal("Sparse textures are not supported!");
		}
		else
		{
			auto pageSizesX = std::vector<int>(numPageSizes);
			glGetInternalformativ(target, format, GL_VIRTUAL_PAGE_SIZE_X_ARB, static_cast<GLsizei>(numPageSizes * sizeof(int)), pageSizesX.data());
			for (int i = 0; i < numPageSizes; ++i)
				globjects::info("GL_VIRTUAL_PAGE_SIZE_X_ARB[%;] = %;", i, pageSizesX[i]);

			auto pageSizesY = std::vector<int>(numPageSizes);
			glGetInternalformativ(target, format, GL_VIRTUAL_PAGE_SIZE_Y_ARB, static_cast<GLsizei>(numPageSizes * sizeof(int)), pageSizesY.data());
			for (int i = 0; i < numPageSizes; ++i)
				globjects::info("GL_VIRTUAL_PAGE_SIZE_Y_ARB[%;] = %;", i, pageSizesY[i]);

			auto pageSizesZ = std::vector<int>(numPageSizes);
			glGetInternalformativ(target, format, GL_VIRTUAL_PAGE_SIZE_Z_ARB, static_cast<GLsizei>(numPageSizes * sizeof(int)), pageSizesZ.data());
			for (int i = 0; i < numPageSizes; ++i)
				globjects::info("GL_VIRTUAL_PAGE_SIZE_Z_ARB[%;] = %;", i, pageSizesZ[i]);

			size = ivec3(pageSizesX[0], pageSizesY[0], pageSizesZ[0]);
		}
	}

	return size;
}
