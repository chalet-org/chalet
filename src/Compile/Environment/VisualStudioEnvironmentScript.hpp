/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_VISUAL_STUDIO_ENVIRONMENT_SCRIPT_HPP
#define CHALET_VISUAL_STUDIO_ENVIRONMENT_SCRIPT_HPP

#include "Compile/Environment/IEnvironmentScript.hpp"
#include "Core/VisualStudioVersion.hpp"

namespace chalet
{
struct VisualStudioEnvironmentScript final : public IEnvironmentScript
{
	static bool visualStudioExists();

	virtual bool makeEnvironment(const BuildState& inState) final;
	virtual void readEnvironmentVariablesFromDeltaFile() final;

	const std::string& architecture() const noexcept;
	void setArchitecture(const std::string& inHost, const std::string& inTarget, const StringList& inOptions);

	void setVersion(const std::string& inValue, const VisualStudioVersion inVsVersion);
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

#endif // CHALET_VISUAL_STUDIO_ENVIRONMENT_SCRIPT_HPP
