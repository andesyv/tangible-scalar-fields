#pragma once

#include <memory>

namespace molumes
{
	class Table;

	class Scene
	{
	public:
		Scene();
		Table* table();

	private:
		std::unique_ptr<Table> m_table;
	};


}