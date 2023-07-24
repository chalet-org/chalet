/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_XCODE_PBXPROJ_GEN_HPP
#define CHALET_XCODE_PBXPROJ_GEN_HPP

#include "Libraries/Json.hpp"
#include "Utility/Uuid.hpp"

namespace chalet
{
class BuildState;

struct XcodePBXProjGen
{
	explicit XcodePBXProjGen(const std::vector<Unique<BuildState>>& inStates);

	bool saveToFile(const std::string& inFilename);

private:
	std::string getHashWithLabel(const std::string& inValue) const;
	std::string getHashWithLabel(const Uuid& inHash, const std::string& inLabel) const;
	UJson getHashedJsonValue(const std::string& inValue) const;
	UJson getHashedJsonValue(const Uuid& inHash, const std::string& inLabel) const;

	const std::vector<Unique<BuildState>>& m_states;

	std::string m_xcodeNamespaceGuid;
	std::string m_projectGuid;
};
}

#endif // CHALET_XCODE_PBXPROJ_GEN_HPP
