/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/ModuleStrategy/IModuleStrategy.hpp"

namespace chalet
{
struct ModuleStrategyGCC final : public IModuleStrategy
{
	explicit ModuleStrategyGCC(BuildState& inState, CompileCommandsGenerator& inCompileCommandsGenerator);

	virtual bool initialize() final;

protected:
	virtual bool isSystemModuleFile(const std::string& inFile) const final;
	virtual std::string getBuildOutputForFile(const SourceFileGroup& inFile, const bool inIsObject) const final;
	virtual bool scanSourcesForModuleDependencies(CommandPool::Job& outJob, CompileToolchainController& inToolchain, const SourceFileGroupList& inGroups) final;
	virtual bool scanHeaderUnitsForModuleDependencies(CommandPool::Job& outJob, CompileToolchainController& inToolchain, Dictionary<ModulePayload>& outPayload, const SourceFileGroupList& inGroups) final;
	virtual bool readModuleDependencies(const SourceOutputs& inOutputs, Dictionary<ModuleLookup>& outModules) final;
	virtual bool readIncludesFromDependencyFile(const std::string& inFile, StringList& outList) final;

private:
	Dictionary<std::string> getSystemModules() const;

	Dictionary<std::string> m_moduleMap;
	Dictionary<StringList> m_moduleImports;
	Dictionary<StringList> m_headerUnitImports;

	StringList m_systemHeaders;
	// StringList m_userHeaders;

	std::string m_systemHeaderDirectory;
};
}
