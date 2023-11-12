/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CommandAdapter/CommandAdapterClang.hpp"

#include "Cache/WorkspaceCache.hpp"
#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Compile/Linker/ILinker.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "System/Files.hpp"
#include "Utility/List.hpp"
#include "Utility/RegexPatterns.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CommandAdapterClang::CommandAdapterClang(const BuildState& inState, const SourceTarget& inProject) :
	m_state(inState),
	m_project(inProject)
{
	m_versionMajorMinor = m_state.toolchain.compilerCxx(m_project.language()).versionMajorMinor;
	m_versionPatch = m_state.toolchain.compilerCxx(m_project.language()).versionPatch;
}

/*****************************************************************************/
std::string CommandAdapterClang::getLanguageStandardCpp() const
{
	auto ret = String::toLowerCase(m_project.cppStandard());
	if (RegexPatterns::matchesGnuCppStandard(ret))
	{
		bool isClang = m_state.environment->isClang();
		std::string yearOnly = ret;
		String::replaceAll(yearOnly, "gnu++", "");
		String::replaceAll(yearOnly, "c++", "");

		if (String::equals("26", yearOnly) && (isClang /* && m_versionMajorMinor < 1700 */))
		{
			String::replaceAll(ret, "26", "2c");
		}
		else if (String::equals("23", yearOnly) && (isClang && m_versionMajorMinor < 1700))
		{
			String::replaceAll(ret, "23", "2b");
		}
		else if (String::equals("20", yearOnly) && (isClang && m_versionMajorMinor < 1000))
		{
			String::replaceAll(ret, "20", "2a");
		}
		else if (String::equals("17", yearOnly) && (isClang && m_versionMajorMinor < 500))
		{
			String::replaceAll(ret, "17", "1z");
		}
		else if (String::equals("14", yearOnly) && (isClang && m_versionMajorMinor < 350))
		{
			String::replaceAll(ret, "14", "1y");
		}

		return ret;
	}

	return std::string();
}

/*****************************************************************************/
std::string CommandAdapterClang::getLanguageStandardC() const
{
	auto ret = String::toLowerCase(m_project.cStandard());
	if (RegexPatterns::matchesGnuCStandard(ret))
	{
		bool isClang = m_state.environment->isClang();
		std::string yearOnly = ret;
		String::replaceAll(yearOnly, "gnu", "");
		String::replaceAll(yearOnly, "c", "");

		// TODO: determine correct revision where 23 can be used
		if (String::equals("23", yearOnly) && (isClang && m_versionMajorMinor < 1600))
		{
			String::replaceAll(ret, "23", "2x");
		}

		return ret;
	}

	return std::string();
}

/*****************************************************************************/
std::string CommandAdapterClang::getCxxLibrary() const
{
	return "libc++";
}

/*****************************************************************************/
std::string CommandAdapterClang::getOptimizationLevel() const
{
	OptimizationLevel level = m_state.configuration.optimizationLevel();
	if (m_state.configuration.debugSymbols()
		&& level != OptimizationLevel::Debug
		&& level != OptimizationLevel::None
		&& level != OptimizationLevel::CompilerDefault)
	{
		// force -O0 (anything else would be in error)
		return "0";
	}
	else
	{
		switch (level)
		{
			case OptimizationLevel::L1:
				return "1";

			case OptimizationLevel::L2:
				return "2";

			case OptimizationLevel::L3:
				return "3";

			case OptimizationLevel::Debug:
				return "g";

			case OptimizationLevel::Size:
				return "s";

			case OptimizationLevel::Fast:
				return "fast";

			case OptimizationLevel::None:
				return "0";

			case OptimizationLevel::CompilerDefault:
			default:
				break;
		}
	}
	return std::string();
}

/*****************************************************************************/
StringList CommandAdapterClang::getWarningList() const
{
	StringList ret;

	auto getWithUserWarnings = [this](StringList&& warnings) {
		auto& userWarnings = m_project.warnings();
		auto exclusions = getWarningExclusions();
		for (auto& warning : userWarnings)
		{
			if (List::contains(exclusions, warning))
				continue;

			List::addIfDoesNotExist(warnings, warning);
		}

		if (m_project.usesPrecompiledHeader())
			List::addIfDoesNotExist(warnings, "invalid-pch");

		if (m_project.treatWarningsAsErrors())
			List::addIfDoesNotExist(warnings, "error");

		return std::move(warnings);
	};

	auto warningsPreset = m_project.warningsPreset();
	if (warningsPreset == ProjectWarningPresets::None)
		return getWithUserWarnings(std::move(ret));

	ret.emplace_back("all");

	if (warningsPreset == ProjectWarningPresets::Minimal)
		return getWithUserWarnings(std::move(ret));

	ret.emplace_back("extra");

	if (warningsPreset == ProjectWarningPresets::Extra)
		return getWithUserWarnings(std::move(ret));

	ret.emplace_back("pedantic");
	// ret.emplace_back("pedantic-errors");

	if (warningsPreset == ProjectWarningPresets::Pedantic)
		return getWithUserWarnings(std::move(ret));

	ret.emplace_back("unused");
	ret.emplace_back("cast-align");
	ret.emplace_back("double-promotion");
	ret.emplace_back("format=2");
	ret.emplace_back("missing-declarations");
	ret.emplace_back("missing-include-dirs");
	ret.emplace_back("non-virtual-dtor");
	ret.emplace_back("redundant-decls");

	if (warningsPreset == ProjectWarningPresets::Strict)
		return getWithUserWarnings(std::move(ret));

	ret.emplace_back("unreachable-code"); // clang only
	ret.emplace_back("shadow");

	if (warningsPreset == ProjectWarningPresets::StrictPedantic)
		return getWithUserWarnings(std::move(ret));

	ret.emplace_back("noexcept");
	ret.emplace_back("undef");
	ret.emplace_back("conversion");
	ret.emplace_back("cast-qual");
	ret.emplace_back("float-equal");
	ret.emplace_back("inline");
	ret.emplace_back("old-style-cast");
	ret.emplace_back("strict-null-sentinel");
	ret.emplace_back("overloaded-virtual");
	ret.emplace_back("sign-conversion");
	ret.emplace_back("sign-promo");

	// if (warningsPreset == ProjectWarningPresets::VeryStrict)
	// 	return getWithUserWarnings(std::move(ret));

	return getWithUserWarnings(std::move(ret));
}

/*****************************************************************************/
StringList CommandAdapterClang::getWarningExclusions() const
{
	return {};
}

/*****************************************************************************/
StringList CommandAdapterClang::getSanitizersList() const
{
	StringList ret;
	if (m_state.configuration.sanitizeAddress())
	{
		ret.emplace_back("address");
	}
	if (m_state.configuration.sanitizeHardwareAddress())
	{
		ret.emplace_back("hwaddress");
	}
	if (m_state.configuration.sanitizeThread())
	{
		ret.emplace_back("thread");
	}
	if (m_state.configuration.sanitizeMemory())
	{
		ret.emplace_back("memory");
	}
	if (m_state.configuration.sanitizeLeaks())
	{
		ret.emplace_back("leak");
	}
	if (m_state.configuration.sanitizeUndefinedBehavior())
	{
		ret.emplace_back("undefined");
		ret.emplace_back("integer");
	}

	return ret;
}

/*****************************************************************************/
bool CommandAdapterClang::supportsCppCoroutines() const
{
	return m_project.cppCoroutines() && m_versionMajorMinor >= 500;
}

/*****************************************************************************/
bool CommandAdapterClang::supportsCppConcepts() const
{
	return m_project.cppConcepts() && m_versionMajorMinor >= 600 && m_versionMajorMinor < 1000;
}

/*****************************************************************************/
bool CommandAdapterClang::supportsFastMath() const
{
	return m_project.fastMath();
}

/*****************************************************************************/
bool CommandAdapterClang::supportsExceptions() const
{
	return m_project.exceptions();
}
/*****************************************************************************/
bool CommandAdapterClang::supportsRunTimeTypeInformation() const
{
	return m_project.runtimeTypeInformation();
}
}