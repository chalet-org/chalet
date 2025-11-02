/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Bundler/MacosNotarizationMsg.hpp"

#include "State/AncillaryTools.hpp"
#include "State/BuildState.hpp"
#include "System/DefinesGithub.hpp"
#include "Terminal/Output.hpp"

namespace chalet
{
/*****************************************************************************/
MacosNotarizationMsg::MacosNotarizationMsg(const BuildState& inState) :
	m_state(inState)
{
	UNUSED(m_state);
}

/*****************************************************************************/
void MacosNotarizationMsg::showMessage(const std::string& inFile)
{
	const auto& color = Output::getAnsiStyle(Output::theme().build);
	const auto& dim = Output::getAnsiStyle(Output::theme().flair);
	const auto& reset = Output::getAnsiStyle(Color::Reset);

	if (m_state.tools.xcodeVersionMajor() < 13)
	{
		auto githubRoot = CHALET_GITHUB_ROOT;
		auto message = fmt::format(R"text(
   {color}To notarize, please do the following:{reset}
   1. Make note of the bundle id used in your Info.plist (ie. com.company.myapp)
   2. To notarize: {color}xcrun altool --notarize-app --primary-bundle-id \"(bundle id)\" --username \"APPLE ID\" --password \"APP-SPECIFIC PASSWORD\" --file {file}{reset}
   3. Wait 5 minutes or so
   4. Check status: {color}xcrun altool --notarization-history 0 -u "APPLE ID" -p "APP-SPECIFIC PASSWORD"{reset}
   5. Staple your ticket: {color}xcrun stapler staple {file}{reset}

   {dim}If the above is inaccurate or out of date, please open an issue:
   {githubRoot}/issues{reset})text",
			fmt::arg("file", inFile),
			FMT_ARG(color),
			FMT_ARG(dim),
			FMT_ARG(githubRoot),
			FMT_ARG(reset));

		Output::print(Color::Reset, message);
	}
}

}
