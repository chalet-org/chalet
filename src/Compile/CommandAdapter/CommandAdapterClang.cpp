/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CommandAdapter/CommandAdapterClang.hpp"

#include "Cache/WorkspaceCache.hpp"
#include "Compile/Environment/ICompileEnvironment.hpp"
#include "Compile/Linker/ILinker.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Terminal/Commands.hpp"
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
}