/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_BUILD_PATHS_HPP
#define CHALET_BUILD_PATHS_HPP

#include "BuildJson/Target/ProjectTarget.hpp"
#include "State/SourceOutputs.hpp"

namespace chalet
{
struct CommandLineInputs;
struct BuildPaths
{
	const std::string& workingDirectory() const noexcept;
	void setWorkingDirectory(const std::string& inValue);

	const std::string& buildPath() const;
	const std::string& buildOutputDir() const noexcept;
	const std::string& objDir() const noexcept;
	const std::string& depDir() const noexcept;
	const std::string& asmDir() const noexcept;

	std::string getTargetFilename(const ProjectTarget& inProject) const;
	std::string getTargetBasename(const ProjectTarget& inProject) const;
	std::string getPrecompiledHeader(const ProjectTarget& inProject) const;
	std::string getPrecompiledHeaderTarget(const ProjectTarget& inProject, const bool inPchExtension = true) const;
	std::string getPrecompiledHeaderInclude(const ProjectTarget& inProject) const;

	SourceOutputs getOutputs(const ProjectTarget& inProject, const bool inIsMsvc, const bool inObjExtension = false) const;
	void setBuildEnvironment(const SourceOutputs& inOutput, const std::string& inHash, const bool inDumpAssembly) const;

private:
	friend class BuildState;

	explicit BuildPaths(const CommandLineInputs& inInputs);

	void initialize(const std::string& inBuildConfiguration);

	struct SourceGroup
	{
		std::string pch;
		StringList list;
	};

	StringList getObjectFilesList(const StringList& inFiles, const bool inObjExtension) const;
	StringList getDependencyFilesList(const SourceGroup& inFiles) const;
	StringList getAssemblyFilesList(const SourceGroup& inFiles, const bool inObjExtension) const;
	StringList getOutputDirectoryList(const SourceGroup& inDirectoryList, const std::string& inFolder) const;
	std::string getPrecompiledHeaderDirectory(const ProjectTarget& inProject) const;
	SourceGroup getFiles(const ProjectTarget& inProject) const;
	SourceGroup getDirectories(const ProjectTarget& inProject) const;
	StringList getFileList(const ProjectTarget& inProject) const;
	StringList getDirectoryList(const ProjectTarget& inProject) const;

	const CommandLineInputs& m_inputs;

	mutable StringList m_fileListCache;
	// mutable StringList m_directoryCache;

	std::string m_workingDirectory;
	std::string m_buildOutputDir;
	std::string m_objDir;
	std::string m_depDir;
	std::string m_asmDir;

	bool m_initialized = false;
	mutable bool m_binDirMade = false;
	bool m_useCache = true;
};
}

#endif // CHALET_BUILD_PATHS_HPP
