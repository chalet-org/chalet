/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CompilerWinResource/ICompilerWinResource.hpp"

#include "Compile/CommandAdapter/CommandAdapterWinResource.hpp"
#include "State/BuildState.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Utility/String.hpp"

#include "Compile/CompilerWinResource/CompilerWinResourceGNUWindRes.hpp"
#include "Compile/CompilerWinResource/CompilerWinResourceLLVMRC.hpp"
#include "Compile/CompilerWinResource/CompilerWinResourceVisualStudioRC.hpp"

namespace chalet
{
/*****************************************************************************/
ICompilerWinResource::ICompilerWinResource(const BuildState& inState, const SourceTarget& inProject) :
	IToolchainExecutableBase(inState, inProject)
{
}

/*****************************************************************************/
[[nodiscard]] Unique<ICompilerWinResource> ICompilerWinResource::make(const ToolchainType inType, const std::string& inExecutable, const BuildState& inState, const SourceTarget& inProject)
{
	const auto exec = String::toLowerCase(String::getPathBaseName(inExecutable));
	// LOG("ICompilerWinResource:", static_cast<i32>(inType), exec);

	auto compilerMatches = [&exec](const char* id, const bool typeMatches, const char* label, const i32 opts = MatchOpts::FailTypeMismatch | MatchOpts::OnlyType) -> i32 {
		return executableMatches(exec, "Windows resource compiler", id, typeMatches, label, opts);
	};

#if defined(CHALET_WIN32)
	if (i32 result = compilerMatches("rc", inType == ToolchainType::VisualStudio || inType == ToolchainType::IntelLLVM, "Visual Studio"); result >= 0)
		return makeTool<CompilerWinResourceVisualStudioRC>(result, inState, inProject);
#endif

	if (i32 result = compilerMatches("llvm-rc", inType == ToolchainType::LLVM || inType == ToolchainType::VisualStudioLLVM, "LLVM", 0); result >= 0)
		return makeTool<CompilerWinResourceLLVMRC>(result, inState, inProject);

	// Allow llvm-rc or windres for MinGW LLVM
	if (i32 result = compilerMatches("llvm-rc", inType == ToolchainType::MingwLLVM, "LLVM", 0); result >= 0)
		return makeTool<CompilerWinResourceLLVMRC>(result, inState, inProject);

	if (i32 result = compilerMatches("llvm-rc", inType == ToolchainType::IntelLLVM, "Intel LLVM", MatchOpts::OnlyType); result >= 0)
		return makeTool<CompilerWinResourceLLVMRC>(result, inState, inProject);

	if (String::equals("llvm-rc", exec))
	{
		Diagnostic::error("Found 'llvm-rc' in a toolchain other than LLVM");
		return nullptr;
	}

	return std::make_unique<CompilerWinResourceGNUWindRes>(inState, inProject);
}

/*****************************************************************************/
bool ICompilerWinResource::initialize()
{
	CommandAdapterWinResource adapter(m_state, m_project);
	if (!adapter.createWindowsApplicationManifest())
		return false;

	if (!adapter.createWindowsApplicationIcon())
		return false;

	return true;
}

/*****************************************************************************/
void ICompilerWinResource::addIncludes(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ICompilerWinResource::addDefines(StringList& outArgList) const
{
	UNUSED(outArgList);
}

}
