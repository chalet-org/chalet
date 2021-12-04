/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_BUILD_CONFIGURATION_HPP
#define CHALET_BUILD_CONFIGURATION_HPP

#include "State/OptimizationLevel.hpp"
#include "State/SanitizeOptions.hpp"

namespace chalet
{
struct BuildConfiguration
{
	static StringList getDefaultBuildConfigurationNames();
	static bool makeDefaultConfiguration(BuildConfiguration& outConfig, const std::string& inName);

	bool validate(const bool isClang);

	const std::string& name() const noexcept;
	void setName(const std::string& inValue) noexcept;

	OptimizationLevel optimizationLevel() const noexcept;
	void setOptimizationLevel(const std::string& inValue) noexcept;

	bool linkTimeOptimization() const noexcept;
	void setLinkTimeOptimization(const bool inValue) noexcept;

	bool stripSymbols() const noexcept;
	void setStripSymbols(const bool inValue) noexcept;

	bool debugSymbols() const noexcept;
	void setDebugSymbols(const bool inValue) noexcept;

	bool enableProfiling() const noexcept;
	void setEnableProfiling(const bool inValue) noexcept;

	void addSanitizeOptions(StringList&& inList);
	void addSanitizeOption(std::string&& inValue);
	bool enableSanitizers() const noexcept;
	bool sanitizeAddress() const noexcept;
	bool sanitizeThread() const noexcept;
	bool sanitizeMemory() const noexcept;
	bool sanitizeLeaks() const noexcept;
	bool sanitizeUndefined() const noexcept;

	bool isReleaseWithDebugInfo() const noexcept;
	bool isMinSizeRelease() const noexcept;
	bool isDebuggable() const noexcept;

private:
	OptimizationLevel parseOptimizationLevel(const std::string& inValue) noexcept;

	std::string m_name;

	SanitizeOptions::Type m_sanitizeOptions = SanitizeOptions::None;

	OptimizationLevel m_optimizationLevel = OptimizationLevel::None;

	bool m_linkTimeOptimization = false;
	bool m_stripSymbols = false;
	bool m_debugSymbols = false;
	bool m_enableProfiling = false;
};

using BuildConfigurationMap = Dictionary<BuildConfiguration>;
}

#endif // CHALET_BUILD_CONFIGURATION_HPP
