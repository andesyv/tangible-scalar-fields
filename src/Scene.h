#pragma once

#include <memory>

namespace molumes
{
	class Protein;

	class Scene
	{
	public:
		Scene();
		Protein* protein();

	private:
		std::unique_ptr<Protein> m_protein;
	};


}