/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/BuildConfiguration.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "Utility/Path.hpp"
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
		"RelHighOpt",
		"Profile",

		// Sanitizers
		"DebugSanitize",
		"DebugSanitizeAddress",
		"DebugSanitizeThread",
		"DebugSanitizeMemory",
		"DebugSanitizeLeak",
		"DebugSanitizeUndefined",
		"DebugSanitizeHW",
		"DebugSanitizeHWAddress",
		"ReleaseSanitizeUndefined"
	};
}

/*****************************************************************************/
std::string BuildConfiguration::getDefaultReleaseConfigurationName()
{
	return std::string("Release");
}

/*****************************************************************************/
bool BuildConfiguration::makeDefaultConfiguration(BuildConfiguration& outConfig, const std::string& inName)
{
	outConfig = BuildConfiguration();
	outConfig.setName(inName);

	auto makeDebug = [](BuildConfiguration& config) {
		config.setOptimizationLevel("0");
		config.setDebugSymbols(true);
		config.setInterproceduralOptimization(false);
		config.setEnableProfiling(false);
	};

	auto makeRelease = [](BuildConfiguration& config) {
		config.setOptimizationLevel("2");
		config.setDebugSymbols(false);
		config.setInterproceduralOptimization(false);
		config.setEnableProfiling(false);
	};

	if (String::equals("Release", inName))
	{
		makeRelease(outConfig);
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
		outConfig.setInterproceduralOptimization(false);
		outConfig.setEnableProfiling(false);
	}
	else if (String::equals("MinSizeRel", inName))
	{
		outConfig.setOptimizationLevel("size");
		outConfig.setDebugSymbols(false);
		outConfig.setInterproceduralOptimization(false);
		outConfig.setEnableProfiling(false);
	}
	else if (String::equals("RelHighOpt", inName))
	{
		outConfig.setOptimizationLevel("3");
		outConfig.setDebugSymbols(false);
		outConfig.setInterproceduralOptimization(true);
		outConfig.setEnableProfiling(false);
	}
	else if (String::equals("Profile", inName))
	{
		outConfig.setOptimizationLevel("0");
		outConfig.setDebugSymbols(true);
		outConfig.setInterproceduralOptimization(false);
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
	else if (String::equals("ReleaseSanitizeUndefined", inName))
	{
		makeRelease(outConfig);
		outConfig.addSanitizeOption("undefined");
	}
	else if (String::equals("DebugSanitizeHW", inName))
	{
		makeDebug(outConfig);
		outConfig.addSanitizeOptions({
			"hwaddress",
			"undefined",
			"leak",
		});
	}
	else if (String::equals("DebugSanitizeHWAddress", inName))
	{
		makeDebug(outConfig);
		outConfig.addSanitizeOption("hwaddress");
	}
	else
	{
		auto names = String::join(getDefaultBuildConfigurationNames(), ", ");
		Diagnostic::error("An invalid build configuration ({}) was requested. Expected: {}", inName, names);
		return false;
	}

	return true;
}

/*****************************************************************************/
bool BuildConfiguration::validate(const BuildState& inState)
{
	bool result = true;

	if (sanitizeAddress() && sanitizeHardwareAddress())
	{
		Diagnostic::error("Sanitizer 'address' cannot be combined with 'hwaddress'");
		result = false;
	}

	bool asan = sanitizeAddress() || sanitizeHardwareAddress();

	if (inState.environment->isClang() && (asan && sanitizeLeaks()))
	{
		// In Clang, LeakSanitizer is integrated into AddressSanitizer
		m_sanitizeOptions &= ~SanitizeOptions::Leak;
	}

	if (sanitizeThread() && (asan || sanitizeLeaks()))
	{
		Diagnostic::error("Sanitizer 'thread' cannot be combined with 'address', 'hwaddress' or 'leak'");
		result = false;
	}

	if (enableSanitizers())
	{
		if (sanitizeHardwareAddress() && inState.info.targetArchitecture() != Arch::Cpu::ARM64)
		{
			Diagnostic::error("The 'hwaddress' sanitizer is only supported with 'arm64' targets.");
			result = false;
		}

		if (inState.environment->isMsvc())
		{
			if (!sanitizeAddress())
			{
				Diagnostic::error("Only the 'address' sanitizer is supported on MSVC.");
				result = false;
			}

			u32 versionMajorMinor = inState.toolchain.compilerCxxAny().versionMajorMinor;
			if (versionMajorMinor < 1928)
			{
				Diagnostic::error("The 'address' sanitizer is only supported in MSVC >= 19.28 (found {})", inState.toolchain.compilerCxxAny().version);
				result = false;
			}
		}
		else if (inState.environment->isMsvcClang())
		{
			if (!sanitizeAddress() && !sanitizeUndefinedBehavior())
			{
				Diagnostic::error("Only the 'address' and 'undefined' sanitizers are supported on Windows clang.");
				result = false;
			}
		}
		else if (inState.environment->isAppleClang())
		{
			if (sanitizeHardwareAddress())
			{
				Diagnostic::error("The 'hwaddress' sanitizer is not yet supported on Apple clang.");
				result = false;
			}
			if (sanitizeMemory())
			{
				Diagnostic::error("The 'memory' sanitizer is not supported on Apple clang.");
				result = false;
			}
			if (sanitizeLeaks())
			{
				Diagnostic::error("The 'leak' sanitizer is not supported on Apple clang.");
				result = false;
			}
		}
		else if (inState.environment->isMingw())
		{
			Diagnostic::error("Sanitizers are not yet supported in MinGW.");
			result = false;
		}
		else if (inState.environment->isIntelClassic())
		{
			Diagnostic::error("Sanitizers are not supported on Intel Compiler Classic.");
			result = false;
		}
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

std::string BuildConfiguration::optimizationLevelString() const
{
	return getOptimizationLevelString(m_optimizationLevel);
}

void BuildConfiguration::setOptimizationLevel(const std::string& inValue)
{
	m_optimizationLevel = parseOptimizationLevel(inValue);
}

/*****************************************************************************/
bool BuildConfiguration::interproceduralOptimization() const noexcept
{
	return m_interproceduralOptimization;
}

void BuildConfiguration::setInterproceduralOptimization(const bool inValue) noexcept
{
	m_interproceduralOptimization = inValue;
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
	else if (String::equals("hwaddress", inValue))
	{
		m_sanitizeOptions |= SanitizeOptions::HardwareAddress;
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
		m_sanitizeOptions |= SanitizeOptions::UndefinedBehavior;
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
bool BuildConfiguration::sanitizeHardwareAddress() const noexcept
{
	return (m_sanitizeOptions & SanitizeOptions::HardwareAddress) == SanitizeOptions::HardwareAddress;
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
bool BuildConfiguration::sanitizeUndefinedBehavior() const noexcept
{
	return (m_sanitizeOptions & SanitizeOptions::UndefinedBehavior) == SanitizeOptions::UndefinedBehavior;
}

StringList BuildConfiguration::getSanitizerList() const
{
	StringList ret;

	if (enableSanitizers())
	{
		if (sanitizeAddress())
			ret.emplace_back("address");

		if (sanitizeHardwareAddress())
			ret.emplace_back("hwaddress");

		if (sanitizeThread())
			ret.emplace_back("thread");

		if (sanitizeMemory())
			ret.emplace_back("memory");

		if (sanitizeLeaks())
			ret.emplace_back("leak");

		if (sanitizeUndefinedBehavior())
			ret.emplace_back("undefined");
	}

	return ret;
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
OptimizationLevel BuildConfiguration::parseOptimizationLevel(const std::string& inValue) const
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

/*****************************************************************************/
std::string BuildConfiguration::getOptimizationLevelString(const OptimizationLevel inValue) const
{
	switch (inValue)
	{
		case OptimizationLevel::Debug:
			return "debug";
		case OptimizationLevel::L3:
			return "3";
		case OptimizationLevel::L2:
			return "2";
		case OptimizationLevel::L1:
			return "1";
		case OptimizationLevel::Size:
			return "size";
		case OptimizationLevel::Fast:
			return "fast";
		case OptimizationLevel::None:
		case OptimizationLevel::CompilerDefault:
		default:
			break;
	}

	return "0";
}
}
