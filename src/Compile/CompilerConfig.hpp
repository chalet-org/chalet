/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILER_CONFIG_HPP
#define CHALET_COMPILER_CONFIG_HPP

#include "Compile/CodeLanguage.hpp"
#include "Compile/Toolchain/ToolchainType.hpp"

namespace chalet
{
class BuildState;
struct ICompileEnvironment;

struct CompilerConfig
{
	explicit CompilerConfig(const CodeLanguage inLanguage, const BuildState& inState, ICompileEnvironment& inEnvironment);

	CodeLanguage language() const noexcept;

	bool isInitialized() const noexcept;

	const std::string& compilerExecutable() const noexcept;
	const std::string& compilerPathBin() const noexcept;
	const std::string& compilerPathLib() const noexcept;
	const std::string& compilerPathInclude() const noexcept;

	bool configureCompilerPaths();
	bool testCompilerMacros();
	bool getSupportedCompilerFlags();

	bool isFlagSupported(const std::string& inFlag) const;
	bool isLinkSupported(const std::string& inLink, const StringList& inDirectories) const;

private:
	void parseGnuHelpList(const StringList& inCommand);
	void parseClangHelpList();

	std::string getCompilerMacros(const std::string& inCompilerExec);

	const BuildState& m_state;
	ICompileEnvironment& m_environment;

	std::string m_compilerPath{ "/usr" };
	std::string m_compilerPathBin{ "/usr/bin" };
	std::string m_compilerPathLib{ "/usr/lib" };
	std::string m_compilerPathInclude{ "/usr/include" };
	std::string m_macrosFile;
	std::string m_flagsFile;

	Dictionary<bool> m_supportedFlags;

	CodeLanguage m_language = CodeLanguage::None;
	ToolchainType m_type = ToolchainType::Unknown;
};
}

#endif // CHALET_COMPILER_CONFIG_HPP
