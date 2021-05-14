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
bool ConfigurationOptions::enableProfiling() const noexcept
{
	return m_enableProfiling;
}
void ConfigurationOptions::setEnableProfiling(const bool inValue) noexcept
{
	m_enableProfiling = inValue;
}

/*****************************************************************************/
std::string ConfigurationOptions::parseOptimizations(const std::string& inValue) noexcept
{
	if (String::equals("debug", inValue))
		return "-Og";

	if (String::equals("3", inValue))
		return "-O3";

	if (String::equals("2", inValue))
		return "-O2";

	if (String::equals("1", inValue))
		return "-O1";

	if (String::equals("0", inValue))
		return "-O0";

	if (String::equals("size", inValue))
		return "-Os";

	if (String::equals("fast", inValue))
		return "-Ofast";

	return std::string();
}
}
