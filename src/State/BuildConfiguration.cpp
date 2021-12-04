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
StringList BuildConfiguration::getDefaultBuildConfigurationNames()
{
	return {
		"Release",
		"Debug",
		"MinSizeRel",
		"RelWithDebInfo",
		"RelStable",
		"Profile",

		// Sanitizers
		"DebugSanitize",
		"DebugSanitizeAddress",
		"DebugSanitizeThread",
		"DebugSanitizeMemory",
		"DebugSanitizeLeak",
		"DebugSanitizeUndefined",
	};
}

/*****************************************************************************/
bool BuildConfiguration::makeDefaultConfiguration(BuildConfiguration& outConfig, const std::string& inName)
{
	outConfig = BuildConfiguration();

	auto makeDebug = [](BuildConfiguration& config) {
		config.setOptimizationLevel("0");
		config.setDebugSymbols(true);
		config.setLinkTimeOptimization(false);
		config.setStripSymbols(false);
		config.setEnableProfiling(false);
	};

	if (String::equals("Release", inName))
	{
		outConfig.setOptimizationLevel("3");
		outConfig.setDebugSymbols(false);
		outConfig.setLinkTimeOptimization(true);
		outConfig.setStripSymbols(true);
		outConfig.setEnableProfiling(false);
	}
	else if (String::equals("Debug", inName))
	{
		makeDebug(outConfig);
	}
	// these two are the same as cmake
	else if (String::equals("RelWithDebInfo", inName))
	{
		outConfig.setOptimizationLevel("2");
		outConfig.setDebugSymbols(true);
		outConfig.setLinkTimeOptimization(false);
		outConfig.setStripSymbols(false);
		outConfig.setEnableProfiling(false);
	}
	else if (String::equals("MinSizeRel", inName))
	{
		outConfig.setOptimizationLevel("size");
		outConfig.setDebugSymbols(false);
		outConfig.setLinkTimeOptimization(false);
		outConfig.setStripSymbols(true);
		outConfig.setEnableProfiling(false);
	}
	else if (String::equals("RelStable", inName))
	{
		outConfig.setOptimizationLevel("2");
		outConfig.setDebugSymbols(false);
		outConfig.setLinkTimeOptimization(false);
		outConfig.setStripSymbols(true);
		outConfig.setEnableProfiling(false);
	}
	else if (String::equals("Profile", inName))
	{
		outConfig.setOptimizationLevel("0");
		outConfig.setDebugSymbols(true);
		outConfig.setLinkTimeOptimization(false);
		outConfig.setStripSymbols(false);
		outConfig.setEnableProfiling(true);
	}
	else if (String::equals("DebugSanitize", inName))
	{
		makeDebug(outConfig);
		outConfig.addSanitizeOptions({
			"address",
			"undefined",
			"leak",
		});
	}
	else if (String::equals("DebugSanitizeAddress", inName))
	{
		makeDebug(outConfig);
		outConfig.addSanitizeOption("address");
	}
	else if (String::equals("DebugSanitizeThread", inName))
	{
		makeDebug(outConfig);
		outConfig.addSanitizeOption("thread");
	}
	else if (String::equals("DebugSanitizeMemory", inName))
	{
		makeDebug(outConfig);
		outConfig.addSanitizeOption("memory");
	}
	else if (String::equals("DebugSanitizeLeak", inName))
	{
		makeDebug(outConfig);
		outConfig.addSanitizeOption("leak");
	}
	else if (String::equals("DebugSanitizeUndefined", inName))
	{
		makeDebug(outConfig);
		outConfig.addSanitizeOption("undefined");
	}
	else
	{
		auto names = String::join(getDefaultBuildConfigurationNames(), ", ");
		Diagnostic::error("An invalid build configuration ({}) was requested. Expected: {}", inName, names);
		return false;
	}

	outConfig.setName(inName);

	return true;
}

/*****************************************************************************/
bool BuildConfiguration::validate(const bool isAppleClang)
{
	bool result = true;

	if (isAppleClang && enableSanitizers() && m_sanitizeOptions != SanitizeOptions::Leak)
	{
		m_sanitizeOptions &= ~SanitizeOptions::Leak;
	}

	if (sanitizeThread() && (sanitizeAddress() || sanitizeLeaks()))
	{
		Diagnostic::error("sanitizer 'thread' cannot be combined with 'address' or 'leak'");
		result = false;
	}

	return result;
}

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
void BuildConfiguration::addSanitizeOptions(StringList&& inList)
{
	for (auto&& item : inList)
	{
		addSanitizeOption(std::move(item));
	}
}

/*****************************************************************************/
void BuildConfiguration::addSanitizeOption(std::string&& inValue)
{
	if (String::equals("address", inValue))
	{
		m_sanitizeOptions |= SanitizeOptions::Address;
	}
	else if (String::equals("thread", inValue))
	{
		m_sanitizeOptions |= SanitizeOptions::Thread;
	}
	else if (String::equals("memory", inValue))
	{
		m_sanitizeOptions |= SanitizeOptions::Memory;
	}
	else if (String::equals("leak", inValue))
	{
		m_sanitizeOptions |= SanitizeOptions::Leak;
	}
	else if (String::equals("undefined", inValue))
	{
		m_sanitizeOptions |= SanitizeOptions::Undefined;
	}
}

/*****************************************************************************/
bool BuildConfiguration::enableSanitizers() const noexcept
{
	return m_sanitizeOptions != SanitizeOptions::None;
}
bool BuildConfiguration::sanitizeAddress() const noexcept
{
	return (m_sanitizeOptions & SanitizeOptions::Address) == SanitizeOptions::Address;
}
bool BuildConfiguration::sanitizeThread() const noexcept
{
	return (m_sanitizeOptions & SanitizeOptions::Thread) == SanitizeOptions::Thread;
}
bool BuildConfiguration::sanitizeMemory() const noexcept
{
	return (m_sanitizeOptions & SanitizeOptions::Memory) == SanitizeOptions::Memory;
}
bool BuildConfiguration::sanitizeLeaks() const noexcept
{
	return (m_sanitizeOptions & SanitizeOptions::Leak) == SanitizeOptions::Leak;
}
bool BuildConfiguration::sanitizeUndefined() const noexcept
{
	return (m_sanitizeOptions & SanitizeOptions::Undefined) == SanitizeOptions::Undefined;
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
