/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILER_CONFIG_CONTROLLER_HPP
#define CHALET_COMPILER_CONFIG_CONTROLLER_HPP

#include "Compile/CodeLanguage.hpp"
#include "Compile/Toolchain/ToolchainType.hpp"

namespace chalet
{
struct CompilerConfig;
struct ICompileEnvironment;
class BuildState;

struct CompilerConfigController
{
	CompilerConfig& get(const CodeLanguage inLanguage);
	const CompilerConfig& get(const CodeLanguage inLanguage) const;

	ToolchainType type() const noexcept;

	bool isWindowsClang() const noexcept;
	bool isClang() const noexcept;
	bool isAppleClang() const noexcept;
	bool isGcc() const noexcept;
	bool isIntelClassic() const noexcept;
	bool isMingw() const noexcept;
	bool isMingwGcc() const noexcept;
	bool isMsvc() const noexcept;
	bool isClangOrMsvc() const noexcept;

private:
	friend class BuildState;

	void makeConfigForLanguage(const CodeLanguage inLanguage, const BuildState& inState, ICompileEnvironment& inEnvironment);
	bool initialize();

	void setToolchainType(const ToolchainType inType) noexcept;

	ToolchainType m_type = ToolchainType::Unknown;

	std::unordered_map<CodeLanguage, Unique<CompilerConfig>> m_configs;
};
}

#endif // CHALET_COMPILER_CONFIG_CONTROLLER_HPP
