/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_BUILD_CONFIGURATION_HPP
#define CHALET_BUILD_CONFIGURATION_HPP

#include "State/OptimizationLevel.hpp"

namespace chalet
{
struct BuildConfiguration
{
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

private:
	OptimizationLevel parseOptimizationLevel(const std::string& inValue) noexcept;

	std::string m_name;
	OptimizationLevel m_optimizationLevel;
	bool m_linkTimeOptimization = false;
	bool m_stripSymbols = false;
	bool m_debugSymbols = false;
	bool m_enableProfiling = false;
};

using BuildConfigurationMap = std::unordered_map<std::string, BuildConfiguration>;
}

#endif // CHALET_BUILD_CONFIGURATION_HPP
