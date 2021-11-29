/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Core/DotEnvFileGenerator.hpp"

namespace chalet
{
/*****************************************************************************/
DotEnvFileGenerator::DotEnvFileGenerator(const std::string& inFilename) :
	m_filename(inFilename)
{
}

/*****************************************************************************/
const std::string& DotEnvFileGenerator::filename() const noexcept
{
	return m_filename;
}

/*****************************************************************************/
void DotEnvFileGenerator::set(const std::string& inKey, const std::string& inValue)
{
	m_variables[inKey] = inValue;
}

/*****************************************************************************/
bool DotEnvFileGenerator::save()
{
	std::string contents;
	for (auto& [name, var] : m_variables)
	{
		contents += fmt::format("{}={}\n", name, var);
	}

	std::ofstream(m_filename) << contents;

	return true;
}

}
