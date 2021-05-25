/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILER_TOOLS_HPP
#define CHALET_COMPILER_TOOLS_HPP

#include "Compile/CodeLanguage.hpp"
#include "Compile/CompilerConfig.hpp"

namespace chalet
{
struct CompilerTools
{
	CompilerTools() = default;

	void fetchCompilerVersions();

	const std::string& compilerVersionStringCpp() const noexcept;
	const std::string& compilerVersionStringC() const noexcept;

	const std::string& archiver() const noexcept;
	void setArchiver(const std::string& inValue) noexcept;
	bool isArchiverLibTool() const noexcept;

	const std::string& cpp() const noexcept;
	void setCpp(const std::string& inValue) noexcept;

	const std::string& cc() const noexcept;
	void setCc(const std::string& inValue) noexcept;

	const std::string& linker() const noexcept;
	void setLinker(const std::string& inValue) noexcept;

	const std::string& rc() const noexcept;
	void setRc(const std::string& inValue) noexcept;

	std::string getRootPathVariable();

	CompilerConfig& getConfig(const CodeLanguage inLanguage) const;

private:
	std::string parseVersionMSVC(const std::string& inExecutable) const;
	std::string parseVersionGNU(const std::string& inExecutable, const std::string_view inEol = "\n") const;

	mutable std::unordered_map<CodeLanguage, CompilerConfig> m_configs;

	std::string m_archiver;
	std::string m_cpp;
	std::string m_cc;
	std::string m_linker;
	std::string m_rc;

	std::string m_compilerVersionStringCpp;
	std::string m_compilerVersionStringC;

	bool m_isArchiverLibTool = false;
};
}

#endif // CHALET_COMPILER_TOOLS_HPP
