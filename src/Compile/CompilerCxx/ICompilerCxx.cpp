/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CompilerCxx/ICompilerCxx.hpp"

#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Utility/String.hpp"

#include "Compile/CompilerCxx/CompilerCxxAppleClang.hpp"
#include "Compile/CompilerCxx/CompilerCxxClang.hpp"
#include "Compile/CompilerCxx/CompilerCxxEmscripten.hpp"
#include "Compile/CompilerCxx/CompilerCxxGCC.hpp"
#include "Compile/CompilerCxx/CompilerCxxIntelClang.hpp"
#include "Compile/CompilerCxx/CompilerCxxIntelClassicCL.hpp"
#include "Compile/CompilerCxx/CompilerCxxIntelClassicGCC.hpp"
#include "Compile/CompilerCxx/CompilerCxxVisualStudioCL.hpp"
#include "Compile/CompilerCxx/CompilerCxxVisualStudioClang.hpp"

namespace chalet
{
/*****************************************************************************/
ICompilerCxx::ICompilerCxx(const BuildState& inState, const SourceTarget& inProject) :
	IToolchainExecutableBase(inState, inProject)
{
	m_versionMajorMinor = m_state.toolchain.compilerCxx(m_project.language()).versionMajorMinor;
	m_versionPatch = m_state.toolchain.compilerCxx(m_project.language()).versionPatch;
}

/*****************************************************************************/
[[nodiscard]] Unique<ICompilerCxx> ICompilerCxx::make(const ToolchainType inType, const std::string& inExecutable, const BuildState& inState, const SourceTarget& inProject)
{
	const auto exec = String::toLowerCase(String::getPathFolderBaseName(String::getPathFilename(inExecutable)));
	// LOG("ICompilerCxx:", static_cast<i32>(inType), executable);

	bool isC = inProject.language() == CodeLanguage::C;

	auto cCompilerMatches = [&exec](const char* id, const bool typeMatches, const char* label, const bool failTypeMismatch = true) -> i32 {
		constexpr bool onlyType = true;
		return executableMatches(exec, "C compiler", id, typeMatches, label, failTypeMismatch, onlyType);
	};

	auto cppCompilerMatches = [&exec](const char* id, const bool typeMatches, const char* label, const bool failTypeMismatch = true) -> i32 {
		constexpr bool onlyType = true;
		return executableMatches(exec, "C++ compiler", id, typeMatches, label, failTypeMismatch, onlyType);
	};

	auto cxxCompilerMatches = [&exec](const char* id, const bool typeMatches, const char* label, const bool failTypeMismatch = true) -> i32 {
		constexpr bool onlyType = true;
		return executableMatches(exec, "C/C++ compiler", id, typeMatches, label, failTypeMismatch, onlyType);
	};

#if defined(CHALET_WIN32)
	if (i32 result = cxxCompilerMatches("cl", inType == ToolchainType::VisualStudio, "Visual Studio"); result >= 0)
		return makeTool<CompilerCxxVisualStudioCL>(result, inState, inProject);

	if (i32 result = cxxCompilerMatches("icl", inType == ToolchainType::IntelClassic, "Intel Classic"); result >= 0)
		return makeTool<CompilerCxxIntelClassicCL>(result, inState, inProject);

	if (isC)
	{
		if (i32 result = cCompilerMatches("clang", inType == ToolchainType::LLVM || inType == ToolchainType::VisualStudioLLVM, "LLVM", false); result >= 0)
			return makeTool<CompilerCxxVisualStudioClang>(result, inState, inProject);

		if (i32 result = cCompilerMatches("clang", inType == ToolchainType::MingwLLVM, "LLVM", false); result >= 0)
			return makeTool<CompilerCxxClang>(result, inState, inProject);
	}
	else
	{
		if (i32 result = cppCompilerMatches("clang++", inType == ToolchainType::LLVM || inType == ToolchainType::VisualStudioLLVM, "LLVM", false); result >= 0)
			return makeTool<CompilerCxxVisualStudioClang>(result, inState, inProject);

		if (i32 result = cppCompilerMatches("clang++", inType == ToolchainType::MingwLLVM, "LLVM", false); result >= 0)
			return makeTool<CompilerCxxClang>(result, inState, inProject);
	}

#elif defined(CHALET_MACOS)
	if (isC)
	{
		if (i32 result = cCompilerMatches("clang", inType == ToolchainType::AppleLLVM, "AppleClang", false); result >= 0)
			return makeTool<CompilerCxxAppleClang>(result, inState, inProject);

		if (i32 result = cCompilerMatches("icc", inType == ToolchainType::IntelClassic, "Intel Classic"); result >= 0)
			return makeTool<CompilerCxxIntelClassicGCC>(result, inState, inProject);
	}
	else
	{
		if (i32 result = cppCompilerMatches("clang++", inType == ToolchainType::AppleLLVM, "AppleClang", false); result >= 0)
			return makeTool<CompilerCxxAppleClang>(result, inState, inProject);

		if (i32 result = cppCompilerMatches("icpc", inType == ToolchainType::IntelClassic, "Intel Classic"); result >= 0)
			return makeTool<CompilerCxxIntelClassicGCC>(result, inState, inProject);
	}

#endif

#if !defined(CHALET_WIN32)
	if (isC)
	{
		if (i32 result = cCompilerMatches("clang", inType == ToolchainType::LLVM, "LLVM", false); result >= 0)
			return makeTool<CompilerCxxClang>(result, inState, inProject);
	}
	else
	{
		if (i32 result = cppCompilerMatches("clang++", inType == ToolchainType::LLVM, "LLVM", false); result >= 0)
			return makeTool<CompilerCxxClang>(result, inState, inProject);
	}

#endif

	if (isC)
	{
		if (i32 result = cCompilerMatches("clang", inType == ToolchainType::IntelLLVM, "Intel LLVM", false); result >= 0)
			return makeTool<CompilerCxxIntelClang>(result, inState, inProject);
	}
	else
	{
		if (i32 result = cppCompilerMatches("clang++", inType == ToolchainType::IntelLLVM, "Intel LLVM", false); result >= 0)
			return makeTool<CompilerCxxIntelClang>(result, inState, inProject);
	}

	if (i32 result = cxxCompilerMatches("emcc", inType == ToolchainType::Emscripten, "Emscripten"); result >= 0)
		return makeTool<CompilerCxxEmscripten>(result, inState, inProject);

	if (String::equals("clang++", exec))
	{
		Diagnostic::error("Found 'clang++' in a toolchain other than LLVM");
		return nullptr;
	}

	if (String::equals("clang", exec))
	{
		Diagnostic::error("Found 'clang' in a toolchain other than LLVM");
		return nullptr;
	}

	return std::make_unique<CompilerCxxGCC>(inState, inProject);
}

/*****************************************************************************/
StringList ICompilerCxx::getModuleCommand(const std::string& inputFile, const std::string& outputFile, const std::string& dependencyFile, const std::string& interfaceFile, const StringList& inModuleReferences, const StringList& inHeaderUnits, const ModuleFileType inType)
{
	UNUSED(inputFile, outputFile, dependencyFile, interfaceFile, inModuleReferences, inHeaderUnits, inType);
	return StringList();
}

/*****************************************************************************/
bool ICompilerCxx::addExecutable(StringList& outArgList) const
{
	auto& executable = m_state.toolchain.compilerCxx(m_project.language()).path;
	if (executable.empty())
		return false;

	outArgList.emplace_back(getQuotedPath(executable));
	return true;
}

/*****************************************************************************/
bool ICompilerCxx::precompiledHeaderAllowedForSourceType(const SourceType derivative) const
{
	if (!m_project.usesPrecompiledHeader())
		return false;

	auto language = m_project.language();
	bool validFile = (language == CodeLanguage::ObjectiveCPlusPlus && derivative != SourceType::ObjectiveC)
		|| (language == CodeLanguage::ObjectiveC && derivative != SourceType::ObjectiveCPlusPlus)
		|| (language == CodeLanguage::C && derivative != SourceType::CPlusPlus)
		|| (language == CodeLanguage::CPlusPlus && derivative != SourceType::C);

	return validFile;
}

/*****************************************************************************/
void ICompilerCxx::addSourceFileInterpretation(StringList& outArgList, const SourceType derivative) const
{
	UNUSED(outArgList, derivative);
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
void ICompilerCxx::addPchInclude(StringList& outArgList, const SourceType derivative) const
{
	UNUSED(outArgList, derivative);
}

/*****************************************************************************/
void ICompilerCxx::addOptimizations(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ICompilerCxx::addLanguageStandard(StringList& outArgList, const SourceType derivative) const
{
	UNUSED(outArgList, derivative);
}

/*****************************************************************************/
void ICompilerCxx::addDebuggingInformationOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ICompilerCxx::addProfileInformation(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ICompilerCxx::addSanitizerOptions(StringList& outArgList) const
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
void ICompilerCxx::addCharsets(StringList& outArgList) const
{
	UNUSED(outArgList);
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
void ICompilerCxx::addFastMathOption(StringList& outArgList) const
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
