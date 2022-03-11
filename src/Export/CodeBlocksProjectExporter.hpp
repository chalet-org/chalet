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

struct CodeBlocksProjectExporter final : public IProjectExporter
{
	explicit CodeBlocksProjectExporter(const BuildState& inCentralState);

	virtual bool validate() final;
	virtual bool generate() final;

private:
	std::string getProjectContent(const SourceTarget& inTarget) const;

	std::string getProjectCompileOptions(const SourceTarget& inTarget) const;
	std::string getProjectCompileIncludes(const SourceTarget& inTarget) const;
	std::string getProjectLinkerOptions(const SourceTarget& inTarget) const;
	std::string getProjectLinkerLibDirs(const SourceTarget& inTarget) const;
	std::string getProjectLinkerLinks(const SourceTarget& inTarget) const;
	std::string getProjectUnits(const SourceTarget& inTarget) const;

	std::string getWorkspaceContent() const;
	std::string getWorkspaceProject(const SourceTarget& inTarget, const bool inActive = false) const;

	std::string m_cwd;
};
}

#endif // CHALET_CODEBLOCKS_PROJECT_EXPORTER_HPP
