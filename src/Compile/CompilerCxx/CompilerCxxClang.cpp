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

	auto targetArchString = inState.info.targetArchitectureTriple();

#if defined(CHALET_LINUX)
	// auto gccArchDir = fmt::format("/lib/gcc/{}", targetArchString);
	// auto archDir = fmt::format("/usr/{}/lib", targetArchString);
	// if (!Commands::pathExists(gccArchDir) && Commands::pathExists(archDir))
	// {
	// 	List::addIfDoesNotExist(outArgList, fmt::format("--gcc-toolchain={}", archDir));
	// }
	// auto libcDir = fmt::format("/usr/{}/lib/libc.a", targetArchString);
	// if (Commands::pathExists(libcDir))
	// {
	// 	List::addIfDoesNotExist(outArgList, fmt::format("--sysroot={}", libcDir));
	// }
#endif

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
