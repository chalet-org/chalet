/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Platform/Arch.hpp"

namespace chalet
{
class BuildState;
struct MesonTarget;

class MesonBuilder
{
public:
	explicit MesonBuilder(const BuildState& inState, const MesonTarget& inTarget, const bool inQuotedPaths = false);

	bool run();

	std::string getBuildFile(const bool inForce = false) const;
	std::string getCacheFile() const;

	StringList getSetupCommand();
	StringList getBuildCommand() const;
	StringList getBuildCommand(const std::string& inOutputLocation) const;
	StringList getInstallCommand() const;
	StringList getInstallCommand(const std::string& inOutputLocation) const;

	bool createNativeFile() const;

private:
	bool dependencyHasUpdated() const;
	StringList getSetupCommand(const std::string& inLocation, const std::string& inBuildFile) const;

	std::string getLocation() const;

	std::string getBackend() const;
	std::string getMesonCompatibleBuildConfiguration() const;
	std::string getMesonCompatibleOptimizationFlag() const;
	std::string getNativeFileOutputPath() const;
	std::string getPlatform(const bool isTarget) const;
	std::string getCpuFamily(const Arch::Cpu inArch) const;
	std::string getCpuEndianness(const bool isTarget) const;
	std::string getStripBinary() const;

	std::string getQuotedPath(const std::string& inPath) const;

	const std::string& outputLocation() const;

	bool usesNinja() const;

	const BuildState& m_state;
	const MesonTarget& m_target;

	u32 m_mesonVersionMajorMinor = 0;

	bool m_quotedPaths = false;
};
}
