/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/ModuleStrategy/IModuleStrategy.hpp"

namespace chalet
{
struct ModuleStrategyGCC : public IModuleStrategy
{
	explicit ModuleStrategyGCC(BuildState& inState, CompileCommandsGenerator& inCompileCommandsGenerator);

	virtual bool initialize() override;

protected:
	virtual bool scanSourcesForModuleDependencies(CommandPool::Job& outJob) override;
	virtual bool readModuleDependencies() override;
	virtual bool readIncludesFromDependencyFile(const std::string& inFile, StringList& outList) override;
	virtual bool scanHeaderUnitsForModuleDependencies(CommandPool::Job& outJob) override;

	virtual Dictionary<std::string> getSystemModules() const;

private:
	Dictionary<std::string> m_moduleMap;
	Dictionary<StringList> m_moduleImports;
	Dictionary<StringList> m_headerUnitImports;
};
}
