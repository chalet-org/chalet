/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "State/OptimizationLevel.hpp"
#include "State/SanitizeOptions.hpp"

namespace chalet
{
class BuildState;

struct BuildConfiguration
{
	static StringList getDefaultBuildConfigurationNames();
	static std::string getDefaultReleaseConfigurationName();
	static bool makeDefaultConfiguration(BuildConfiguration& outConfig, const std::string& inName);

	bool validate(const BuildState& inState);

	const std::string& name() const noexcept;
	void setName(const std::string& inValue) noexcept;

	OptimizationLevel optimizationLevel() const noexcept;
	std::string optimizationLevelString() const;
	void setOptimizationLevel(const std::string& inValue);

	bool interproceduralOptimization() const noexcept;
	void setInterproceduralOptimization(const bool inValue) noexcept;

	bool debugSymbols() const noexcept;
	void setDebugSymbols(const bool inValue) noexcept;

	bool enableProfiling() const noexcept;
	void setEnableProfiling(const bool inValue) noexcept;

	void addSanitizeOptions(StringList&& inList);
	void addSanitizeOption(std::string&& inValue);
	bool enableSanitizers() const noexcept;
	bool sanitizeAddress() const noexcept;
	bool sanitizeHardwareAddress() const noexcept;
	bool sanitizeThread() const noexcept;
	bool sanitizeMemory() const noexcept;
	bool sanitizeLeaks() const noexcept;
	bool sanitizeUndefinedBehavior() const noexcept;
	StringList getSanitizerList() const;

	bool isReleaseWithDebugInfo() const noexcept;
	bool isMinSizeRelease() const noexcept;
	bool isDebuggable() const noexcept;

private:
	OptimizationLevel parseOptimizationLevel(const std::string& inValue) const;
	std::string getOptimizationLevelString(const OptimizationLevel inValue) const;

	std::string m_name;

	SanitizeOptions::Type m_sanitizeOptions = SanitizeOptions::None;

	OptimizationLevel m_optimizationLevel = OptimizationLevel::None;

	bool m_interproceduralOptimization = false;
	bool m_debugSymbols = false;
	bool m_enableProfiling = false;
};

using BuildConfigurationMap = OrderedDictionary<BuildConfiguration>;
}
