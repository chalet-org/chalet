/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_CONFIGURATION_OPTIONS_HPP
#define CHALET_CONFIGURATION_OPTIONS_HPP

namespace chalet
{
struct ConfigurationOptions
{
	const std::string& name() const noexcept;
	void setName(const std::string& inValue) noexcept;

	const std::string& optimizations() const noexcept;
	void setOptimizations(const std::string& inValue) noexcept;

	bool linkTimeOptimization() const noexcept;
	void setLinkTimeOptimization(const bool inValue) noexcept;

	bool stripSymbols() const noexcept;
	void setStripSymbols(const bool inValue) noexcept;

	bool debugSymbols() const noexcept;
	void setDebugSymbols(const bool inValue) noexcept;

private:
	std::string parseOptimizations(const std::string& inValue) noexcept;

	std::string m_name;
	std::string m_optimizations;
	bool m_linkTimeOptimization = false;
	bool m_stripSymbols = false;
	bool m_debugSymbols = false;
};
}

#endif // CHALET_CONFIGURATION_OPTIONS_HPP
