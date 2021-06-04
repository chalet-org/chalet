/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILER_CONFIG_HPP
#define CHALET_COMPILER_CONFIG_HPP

#include <unordered_map>

#include "Compile/CodeLanguage.hpp"
#include "Compile/Toolchain/ToolchainType.hpp"

namespace chalet
{
struct CompilerTools;

struct CompilerConfig
{
	explicit CompilerConfig(const CodeLanguage inLanguage, const CompilerTools& inCompilers);

	CodeLanguage language() const noexcept;

	const std::string& compilerExecutable() const noexcept;
	const std::string& compilerPathBin() const noexcept;
	const std::string& compilerPathLib() const noexcept;
	const std::string& compilerPathInclude() const noexcept;

	bool configureCompilerPaths();
	bool testCompilerMacros();
	bool getSupportedCompilerFlags();

	bool isFlagSupported(const std::string& inFlag) const;
	bool isLinkSupported(const std::string& inLink, const StringList& inDirectories) const;

	CppCompilerType compilerType() const noexcept;
	bool isClang() const noexcept;
	bool isAppleClang() const noexcept;
	bool isGcc() const noexcept;
	bool isMingw() const noexcept;
	bool isMingwGcc() const noexcept;
	bool isMsvc() const noexcept;
	bool isClangOrMsvc() const noexcept;

private:
	void parseGnuHelpList(const std::string& inIdentifier);

	const std::unordered_map<std::string, std::string> kCompilerStructures;

	const CompilerTools& m_compilers;

	std::string m_compilerPath{ "/usr" };
	std::string m_compilerPathBin{ "/usr/bin" };
	std::string m_compilerPathLib{ "/usr/lib" };
	std::string m_compilerPathInclude{ "/usr/include" };

	std::unordered_map<std::string, bool> m_supportedFlags;

	CodeLanguage m_language = CodeLanguage::None;
	CppCompilerType m_compilerType = CppCompilerType::Unknown;
};
}

#endif // CHALET_COMPILER_CONFIG_HPP
