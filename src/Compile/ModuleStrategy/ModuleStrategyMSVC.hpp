/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_MODULE_STRATEGY_MSVC_HPP
#define CHALET_MODULE_STRATEGY_MSVC_HPP

#include "Compile/ModuleStrategy/IModuleStrategy.hpp"

namespace chalet
{
struct ModuleStrategyMSVC final : public IModuleStrategy
{
	explicit ModuleStrategyMSVC(BuildState& inState);

	virtual bool initialize() final;

protected:
	virtual bool isSystemHeader(const std::string& inHeader) const final;
	virtual std::string getBuildOutputForFile(const SourceFileGroup& inFile, const bool inIsObject) final;
	virtual bool readModuleDependencies(const SourceOutputs& inOutputs, Dictionary<ModuleLookup>& outModules) final;

private:
	std::string m_msvcToolsDirectory;
	std::string m_msvcToolsDirectoryLower;
};
}

#endif // CHALET_MODULE_STRATEGY_MSVC_HPP
