/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_ICOMPILE_STRATEGY_HPP
#define CHALET_ICOMPILE_STRATEGY_HPP

#include "Compile/Strategy/StrategyType.hpp"
#include "State/SourceOutputs.hpp"

namespace chalet
{
struct ICompileStrategy
{
	virtual ~ICompileStrategy() = default;

	virtual StrategyType type() const = 0;

	virtual bool createCache(const SourceOutputs& inOutputs) = 0;
	virtual bool initialize() = 0;
	virtual bool run() = 0;
};

using CompileStrategy = std::unique_ptr<ICompileStrategy>;
}

#endif // CHALET_ICOMPILE_STRATEGY_HPP
