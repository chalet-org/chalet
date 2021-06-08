/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Builder/BuildManager/SubChaletBuilder.hpp"

#include "Libraries/Format.hpp"
#include "State/BuildState.hpp"
#include "State/Target/SubChaletTarget.hpp"
#include "Terminal/Output.hpp"

namespace chalet
{
/*****************************************************************************/
SubChaletBuilder::SubChaletBuilder(const BuildState& inState, const SubChaletTarget& inTarget, const std::string& inChaletExecutable, const bool inCleanOutput) :
	m_state(inState),
	m_target(inTarget),
	m_chaletExecutable(inChaletExecutable),
	m_cleanOutput(inCleanOutput)
{
	UNUSED(m_cleanOutput);
}

/*****************************************************************************/
bool SubChaletBuilder::run()
{
	const auto& name = m_target.name();
	const auto& buildConfiguration = m_state.info.buildConfiguration();

	Output::msgBuild(buildConfiguration, name);
	Output::lineBreak();

	Output::displayStyledSymbol(Color::Blue, " ", fmt::format("executable: {}", m_chaletExecutable), false);
	Output::displayStyledSymbol(Color::Blue, " ", fmt::format("Ran the chalet project: {}", name), false);

	Output::lineBreak();

	//
	Output::msgTargetUpToDate(m_state.targets.size() > 1, name);

	return false;
}
}
