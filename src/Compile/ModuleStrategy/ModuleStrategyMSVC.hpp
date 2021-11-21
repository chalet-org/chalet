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
};
}

#endif // CHALET_MODULE_STRATEGY_MSVC_HPP
