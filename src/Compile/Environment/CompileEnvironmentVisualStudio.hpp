/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_ENVIRONMENT_VISUAL_STUDIO_HPP
#define CHALET_COMPILE_ENVIRONMENT_VISUAL_STUDIO_HPP

#include "Compile/Environment/ICompileEnvironment.hpp"
#include "Core/VisualStudioVersion.hpp"

namespace chalet
{
class BuildState;
struct CommandLineInputs;

struct VisualStudioEnvironmentConfig
{
	// inputs
	std::string varsFileOriginal;
	std::string varsFileMsvc;
	std::string varsFileMsvcDelta;
	std::string varsAllArch;
	StringList varsAllArchOptions;

	VisualStudioVersion inVersion = VisualStudioVersion::None;

	// set during creation
	std::string pathVariable;
	std::string pathInject;
	std::string visualStudioPath;
	std::string detectedVersion;

	bool isPreset = false;
};

struct CompileEnvironmentVisualStudio final : ICompileEnvironment
{
	explicit CompileEnvironmentVisualStudio(const ToolchainType inType, const CommandLineInputs& inInputs, BuildState& inState);

	static bool exists();
	static bool makeEnvironment(VisualStudioEnvironmentConfig& outConfig, const std::string& inVersion, const BuildState& inState);
	static void populateVariables(VisualStudioEnvironmentConfig& outConfig, Dictionary<std::string>& outVariables);

protected:
	virtual std::string getIdentifier() const noexcept final;
	virtual StringList getVersionCommand(const std::string& inExecutable) const final;
	virtual std::string getFullCxxCompilerString(const std::string& inVersion) const final;
	virtual bool verifyToolchain() final;

	virtual bool compilerVersionIsToolchainVersion() const final;
	virtual bool createFromVersion(const std::string& inVersion) final;
	virtual bool validateArchitectureFromInput() final;
	virtual bool getCompilerVersionAndDescription(CompilerInfo& outInfo) const final;
	virtual std::vector<CompilerPathStructure> getValidCompilerPaths() const final;

private:
	std::string makeToolchainName() const;
	static bool saveMsvcEnvironment(VisualStudioEnvironmentConfig& outConfig);
	static StringList getAllowedArchitectures();

	static StringList getStartOfVsWhereCommand(const VisualStudioVersion inVersion);
	static void addProductOptions(StringList& outCmd);
	static std::string getVisualStudioVersion(const VisualStudioVersion inVersion);

	VisualStudioEnvironmentConfig m_config;

	bool m_msvcArchitectureSet = false;
};
}

#endif // CHALET_COMPILE_ENVIRONMENT_VISUAL_STUDIO_HPP
