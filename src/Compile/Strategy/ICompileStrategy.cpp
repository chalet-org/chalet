/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Strategy/ICompileStrategy.hpp"

#include "Compile/Generator/MakefileGeneratorGNU.hpp"
#include "Compile/Generator/MakefileGeneratorNMake.hpp"
#include "Compile/Generator/NinjaGenerator.hpp"

#include "Libraries/Format.hpp"

namespace chalet
{
namespace
{
/*****************************************************************************/
[[nodiscard]] StrategyGenerator makeGenerator(const StrategyType inType, BuildState& inState)
{

	switch (inType)
	{
		case StrategyType::Native:
			return nullptr;

		case StrategyType::Ninja: {
			inState.tools.fetchNinjaVersion();
			return std::make_unique<NinjaGenerator>(inState);
		}

		case StrategyType::Makefile: {
			inState.tools.fetchMakeVersion();
#if defined(CHALET_WIN32)
			if (inState.tools.makeIsNMake())
				return std::make_unique<MakefileGeneratorNMake>(inState);
			else
#endif
				return std::make_unique<MakefileGeneratorGNU>(inState);
		}
		default:
			break;
	}

	Diagnostic::errorAbort(fmt::format("Unimplemented StrategyGenerator requested: ", static_cast<int>(inType)));
	return nullptr;
}
}

/*****************************************************************************/
ICompileStrategy::ICompileStrategy(const StrategyType inType, BuildState& inState) :
	m_state(inState),
	m_type(inType)
{
	m_generator = makeGenerator(inType, inState);
}

/*****************************************************************************/
StrategyType ICompileStrategy::type() const noexcept
{
	return m_type;
}
}
