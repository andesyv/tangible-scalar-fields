#include "Molume.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp>

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
	m_pageCount = ivec3(ceil(size / vec3(pageSize)));

	m_pages.reserve(m_pageCount.x*m_pageCount.y*m_pageCount.z);

	m_transform = mat4(1.0f);
	m_transform = translate(m_transform, minimumBounds);
	m_transform = scale(m_transform, vec3(1.0f)/ m_resolutionScale);

}

const vec3& Molume::minimumBounds() const
{
	return m_minimumBounds;
}

const vec3& Molume::maximumBounds() const
{
	return m_maximumBounds;
}

const mat4& Molume::transform() const
{
	return m_transform;
}

void Molume::insert(const std::vector<vec3>& positions)
{
	static ivec3 pageSize = Molume::pageSize();

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

	m_pagePositions.clear();
	m_pagePositions.reserve(m_pages.size());

	for (int z = 0; z < m_pageCount.z; z++)
	{
		for (int y = 0; y < m_pageCount.y; y++)
		{
			for (int x = 0; x < m_pageCount.x; x++)
			{
				size_t pageIndex = x + y*m_pageCount.x + z*m_pageCount.x*m_pageCount.y;

				if (pageIndex < m_pages.size())
				{
					if (m_pages.at(pageIndex) != nullptr)
					{
						m_pagePositions.push_back(vec3(x, y, z)*vec3(pageSize));
						//std::cout << to_string(vec3(x, y, z)*vec3(pageSize)) << std::endl;
					}
				}
			}
		}
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

	size_t pageIndex = pagePosition.x + pagePosition.y*m_pageCount.x + pagePosition.z*m_pageCount.x*m_pageCount.y;

	if (pageIndex >= 0)
	{
		if (m_pages.size() <= pageIndex)
			m_pages.resize(pageIndex + 1);

		if (m_pages[pageIndex] == nullptr)
		{
			m_pages[pageIndex] = std::make_unique<u8vec4[]>(pageSize.x*pageSize.y*pageSize.z);
		}
		
		size_t cellIndex = cellPosition.x + cellPosition.y*pageSize.x + cellPosition.z*pageSize.x*pageSize.y;

		if (m_pages[pageIndex][cellIndex].w > 0)
			globjects::warning() << "Page " << int(pageIndex) << ", cell " << int(cellIndex) << " for positon " << to_string(pos) << " already set -- overwriting!";

		m_pages[pageIndex][cellIndex] = u8vec4((uint8)255.0f*cellOffset.x, (uint8)255.0f*cellOffset.y, (uint8)255.0f*cellOffset.z, (uint8)255.0f*val);		
	}
}

vec4 Molume::value(const vec3& pos) const
{
	return vec4(0.0);
}

const std::vector<glm::vec3>& Molume::pagePositions() const
{
	return m_pagePositions;
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

size_t Molume::pageIndex(const glm::ivec3& pagePosition) const
{
	size_t pageIndex = pagePosition.x + pagePosition.y*m_pageCount.x + pagePosition.z*m_pageCount.x*m_pageCount.y;
	return pageIndex;
}

const glm::u8vec4 * Molume::pageData(size_t index) const
{
	if (index >= m_pages.size())
		return nullptr;

	return m_pages.at(index).get();
}

ivec3 Molume::pageCount() const
{
	return m_pageCount;
}
ivec3 Molume::cellCount() const
{
	static ivec3 pageSize = Molume::pageSize();
	return m_pageCount*pageSize;
}

size_t Molume::numberOfPages() const
{
	return m_pages.size();
}


