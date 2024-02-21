/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/ModuleStrategy/IModuleStrategy.hpp"

namespace chalet
{
struct ModuleStrategyMSVC final : public IModuleStrategy
{
	explicit ModuleStrategyMSVC(BuildState& inState, CompileCommandsGenerator& inCompileCommandsGenerator);

	virtual bool initialize() final;

protected:
	virtual bool scanSourcesForModuleDependencies(CommandPool::Job& outJob, CompileToolchainController& inToolchain, const SourceFileGroupList& inGroups) final;
	virtual bool scanHeaderUnitsForModuleDependencies(CommandPool::Job& outJob, CompileToolchainController& inToolchain, Dictionary<ModulePayload>& outPayload, const SourceFileGroupList& inGroups) final;
	virtual bool readModuleDependencies(const SourceOutputs& inOutputs, Dictionary<ModuleLookup>& outModules) final;
	virtual bool readIncludesFromDependencyFile(const std::string& inFile, StringList& outList) final;

private:
	Dictionary<std::string> getSystemModules() const;

	std::string m_systemModuleDirectory;
};
}
