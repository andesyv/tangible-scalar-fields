#include "Scene.h"
#include "Protein.h"
#include <iostream>

using namespace molumes;

Scene::Scene()
{
	m_protein = std::make_unique<Protein>();
}

Protein * Scene::protein()
{
	return m_protein.get();
}