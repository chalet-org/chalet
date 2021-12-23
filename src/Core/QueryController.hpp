/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_QUERY_CONTROLLER_HPP
#define CHALET_QUERY_CONTROLLER_HPP

#include "Libraries/Json.hpp"

namespace chalet
{
struct StatePrototype;

struct QueryController
{
	QueryController(const StatePrototype& inPrototype);

	bool printListOfRequestedType();

private:
	const Json& getSettingsJson() const;

	StringList getBuildConfigurationList() const;
	StringList getUserToolchainList() const;
	StringList getArchitectures() const;
	StringList getArchitectures(const std::string& inToolchain) const;
	StringList getCurrentArchitecture() const;
	StringList getCurrentBuildConfiguration() const;
	StringList getCurrentToolchain() const;
	StringList getCurrentRunTarget() const;
	StringList getChaletJsonState() const;
	StringList getSettingsJsonState() const;
	StringList getChaletSchema() const;
	StringList getSettingsSchema() const;

	const StatePrototype& m_prototype;

	const Json kEmptyJson;
};
}

#endif // CHALET_QUERY_CONTROLLER_HPP
