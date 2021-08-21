/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/BuildConfiguration.hpp"

#include "Terminal/Path.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
const std::string& BuildConfiguration::name() const noexcept
{
	return m_name;
}

void BuildConfiguration::setName(const std::string& inValue) noexcept
{
	m_name = inValue;
}

/*****************************************************************************/
OptimizationLevel BuildConfiguration::optimizationLevel() const noexcept
{
	return m_optimizationLevel;
}

void BuildConfiguration::setOptimizationLevel(const std::string& inValue) noexcept
{
	m_optimizationLevel = parseOptimizationLevel(inValue);
}

/*****************************************************************************/
bool BuildConfiguration::linkTimeOptimization() const noexcept
{
	return m_linkTimeOptimization;
}

void BuildConfiguration::setLinkTimeOptimization(const bool inValue) noexcept
{
	m_linkTimeOptimization = inValue;
}

/*****************************************************************************/
bool BuildConfiguration::stripSymbols() const noexcept
{
	return m_stripSymbols;
}
void BuildConfiguration::setStripSymbols(const bool inValue) noexcept
{
	m_stripSymbols = inValue;
}

/*****************************************************************************/
bool BuildConfiguration::debugSymbols() const noexcept
{
	return m_debugSymbols;
}

void BuildConfiguration::setDebugSymbols(const bool inValue) noexcept
{
	m_debugSymbols = inValue;
}

/*****************************************************************************/
bool BuildConfiguration::enableProfiling() const noexcept
{
	return m_enableProfiling;
}
void BuildConfiguration::setEnableProfiling(const bool inValue) noexcept
{
	m_enableProfiling = inValue;
}

/*****************************************************************************/
bool BuildConfiguration::isReleaseWithDebugInfo() const noexcept
{
	return (m_optimizationLevel == OptimizationLevel::L2 || m_optimizationLevel == OptimizationLevel::L3) && m_debugSymbols;
}
bool BuildConfiguration::isMinSizeRelease() const noexcept
{
	return m_optimizationLevel == OptimizationLevel::Size && !m_debugSymbols;
}
bool BuildConfiguration::isDebuggable() const noexcept
{
	return m_debugSymbols || m_enableProfiling;
}

/*****************************************************************************/
OptimizationLevel BuildConfiguration::parseOptimizationLevel(const std::string& inValue) noexcept
{
	if (String::equals("debug", inValue))
		return OptimizationLevel::Debug;

	if (String::equals("3", inValue))
		return OptimizationLevel::L3;

	if (String::equals("2", inValue))
		return OptimizationLevel::L2;

	if (String::equals("1", inValue))
		return OptimizationLevel::L1;

	if (String::equals("0", inValue))
		return OptimizationLevel::None;

	if (String::equals("size", inValue))
		return OptimizationLevel::Size;

	if (String::equals("fast", inValue))
		return OptimizationLevel::Fast;

	return OptimizationLevel::CompilerDefault;
}
}
