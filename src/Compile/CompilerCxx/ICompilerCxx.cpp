/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CompilerCxx/ICompilerCxx.hpp"

#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "Utility/String.hpp"

#include "Compile/CompilerCxx/CompilerCxxAppleClang.hpp"
#include "Compile/CompilerCxx/CompilerCxxClang.hpp"
#include "Compile/CompilerCxx/CompilerCxxGCC.hpp"
#include "Compile/CompilerCxx/CompilerCxxIntelClang.hpp"
#include "Compile/CompilerCxx/CompilerCxxIntelClassicCL.hpp"
#include "Compile/CompilerCxx/CompilerCxxIntelClassicGCC.hpp"
#include "Compile/CompilerCxx/CompilerCxxVisualStudioCL.hpp"

namespace chalet
{
/*****************************************************************************/
ICompilerCxx::ICompilerCxx(const BuildState& inState, const SourceTarget& inProject) :
	IToolchainExecutableBase(inState, inProject)
{
}

/*****************************************************************************/
[[nodiscard]] Unique<ICompilerCxx> ICompilerCxx::make(const ToolchainType inType, const std::string& inExecutable, const BuildState& inState, const SourceTarget& inProject)
{
	const auto executable = String::toLowerCase(String::getPathFolderBaseName(String::getPathFilename(inExecutable)));
	LOG("ICompilerCxx:", static_cast<int>(inType), executable);
	const bool clang = String::equals({ "clang", "clang++" }, executable);

#if defined(CHALET_WIN32)
	if (String::equals("cl", executable))
		return std::make_unique<CompilerCxxVisualStudioCL>(inState, inProject);
	else if (inType == ToolchainType::IntelClassic && String::equals("icl", executable))
		return std::make_unique<CompilerCxxIntelClassicCL>(inState, inProject);
	else
#elif defined(CHALET_MACOS)
	if (clang && inType == ToolchainType::AppleLLVM)
		return std::make_unique<CompilerCxxAppleClang>(inState, inProject);
	else if (inType == ToolchainType::IntelClassic && String::equals({ "icc", "icpc" }, executable))
		return std::make_unique<CompilerCxxIntelClassicGCC>(inState, inProject);
	else
#endif
		if (clang && inType == ToolchainType::IntelLLVM)
		return std::make_unique<CompilerCxxIntelClang>(inState, inProject);

	if (clang || inType == ToolchainType::LLVM)
		return std::make_unique<CompilerCxxClang>(inState, inProject);

	return std::make_unique<CompilerCxxGCC>(inState, inProject);
}

/*****************************************************************************/
void ICompilerCxx::addIncludes(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ICompilerCxx::addWarnings(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ICompilerCxx::addDefines(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ICompilerCxx::addPchInclude(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ICompilerCxx::addOptimizationOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ICompilerCxx::addLanguageStandard(StringList& outArgList, const CxxSpecialization specialization) const
{
	UNUSED(outArgList, specialization);
}

/*****************************************************************************/
void ICompilerCxx::addDebuggingInformationOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ICompilerCxx::addProfileInformationCompileOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ICompilerCxx::addCompileOptions(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ICompilerCxx::addDiagnosticColorOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ICompilerCxx::addLibStdCppCompileOption(StringList& outArgList, const CxxSpecialization specialization) const
{
	UNUSED(outArgList, specialization);
}

/*****************************************************************************/
void ICompilerCxx::addPositionIndependentCodeOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ICompilerCxx::addNoRunTimeTypeInformationOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ICompilerCxx::addNoExceptionsOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ICompilerCxx::addThreadModelCompileOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
bool ICompilerCxx::addArchitecture(StringList& outArgList, const std::string& inArch) const
{
	UNUSED(outArgList, inArch);

	return true;
}

}