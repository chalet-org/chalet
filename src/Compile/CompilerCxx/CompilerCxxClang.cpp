/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CompilerCxx/CompilerCxxClang.hpp"

#include "Compile/Environment/ICompileEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildState.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/List.hpp"
#include "Utility/RegexPatterns.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CompilerCxxClang::CompilerCxxClang(const BuildState& inState, const SourceTarget& inProject) :
	CompilerCxxGCC(inState, inProject)
{
}

/*****************************************************************************/
std::string CompilerCxxClang::getPragmaId() const
{
	return std::string("clang");
}

/*****************************************************************************/
StringList CompilerCxxClang::getWarningExclusions() const
{
	return {
		"noexcept",
		"strict-null-sentinel"
	};
}

/*****************************************************************************/
void CompilerCxxClang::addWarnings(StringList& outArgList) const
{
	CompilerCxxGCC::addWarnings(outArgList);
}

/*****************************************************************************/
void CompilerCxxClang::addProfileInformation(StringList& outArgList) const
{
	CompilerCxxGCC::addProfileInformation(outArgList);
}

/*****************************************************************************/
void CompilerCxxClang::addSanitizerOptions(StringList& outArgList, const BuildState& inState)
{
	StringList sanitizers;
	if (inState.configuration.sanitizeAddress())
	{
		sanitizers.emplace_back("address");
	}
	if (inState.configuration.sanitizeHardwareAddress())
	{
		sanitizers.emplace_back("hwaddress");
	}
	if (inState.configuration.sanitizeThread())
	{
		sanitizers.emplace_back("thread");
	}
	if (inState.configuration.sanitizeMemory())
	{
		sanitizers.emplace_back("memory");
	}
	if (inState.configuration.sanitizeLeaks())
	{
		sanitizers.emplace_back("leak");
	}
	if (inState.configuration.sanitizeUndefinedBehavior())
	{
		sanitizers.emplace_back("undefined");
		sanitizers.emplace_back("integer");
	}

	if (!sanitizers.empty())
	{
		auto list = String::join(sanitizers, ',');
		outArgList.emplace_back(fmt::format("-fsanitize={}", list));
	}
}

/*****************************************************************************/
void CompilerCxxClang::addSanitizerOptions(StringList& outArgList) const
{
	if (m_state.configuration.enableSanitizers())
	{
		CompilerCxxClang::addSanitizerOptions(outArgList, m_state);
	}
}

/*****************************************************************************/
void CompilerCxxClang::addLanguageStandard(StringList& outArgList, const SourceType derivative) const
{
	const CodeLanguage language = m_project.language();
	bool validPchType = derivative == SourceType::CxxPrecompiledHeader && (language == CodeLanguage::C || language == CodeLanguage::ObjectiveC);
	bool useC = validPchType || derivative == SourceType::C || derivative == SourceType::ObjectiveC;

	const auto& langStandard = useC ? m_project.cStandard() : m_project.cppStandard();
	std::string ret = String::toLowerCase(langStandard);

	bool isClang = m_state.environment->isClang();
	if (!useC)
	{
		if (RegexPatterns::matchesGnuCppStandard(ret))
		{
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

			ret = "-std=" + ret;
			outArgList.emplace_back(std::move(ret));
		}
	}
	else
	{
		if (RegexPatterns::matchesGnuCStandard(ret))
		{
			std::string yearOnly = ret;
			String::replaceAll(yearOnly, "gnu", "");
			String::replaceAll(yearOnly, "c", "");

			// TODO: determine correct revision where 23 can be used
			if (String::equals("23", yearOnly) && (isClang && m_versionMajorMinor < 1600))
			{
				String::replaceAll(ret, "23", "2x");
			}

			ret = "-std=" + ret;
			outArgList.emplace_back(std::move(ret));
		}
	}
}

/*****************************************************************************/
void CompilerCxxClang::addDiagnosticColorOption(StringList& outArgList) const
{
	std::string diagnosticColor{ "-fcolor-diagnostics" };
	if (isFlagSupported(diagnosticColor))
		List::addIfDoesNotExist(outArgList, std::move(diagnosticColor));
}

/*****************************************************************************/
void CompilerCxxClang::addLibStdCppCompileOption(StringList& outArgList, const SourceType derivative) const
{
	UNUSED(outArgList, derivative);
}

/*****************************************************************************/
void CompilerCxxClang::addPositionIndependentCodeOption(StringList& outArgList) const
{
	CompilerCxxGCC::addPositionIndependentCodeOption(outArgList);
}

/*****************************************************************************/
void CompilerCxxClang::addThreadModelCompileOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
bool CompilerCxxClang::addArchitecture(StringList& outArgList, const std::string& inArch) const
{
	return CompilerCxxClang::addArchitectureToCommand(outArgList, inArch, m_state);
}

/*****************************************************************************/
bool CompilerCxxClang::addArchitectureToCommand(StringList& outArgList, const std::string& inArch, const BuildState& inState)
{
	// https://clang.llvm.org/docs/CrossCompilation.html

	// clang -print-supported-cpus

	UNUSED(inArch);

	const auto& targetArchString = inState.info.targetArchitectureTriple();

	outArgList.emplace_back("-target");
	outArgList.push_back(targetArchString);

	const auto& archOptions = inState.inputs.archOptions();
	if (archOptions.size() == 3) // <cpu-name>,<fpu-name>,<fabi>
	{
		outArgList.emplace_back(fmt::format("-mcpu={}", archOptions[0]));
		outArgList.emplace_back(fmt::format("-mfpu={}", archOptions[1]));
		outArgList.emplace_back(fmt::format("-mfloat-abi={}", archOptions[2]));
	}

	return true;
}

/*****************************************************************************/
void CompilerCxxClang::addLinkTimeOptimizations(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void CompilerCxxClang::addCppCoroutines(StringList& outArgList) const
{
	if (m_project.cppCoroutines() && m_versionMajorMinor >= 500)
	{
		std::string option{ "-fcoroutines-ts" };
		// if (isFlagSupported(option))
		List::addIfDoesNotExist(outArgList, std::move(option));
	}
}

/*****************************************************************************/
void CompilerCxxClang::addCppConcepts(StringList& outArgList) const
{
	if (m_project.cppConcepts() && m_versionMajorMinor >= 600 && m_versionMajorMinor < 1000)
	{
		std::string option{ "-fconcepts-ts" };
		// if (isFlagSupported(option))
		List::addIfDoesNotExist(outArgList, std::move(option));
	}
}

}
