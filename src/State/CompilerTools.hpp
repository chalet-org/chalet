/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILER_TOOLS_HPP
#define CHALET_COMPILER_TOOLS_HPP

#include "Compile/CodeLanguage.hpp"
#include "Compile/CompilerConfig.hpp"
#include "Compile/Toolchain/ToolchainType.hpp"
#include "State/Target/IBuildTarget.hpp"

namespace chalet
{
class BuildState;
struct CommandLineInputs;

struct CompilerTools
{
	explicit CompilerTools(const CommandLineInputs& inInputs, const BuildState& inState);

	void detectToolchain();
	bool initialize(const BuildTargetList& inTargets);
	void fetchCompilerVersions();

	ToolchainType detectedToolchain() const;
	const std::string& compiler() const noexcept;

	const std::string& compilerVersionStringCpp() const noexcept;
	const std::string& compilerVersionStringC() const noexcept;

	const std::string& archiver() const noexcept;
	void setArchiver(std::string&& inValue) noexcept;
	bool isArchiverLibTool() const noexcept;

	const std::string& cpp() const noexcept;
	void setCpp(std::string&& inValue) noexcept;

	const std::string& cc() const noexcept;
	void setCc(std::string&& inValue) noexcept;

	const std::string& linker() const noexcept;
	void setLinker(std::string&& inValue) noexcept;

	const std::string& rc() const noexcept;
	void setRc(std::string&& inValue) noexcept;

	std::string getRootPathVariable();

	CompilerConfig& getConfig(const CodeLanguage inLanguage) const;

private:
	bool initializeCompilerConfigs(const BuildTargetList& inTargets);

	std::string parseVersionMSVC(const std::string& inExecutable) const;
	std::string parseVersionGNU(const std::string& inExecutable, const std::string_view inEol = "\n") const;

	const CommandLineInputs& m_inputs;
	const BuildState& m_state;

	mutable std::unordered_map<CodeLanguage, std::unique_ptr<CompilerConfig>> m_configs;

	std::string m_archiver;
	std::string m_cpp;
	std::string m_cc;
	std::string m_linker;
	std::string m_rc;

	std::string m_compilerVersionStringCpp;
	std::string m_compilerVersionStringC;

	ToolchainType m_detectedToolchain = ToolchainType::Unknown;

	bool m_isArchiverLibTool = false;
	bool m_ccDetected = false;
};
}

#endif // CHALET_COMPILER_TOOLS_HPP
