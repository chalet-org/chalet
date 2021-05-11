/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILER_CACHE_HPP
#define CHALET_COMPILER_CACHE_HPP

#include "Compile/CodeLanguage.hpp"
#include "Compile/CompilerConfig.hpp"

namespace chalet
{
struct CompilerCache
{
	CompilerCache() = default;

	const std::string& cpp() const noexcept;
	void setCpp(const std::string& inValue) noexcept;

	const std::string& cc() const noexcept;
	void setCc(const std::string& inValue) noexcept;

	const std::string& rc() const noexcept;
	void setRc(const std::string& inValue) noexcept;

	std::string getRootPathVariable();

	CompilerConfig& getConfig(const CodeLanguage inLanguage) const;

private:
	mutable std::unordered_map<CodeLanguage, CompilerConfig> m_configs;

	std::string m_cpp;
	std::string m_cc;
	std::string m_rc;
};
}

#endif // CHALET_COMPILER_CACHE_HPP
