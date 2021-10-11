/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_QUERY_CONTROLLER_HPP
#define CHALET_QUERY_CONTROLLER_HPP

#include "Libraries/Json.hpp"

namespace chalet
{
struct CommandLineInputs;
struct StatePrototype;

struct QueryController
{
	QueryController(const CommandLineInputs& inInputs, const StatePrototype& inPrototype);

	bool printListOfRequestedType();

private:
	const Json& getSettingsJson() const;

	StringList getBuildConfigurationList() const;
	StringList getUserToolchainList() const;
	StringList getArchitectures() const;
	StringList getCurrentArchitecture() const;
	StringList getCurrentBuildConfiguration() const;
	StringList getCurrentToolchain() const;
	StringList getCurrentRunTarget() const;

	const CommandLineInputs& m_inputs;
	const StatePrototype& m_prototype;

	const Json kEmptyJson;
};
}

#endif // CHALET_QUERY_CONTROLLER_HPP
