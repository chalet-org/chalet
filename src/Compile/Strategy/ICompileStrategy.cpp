/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Strategy/ICompileStrategy.hpp"

#include "Compile/Generator/IStrategyGenerator.hpp"

#include "Compile/Strategy/CompileStrategyMakefile.hpp"
#include "Compile/Strategy/CompileStrategyNative.hpp"
#include "Compile/Strategy/CompileStrategyNinja.hpp"

namespace chalet
{
namespace
{
}

/*****************************************************************************/
ICompileStrategy::ICompileStrategy(const StrategyType inType, BuildState& inState) :
	m_state(inState),
	m_type(inType)
{
	m_generator = IStrategyGenerator::make(inType, inState);
}

/*****************************************************************************/
[[nodiscard]] CompileStrategy ICompileStrategy::make(const StrategyType inType, BuildState& inState)
{
	switch (inType)
	{
		case StrategyType::Makefile:
			return std::make_unique<CompileStrategyMakefile>(inState);
		case StrategyType::Ninja:
			return std::make_unique<CompileStrategyNinja>(inState);
		case StrategyType::Native:
			return std::make_unique<CompileStrategyNative>(inState);
		default:
			break;
	}

	Diagnostic::errorAbort("Unimplemented StrategyType requested: ", static_cast<int>(inType));
	return nullptr;
}

/*****************************************************************************/
StrategyType ICompileStrategy::type() const noexcept
{
	return m_type;
}

/*****************************************************************************/
bool ICompileStrategy::doPostBuild() const
{
	return true;
}
}
