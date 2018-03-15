#pragma once

#include <glm/glm.hpp>

class GFSDK_SSAO_Context_GL;

namespace molumes
{
	class SSAO
	{
	public:
		SSAO();
		void display(const glm::mat4 & viewTransform, const glm::mat4 & projectionTransform, glm::uint frameBuffer, glm::uint depthTexture, glm::uint normalTexture);

	private:

		GFSDK_SSAO_Context_GL* m_aoContext = nullptr;

	};

}