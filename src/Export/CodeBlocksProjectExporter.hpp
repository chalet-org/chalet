/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_CODEBLOCKS_PROJECT_EXPORTER_HPP
#define CHALET_CODEBLOCKS_PROJECT_EXPORTER_HPP

#include "Export/IProjectExporter.hpp"

namespace chalet
{
struct SourceTarget;
struct CompileToolchainController;

struct CodeBlocksProjectExporter final : public IProjectExporter
{
	explicit CodeBlocksProjectExporter(CentralState& inCentralState);

protected:
	virtual std::string getProjectTypeName() const final;
	virtual bool validate(const BuildState& inState) final;
	virtual bool generateProjectFiles() final;

private:
	std::string getProjectContent(const std::string& inName) const;
	std::string getProjectBuildConfiguration(const BuildState& inState, const SourceTarget& inTarget, const CompileToolchainController& inToolchain) const;

	std::string getProjectCompileOptions(const SourceTarget& inTarget, const CompileToolchainController& inToolchain) const;
	std::string getProjectCompileIncludes(const SourceTarget& inTarget) const;
	std::string getProjectLinkerOptions(const CompileToolchainController& inToolchain) const;
	std::string getProjectLinkerLibDirs(const BuildState& inState, const SourceTarget& inTarget) const;
	std::string getProjectLinkerLinks(const SourceTarget& inTarget) const;
	std::string getProjectUnits(const SourceTarget& inTarget) const;

	std::string getWorkspaceContent(const BuildState& inState) const;
	std::string getWorkspaceProject(const SourceTarget& inTarget, const bool inActive = false) const;

	std::string getWorkspaceLayoutContent(const BuildState& inState) const;
};
}

#endif // CHALET_CODEBLOCKS_PROJECT_EXPORTER_HPP
