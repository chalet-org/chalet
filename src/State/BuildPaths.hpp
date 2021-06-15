/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_BUILD_PATHS_HPP
#define CHALET_BUILD_PATHS_HPP

#include "State/SourceOutputs.hpp"
#include "State/Target/ProjectTarget.hpp"

namespace chalet
{
struct CommandLineInputs;
struct BuildInfo;

struct BuildPaths
{
	const std::string& workingDirectory() const noexcept;
	void setWorkingDirectory(std::string&& inValue);

	const std::string& externalDepDir() const noexcept;
	void setExternalDepDir(std::string&& inValue) noexcept;

	const std::string& buildPath() const;
	const std::string& buildOutputDir() const noexcept;
	const std::string& configuration() const noexcept;
	const std::string& objDir() const noexcept;
	const std::string& depDir() const noexcept;
	const std::string& asmDir() const noexcept;
	const std::string& intermediateDir() const noexcept;

	std::string getTargetFilename(const ProjectTarget& inProject) const;
	std::string getTargetBasename(const ProjectTarget& inProject) const;
	std::string getPrecompiledHeader(const ProjectTarget& inProject) const;
	std::string getPrecompiledHeaderTarget(const ProjectTarget& inProject, const bool inPchExtension = true) const;
	std::string getPrecompiledHeaderInclude(const ProjectTarget& inProject) const;
	std::string getWindowsManifestFilename(const ProjectTarget& inProject) const;
	std::string getWindowsManifestResourceFilename(const ProjectTarget& inProject) const;
	std::string getWindowsIconResourceFilename(const ProjectTarget& inProject) const;

	SourceOutputs getOutputs(const ProjectTarget& inProject, const bool inIsMsvc, const bool inDumpAssembly, const bool inObjExtension = false) const;
	void setBuildEnvironment(const SourceOutputs& inOutput, const std::string& inHash) const;

	void replaceVariablesInPath(std::string& outPath, const std::string& inName = std::string()) const;

private:
	friend class BuildState;

	explicit BuildPaths(const CommandLineInputs& inInputs, const BuildInfo& inInfo);

	void initialize();

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
	const BuildInfo& m_info;

	mutable StringList m_fileListCache;
	// mutable StringList m_directoryCache;

	std::string m_workingDirectory;
	std::string m_configuration;
	std::string m_externalDepDir;
	std::string m_buildOutputDir;
	std::string m_objDir;
	std::string m_depDir;
	std::string m_asmDir;
	std::string m_intermediateDir;

	std::string m_homeDirectory;

	bool m_initialized = false;
	bool m_useCache = true;
};
}

#endif // CHALET_BUILD_PATHS_HPP
