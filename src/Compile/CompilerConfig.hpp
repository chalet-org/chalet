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

	bool getSupportedCompilerFlags();

	bool isFlagSupported(const std::string& inFlag) const;
	bool isLinkSupported(const std::string& inLink, const StringList& inDirectories) const;

private:
	void parseGnuHelpList(const StringList& inCommand);
	void parseClangHelpList();

	const BuildState& m_state;
	ICompileEnvironment& m_environment;

	Dictionary<bool> m_supportedFlags;

	CodeLanguage m_language = CodeLanguage::None;
};
}

#endif // CHALET_COMPILER_CONFIG_HPP
