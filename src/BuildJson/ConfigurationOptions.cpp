/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "BuildJson/ConfigurationOptions.hpp"

#include "Terminal/Path.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
const std::string& ConfigurationOptions::name() const noexcept
{
	return m_name;
}

void ConfigurationOptions::setName(const std::string& inValue) noexcept
{
	m_name = inValue;
}

/*****************************************************************************/
const std::string& ConfigurationOptions::optimizations() const noexcept
{
	return m_optimizations;
}

void ConfigurationOptions::setOptimizations(const std::string& inValue) noexcept
{
	m_optimizations = parseOptimizations(inValue);
}

/*****************************************************************************/
bool ConfigurationOptions::linkTimeOptimization() const noexcept
{
	return m_linkTimeOptimization;
}

void ConfigurationOptions::setLinkTimeOptimization(const bool inValue) noexcept
{
	m_linkTimeOptimization = inValue;
}

/*****************************************************************************/
bool ConfigurationOptions::stripSymbols() const noexcept
{
	return m_stripSymbols;
}
void ConfigurationOptions::setStripSymbols(const bool inValue) noexcept
{
	m_stripSymbols = inValue;
}

/*****************************************************************************/
bool ConfigurationOptions::debugSymbols() const noexcept
{
	return m_debugSymbols;
}

void ConfigurationOptions::setDebugSymbols(const bool inValue) noexcept
{
	m_debugSymbols = inValue;
}

/*****************************************************************************/
std::string ConfigurationOptions::parseOptimizations(const std::string& inValue) noexcept
{
	if (String::equals(inValue, "debug"))
		return "-Og";

	if (String::equals(inValue, "3"))
		return "-O3";

	if (String::equals(inValue, "2"))
		return "-O2";

	if (String::equals(inValue, "1"))
		return "-O1";

	if (String::equals(inValue, "0"))
		return "-O0";

	if (String::equals(inValue, "size"))
		return "-Os";

	if (String::equals(inValue, "fast"))
		return "-Ofast";

	return std::string();
}
}
