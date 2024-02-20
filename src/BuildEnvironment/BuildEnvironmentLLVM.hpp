/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "BuildEnvironment/BuildEnvironmentGNU.hpp"

namespace chalet
{
struct BuildEnvironmentLLVM : BuildEnvironmentGNU
{
	explicit BuildEnvironmentLLVM(const ToolchainType inType, BuildState& inState);

	virtual bool supportsCppModules() const override;

protected:
	virtual std::string getArchiveExtension() const override;
	virtual std::string getPrecompiledHeaderExtension() const override;

	virtual std::string getCompilerAliasForVisualStudio() const override;

	virtual std::string getModuleDirectivesDependencyFile(const std::string& inSource) const override;
	virtual std::string getModuleBinaryInterfaceFile(const std::string& inSource) const override;
	virtual std::string getModuleBinaryInterfaceDependencyFile(const std::string& inSource) const override;

	virtual StringList getVersionCommand(const std::string& inExecutable) const override;
	virtual std::string getFullCxxCompilerString(const std::string& inPath, const std::string& inVersion) const override;

	virtual bool readArchitectureTripleFromCompiler() override;
	virtual bool validateArchitectureFromInput() override;
	virtual ToolchainType getToolchainTypeFromMacros(const std::string& inMacros) const override;
	virtual bool populateSupportedFlags(const std::string& inExecutable) override;
	virtual void parseSupportedFlagsFromHelpList(const StringList& inCommand) override;
};
}
