/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/ModuleStrategy/ModuleStrategyClang.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Process/Environment.hpp"
#include "Process/Process.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "System/Files.hpp"
#include "Utility/List.hpp"
#include "Utility/Path.hpp"
#include "Utility/String.hpp"
#include "Json/JsonComments.hpp"
#include "Json/JsonKeys.hpp"

namespace chalet
{
/*****************************************************************************/
ModuleStrategyClang::ModuleStrategyClang(BuildState& inState, CompileCommandsGenerator& inCompileCommandsGenerator) :
	ModuleStrategyGCC(inState, inCompileCommandsGenerator)
{
}

/*****************************************************************************/
bool ModuleStrategyClang::initialize()
{
	if (!IModuleStrategy::initialize())
		return false;

	if (m_state.environment->isEmscripten())
	{
		auto& upstreamInclude = m_state.toolchain.compilerCpp().includeDir;
		List::addIfDoesNotExist(m_systemHeaderDirectories, fmt::format("{}/c++/v1", upstreamInclude));
	}

	return true;
}

/*****************************************************************************/
bool ModuleStrategyClang::scanHeaderUnitsForModuleDependencies(CommandPool::Job& outJob)
{
	UNUSED(outJob);

	// We need to call this to update the compiler cache, but we don't want to use the commands
	auto commands = getModuleCommands(m_headerUnitList, m_modulePayload, ModuleFileType::HeaderUnitDependency);
	UNUSED(commands);

	return true;
}

/*****************************************************************************/
bool ModuleStrategyClang::readIncludesFromDependencyFile(const std::string& inFile, StringList& outList)
{
	UNUSED(inFile, outList);
	return true;
}

/*****************************************************************************/
Dictionary<std::string> ModuleStrategyClang::getSystemModules() const
{
	Dictionary<std::string> ret;

	ret.emplace("std", "std");
	ret.emplace("std.compat", "std.compat");

	return ret;
}

}
