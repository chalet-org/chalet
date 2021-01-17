/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_BUILD_PATHS_HPP
#define CHALET_BUILD_PATHS_HPP

#include "BuildJson/ProjectConfiguration.hpp"
#include "State/SourceOutputs.hpp"

namespace chalet
{
struct BuildPaths
{
	const std::string& workingDirectory() const noexcept;
	void setWorkingDirectory(const std::string& inValue);

	const std::string& binDir() const;
	const std::string& buildDir() const noexcept;
	const std::string& objDir() const noexcept;
	const std::string& depDir() const noexcept;
	const std::string& asmDir() const noexcept;

	std::string getTargetFilename(const ProjectConfiguration& inProject) const;
	std::string getTargetBasename(const ProjectConfiguration& inProject) const;
	std::string getPrecompiledHeader(const ProjectConfiguration& inProject) const;
	std::string getPrecompiledHeaderTarget(const ProjectConfiguration& inProject, const bool inIsClang) const;
	std::string getPrecompiledHeaderInclude(const ProjectConfiguration& inProject) const;

	SourceOutputs getOutputs(const ProjectConfiguration& inProject) const;
	void setBuildEnvironment(const SourceOutputs& inOutput, const bool inDumpAssembly) const;

private:
	friend class BuildState;

	BuildPaths() = default;

	void initialize(const std::string& inBuildConfiguration);

	struct SourceGroup
	{
		std::string pch;
		StringList list;
	};

	StringList getObjectFilesList(const SourceGroup& inFiles) const;
	StringList getDependencyFilesList(const SourceGroup& inFiles) const;
	StringList getAssemblyFilesList(const SourceGroup& inFiles) const;
	StringList getOutputDirectoryList(const SourceGroup& inDirectoryList, const std::string& inFolder) const;
	std::string getPrecompiledHeaderDirectory(const ProjectConfiguration& inProject) const;
	SourceGroup getFiles(const ProjectConfiguration& inProject) const;
	SourceGroup getDirectories(const ProjectConfiguration& inProject) const;
	StringList getFileList(const ProjectConfiguration& inProject) const;
	StringList getDirectoryList(const ProjectConfiguration& inProject) const;

	std::string m_workingDirectory;
	mutable std::string m_binDir{ "bin" };
	std::string m_buildDir;
	std::string m_objDir;
	std::string m_depDir;
	std::string m_asmDir;

	bool m_initialized = false;
	mutable bool m_binDirMade = false;
};
}

#endif // CHALET_BUILD_PATHS_HPP
