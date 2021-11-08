/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CompilerCxx/CompilerCxxClang.hpp"

#include "Compile/Environment/ICompileEnvironment.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildState.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Utility/List.hpp"

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

	if (m_state.environment->isWindowsClang())
	{
		const std::string prefix{ "-W" };
		std::string noLangExtensions{ "no-language-extension-token" };
		if (!List::contains(m_project.warnings(), noLangExtensions))
			outArgList.emplace_back(prefix + noLangExtensions);
	}
}

/*****************************************************************************/
void CompilerCxxClang::addProfileInformationCompileOption(StringList& outArgList) const
{
	// TODO: "-pg" was added in a recent version of Clang (12 or 13 maybe?)
	UNUSED(outArgList);
}

/*****************************************************************************/
void CompilerCxxClang::addDiagnosticColorOption(StringList& outArgList) const
{
	std::string diagnosticColor{ "-fcolor-diagnostics" };
	if (isFlagSupported(diagnosticColor))
		List::addIfDoesNotExist(outArgList, std::move(diagnosticColor));
}

/*****************************************************************************/
void CompilerCxxClang::addLibStdCppCompileOption(StringList& outArgList, const CxxSpecialization specialization) const
{
	UNUSED(outArgList, specialization);
}

/*****************************************************************************/
void CompilerCxxClang::addPositionIndependentCodeOption(StringList& outArgList) const
{
#if defined(CHALET_LINUX)
	CompilerCxxGCC::addPositionIndependentCodeOption(outArgList);
#else
	UNUSED(outArgList);
#endif
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

	const auto& archOptions = inState.info.archOptions();
	if (archOptions.size() == 3) // <cpu-name>,<fpu-name>,<fabi>
	{
		outArgList.emplace_back(fmt::format("-mcpu={}", archOptions[0]));
		outArgList.emplace_back(fmt::format("-mfpu={}", archOptions[1]));
		outArgList.emplace_back(fmt::format("-mfloat-abi={}", archOptions[2]));
	}

	return true;
}
}
