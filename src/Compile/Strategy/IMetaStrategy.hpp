/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_IMETA_STRATEGY_HPP
#define CHALET_IMETA_STRATEGY_HPP

#include "BuildJson/ProjectConfiguration.hpp"
#include "Compile/Strategy/StrategyType.hpp"
#include "Compile/Toolchain/ICompileToolchain.hpp"
#include "State/SourceOutputs.hpp"

namespace chalet
{
struct IMetaStrategy
{
	virtual ~IMetaStrategy() = default;

	virtual StrategyType type() const noexcept = 0;

	virtual bool initialize() = 0;
	virtual bool addProject(const ProjectConfiguration& inProject, const SourceOutputs& inOutputs, CompileToolchain& inToolchain) = 0;

	virtual bool saveBuildFile() const = 0;
	virtual bool buildProject(const ProjectConfiguration& inProject) const = 0;
};

using MetaStrategy = std::unique_ptr<IMetaStrategy>;
}

#endif // CHALET_IMETA_STRATEGY_HPP
