/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Libraries/Json.hpp"
#include "Query/QueryOption.hpp"

namespace chalet
{
struct CentralState;

struct QueryController
{
	QueryController(const CentralState& inCentralState);

	bool printListOfRequestedType();
	StringList getRequestedType(const QueryOption inOption) const;

	StringList getArchitectures(const std::string& inToolchain) const;

private:
	const Json& getSettingsJson() const;

	Json getBuildConfigurationDetails() const;

	StringList getVersion() const;
	StringList getBuildConfigurationList() const;
	StringList getUserToolchainList() const;
	StringList getArchitectures() const;
	StringList getOptions() const;
	StringList getCurrentArchitecture() const;
	StringList getCurrentBuildConfiguration() const;
	StringList getCurrentToolchain() const;
	StringList getCurrentToolchainBuildStrategy() const;
	StringList getToolchainBuildStrategies() const;
	StringList getCurrentToolchainBuildPathStyle() const;
	StringList getToolchainBuildPathStyles() const;
	StringList getAllBuildTargets() const;
	StringList getAllRunTargets() const;
	StringList getCurrentLastTarget() const;
	StringList getChaletJsonState() const;
	StringList getSettingsJsonState() const;
	StringList getChaletSchema() const;
	StringList getSettingsSchema() const;

	//
	StringList getRunnableTargetKinds() const;
	StringList getMetaBuildKinds() const;

	const CentralState& m_centralState;

	const Json kEmptyJson;
};
}
