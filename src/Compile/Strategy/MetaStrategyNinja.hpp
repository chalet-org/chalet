/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_META_STRATEGY_NINJA_HPP
#define CHALET_META_STRATEGY_NINJA_HPP

#include "State/BuildState.hpp"

namespace chalet
{
class MetaStrategyNinja
{
public:
	explicit MetaStrategyNinja(BuildState& inState);

	bool createCache();

private:
	BuildState& m_state;

	std::string m_cacheFile;
};
}

#endif // CHALET_META_STRATEGY_NINJA_HPP
