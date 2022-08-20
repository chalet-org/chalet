/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_QUERY_CONTROLLER_HPP
#define CHALET_QUERY_CONTROLLER_HPP

#include "Core/QueryOption.hpp"
#include "Libraries/Json.hpp"

namespace chalet
{
struct CentralState;

struct QueryController
{
	QueryController(const CentralState& inCentralState);

	bool printListOfRequestedType();
	StringList getRequestedType(const QueryOption inOption) const;

private:
	const Json& getSettingsJson() const;

	Json getBuildConfigurationDetails() const;

	StringList getBuildConfigurationList() const;
	StringList getUserToolchainList() const;
	StringList getArchitectures() const;
	StringList getArchitectures(const std::string& inToolchain) const;
	StringList getCurrentArchitecture() const;
	StringList getCurrentBuildConfiguration() const;
	StringList getCurrentToolchain() const;
	StringList getToolchainStrategies() const;
	StringList getToolchainBuildPathStyles() const;
	StringList getAllBuildTargets() const;
	StringList getAllRunTargets() const;
	StringList getCurrentRunTarget() const;
	StringList getChaletJsonState() const;
	StringList getSettingsJsonState() const;
	StringList getChaletSchema() const;
	StringList getSettingsSchema() const;

	const CentralState& m_centralState;

	const Json kEmptyJson;
};
}

#endif // CHALET_QUERY_CONTROLLER_HPP
