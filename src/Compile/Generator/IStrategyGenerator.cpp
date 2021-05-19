/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Generator/IStrategyGenerator.hpp"

namespace chalet
{
/*****************************************************************************/
IStrategyGenerator::IStrategyGenerator(const BuildState& inState) :
	m_state(inState)
{
}

/*****************************************************************************/
bool IStrategyGenerator::hasProjectRecipes() const
{
	return m_targetRecipes.size() > 0;
}

}
