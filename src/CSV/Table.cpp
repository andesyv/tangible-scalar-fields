#pragma once

#include "Table.h"

#include <string>
#include <limits>
#include <unordered_map>
#include <string_view>

using namespace molumes;
using namespace glm;

#ifdef HAVE_MATLAB
#include <engine.h>

#endif

auto stripSign(const std::string_view& str) {
    return (str.starts_with('-') || str.starts_with('+')) ? str.substr(1) : str;
}

bool isInteger(const std::string_view& str) {
    const auto& s = stripSign(str);
    return std::all_of(s.begin(), s.end(), [](char c){ return std::isdigit(c); });
}

bool isFloat(const std::string_view& str) {
    auto separator = str.find_first_of(".,");
    if (separator == std::string_view::npos)
        return false;

    return isInteger(str.substr(0, separator)) && isInteger(str.substr(separator+1));
}

Table::Table()
{

}

const std::string & molumes::Table::filename() const
{
	return m_filename;
}

Table::Table(const std::string& filename)
{
	load(m_filename);
}

const std::vector<std::string> Table::getColumnNames()
{
	return m_columnNames;
}

void Table::load(const std::string& filename)
{

	m_filename = filename;

	rapidcsv::Document loadedDocument(m_filename, rapidcsv::LabelParams(0, -1));
	m_csvDocument = loadedDocument;

	// get titles of all columns
	m_columnNames = m_csvDocument.GetColumnNames();
    std::erase_if(m_columnNames, [this](const auto& name){
        const auto& col = m_csvDocument.GetColumn<std::string>(name);
        return col.empty() || !isFloat(col.front()) && !isInteger(col.front());
    });

	// clear buffers if they already contain data ----------
	m_activeXColumn.clear();
	m_activeYColumn.clear();
	//------------------------------------------------------

	// clear table if it already contains data
	m_tableData.clear();

	// fill table with column vectors
	for (const auto& col : m_columnNames)
	{
        m_tableData.push_back(m_csvDocument.GetColumn<float>(col));
	}
}


const void  Table::updateBuffers(int xID, int yID, int radiusID, int colorID)
{

	m_minimumBounds = vec3(std::numeric_limits<float>::max());
	m_maximumBounds = vec3(-std::numeric_limits<float>::max());

	// clear active columns and prepare
	m_activeXColumn.clear();
	m_activeYColumn.clear();
	m_activeRadiusColumn.clear();
	m_activeColorColumn.clear();

	m_activeTableData.clear();

	//assign column vectors
	m_activeXColumn = m_csvDocument.GetColumn<float>(m_columnNames[xID]);
	m_activeYColumn = m_csvDocument.GetColumn<float>(m_columnNames[yID]);
	m_activeRadiusColumn = m_csvDocument.GetColumn<float>(m_columnNames[radiusID]);
	m_activeColorColumn = m_csvDocument.GetColumn<float>(m_columnNames[colorID]);

	std::vector<vec4> tableEntries;

	// update bounding volume 
	for (int i = 0; i < m_tableData[0].size(); i++)
	{

		// fill atom vector
		vec4 entry = vec4(m_activeXColumn[i], m_activeYColumn[i], m_activeRadiusColumn[i], m_activeColorColumn[i]);
		tableEntries.push_back(entry);

		// update bounding volume depending on X and Y values
		m_minimumBounds = min(m_minimumBounds, vec3(entry));
		m_maximumBounds = max(m_maximumBounds, vec3(entry));

	}

	m_activeTableData.push_back(tableEntries);

}

vec3 Table::minimumBounds() const
{
	return m_minimumBounds;
}

vec3 Table::maximumBounds() const
{
	return m_maximumBounds;
}

const std::vector< std::vector<glm::vec4> > & Table::activeTableData() const
{
	return m_activeTableData;
}

const std::vector<float>& Table::activeXColumn() const
{
	return m_activeXColumn;
}

const std::vector<float>& Table::activeYColumn() const
{
	return m_activeYColumn;
}

const std::vector<float>& Table::activeRadiusColumn() const
{
	return m_activeRadiusColumn;
}

const std::vector<float>& Table::activeColorColumn() const
{
	return m_activeColorColumn;
}
