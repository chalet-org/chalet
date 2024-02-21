/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/ModuleStrategy/ModuleStrategyClang.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Process/Environment.hpp"
#include "Process/Process.hpp"
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
	return IModuleStrategy::initialize();
}

/*****************************************************************************/
bool ModuleStrategyClang::scanSourcesForModuleDependencies(CommandPool::Job& outJob, CompileToolchainController& inToolchain, const SourceFileGroupList& inGroups)
{
	return ModuleStrategyGCC::scanSourcesForModuleDependencies(outJob, inToolchain, inGroups);
}

/*****************************************************************************/
bool ModuleStrategyClang::scanHeaderUnitsForModuleDependencies(CommandPool::Job& outJob, CompileToolchainController& inToolchain, Dictionary<ModulePayload>& outPayload, const SourceFileGroupList& inGroups)
{
	return ModuleStrategyGCC::scanHeaderUnitsForModuleDependencies(outJob, inToolchain, outPayload, inGroups);
}

/*****************************************************************************/
bool ModuleStrategyClang::readModuleDependencies(const SourceOutputs& inOutputs, Dictionary<ModuleLookup>& outModules)
{
	return ModuleStrategyGCC::readModuleDependencies(inOutputs, outModules);
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
