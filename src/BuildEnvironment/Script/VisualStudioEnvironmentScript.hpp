/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "BuildEnvironment/Script/IEnvironmentScript.hpp"
#include "BuildEnvironment/VisualStudioVersion.hpp"

namespace chalet
{
struct VisualStudioEnvironmentScript final : public IEnvironmentScript
{
	static bool visualStudioExists();

	virtual bool makeEnvironment(const BuildState& inState) final;
	virtual void readEnvironmentVariablesFromDeltaFile() final;

	bool validateArchitectureFromInput(const BuildState& inState, std::string& outHost, std::string& outTarget);

	const std::string& architecture() const noexcept;
	void setArchitecture(const std::string& inHost, const std::string& inTarget, const StringList& inOptions);

	void setVersion(const std::string& inValue, const std::string& inRawValue, const VisualStudioVersion inVsVersion);
	const std::string& detectedVersion() const noexcept;

	bool isPreset() noexcept;

	std::string getVisualStudioVersion(const VisualStudioVersion inVersion);

private:
	virtual bool saveEnvironmentFromScript() final;
	virtual StringList getAllowedArchitectures() final;

	StringList getStartOfVsWhereCommand(const VisualStudioVersion inVersion);
	void addProductOptions(StringList& outCmd);

	// inputs
	std::string m_varsAllArch;
	StringList m_varsAllArchOptions;

	// set during creation
	std::string m_pathInject;
	std::string m_visualStudioPath;
	std::string m_rawVersion;
	std::string m_detectedVersion;

	VisualStudioVersion m_vsVersion = VisualStudioVersion::None;
};
}
