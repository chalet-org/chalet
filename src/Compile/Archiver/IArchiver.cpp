/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Archiver/IArchiver.hpp"

#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Utility/String.hpp"

#include "Compile/Archiver/ArchiverGNUAR.hpp"
#include "Compile/Archiver/ArchiverIntelClassicAR.hpp"
#include "Compile/Archiver/ArchiverIntelClassicLIB.hpp"
#include "Compile/Archiver/ArchiverLLVMAR.hpp"
#include "Compile/Archiver/ArchiverLibTool.hpp"
#include "Compile/Archiver/ArchiverVisualStudioLIB.hpp"

namespace chalet
{
/*****************************************************************************/
IArchiver::IArchiver(const BuildState& inState, const SourceTarget& inProject) :
	IToolchainExecutableBase(inState, inProject)
{
}

/*****************************************************************************/
[[nodiscard]] Unique<IArchiver> IArchiver::make(const ToolchainType inType, const std::string& inExecutable, const BuildState& inState, const SourceTarget& inProject)
{
	const auto exec = String::toLowerCase(String::getPathBaseName(inExecutable));
	// LOG("IArchiver:", static_cast<i32>(inType), exec);

	auto archiverMatches = [&exec](const char* id, const bool typeMatches, const char* label, const bool failTypeMismatch = true) -> i32 {
		constexpr bool onlyType = true;
		return executableMatches(exec, "archiver", id, typeMatches, label, failTypeMismatch, onlyType);
	};

#if defined(CHALET_WIN32)
	if (i32 result = archiverMatches("lib", inType == ToolchainType::VisualStudio, "Visual Studio"); result >= 0)
		return makeTool<ArchiverVisualStudioLIB>(result, inState, inProject);

	if (i32 result = archiverMatches("xilib", inType == ToolchainType::IntelClassic, "Intel Classic"); result >= 0)
		return makeTool<ArchiverIntelClassicLIB>(result, inState, inProject);

#elif defined(CHALET_MACOS)
	if (i32 result = archiverMatches("libtool", inType == ToolchainType::AppleLLVM, "Apple"); result >= 0)
		return makeTool<ArchiverLibTool>(result, inState, inProject);

	if (i32 result = archiverMatches("xiar", inType == ToolchainType::IntelClassic, "Intel Classic"); result >= 0)
		return makeTool<ArchiverIntelClassicAR>(result, inState, inProject);

#endif

	if (i32 result = archiverMatches("llvm-ar", inType == ToolchainType::VisualStudioLLVM || inType == ToolchainType::LLVM || inType == ToolchainType::MingwLLVM, "LLVM", false); result >= 0)
		return makeTool<ArchiverLLVMAR>(result, inState, inProject);

	if (i32 result = archiverMatches("llvm-ar", inType == ToolchainType::IntelLLVM, "Intel LLVM", false); result >= 0)
		return makeTool<ArchiverLLVMAR>(result, inState, inProject);

	if (i32 result = archiverMatches("llvm-ar", inType == ToolchainType::Emscripten, "Emscripten", false); result >= 0)
		return makeTool<ArchiverLLVMAR>(result, inState, inProject);

	if (String::equals("llvm-ar", exec))
	{
		Diagnostic::error("Found 'llvm-ar' in a toolchain other than LLVM");
		return nullptr;
	}

	return std::make_unique<ArchiverGNUAR>(inState, inProject);
}

/*****************************************************************************/
bool IArchiver::initialize()
{
	return true;
}

/*****************************************************************************/
void IArchiver::addSourceObjects(StringList& outArgList, const StringList& sourceObjs) const
{
	auto strategy = m_state.toolchain.strategy();
	if (strategy == StrategyType::Ninja || strategy == StrategyType::Makefile)
	{
		for (auto& source : sourceObjs)
		{
			outArgList.emplace_back(source);
		}
	}
	else
	{
		for (auto& source : sourceObjs)
		{
			outArgList.emplace_back(getQuotedPath(source));
		}
	}
}
}
