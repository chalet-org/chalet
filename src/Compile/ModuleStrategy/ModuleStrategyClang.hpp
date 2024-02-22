/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/ModuleStrategy/ModuleStrategyGCC.hpp"

namespace chalet
{
struct ModuleStrategyClang final : public ModuleStrategyGCC
{
	explicit ModuleStrategyClang(BuildState& inState, CompileCommandsGenerator& inCompileCommandsGenerator);

	virtual bool initialize() final;

protected:
	virtual bool readIncludesFromDependencyFile(const std::string& inFile, StringList& outList) final;
	virtual bool scanHeaderUnitsForModuleDependencies(CommandPool::Job& outJob) final;

private:
	Dictionary<std::string> getSystemModules() const;
};
}
