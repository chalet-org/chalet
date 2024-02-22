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
	virtual bool scanSourcesForModuleDependencies(CommandPool::Job& outJob) final;
	virtual bool scanHeaderUnitsForModuleDependencies(CommandPool::Job& outJob) final;
	virtual bool readModuleDependencies() final;
	virtual bool readIncludesFromDependencyFile(const std::string& inFile, StringList& outList) final;

private:
	Dictionary<std::string> getSystemModules() const;

	std::string m_systemModuleDirectory;
};
}
