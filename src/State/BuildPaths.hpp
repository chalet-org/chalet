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
struct CompilerTools;
struct CommandLineInputs;
struct BuildInfo;
struct WorkspaceEnvironment;
struct CompilerConfig;

struct BuildPaths
{
	const std::string& homeDirectory() const noexcept;

	const std::string& outputDirectory() const;
	const std::string& buildOutputDir() const noexcept;
	const std::string& objDir() const noexcept;
	const std::string& depDir() const noexcept;
	const std::string& asmDir() const noexcept;
	const std::string& intermediateDir() const noexcept;

	const StringList& allFileExtensions() const noexcept;

	std::string getTargetFilename(const ProjectTarget& inProject) const;
	std::string getTargetBasename(const ProjectTarget& inProject) const;
	std::string getPrecompiledHeader(const ProjectTarget& inProject) const;
	std::string getPrecompiledHeaderTarget(const ProjectTarget& inProject, const bool inPchExtension = true) const;
	std::string getPrecompiledHeaderInclude(const ProjectTarget& inProject) const;
	std::string getWindowsManifestFilename(const ProjectTarget& inProject) const;
	std::string getWindowsManifestResourceFilename(const ProjectTarget& inProject) const;
	std::string getWindowsIconResourceFilename(const ProjectTarget& inProject) const;

	SourceOutputs getOutputs(const ProjectTarget& inProject, const CompilerConfig& inConfig, const bool inDumpAssembly) const;
	void setBuildEnvironment(const SourceOutputs& inOutput, const std::string& inHash) const;

	void replaceVariablesInPath(std::string& outPath, const std::string& inName = std::string()) const;

private:
	friend class BuildState;

	explicit BuildPaths(const CommandLineInputs& inInputs, const WorkspaceEnvironment& inEnvironment);

	bool initialize(const BuildInfo& inInfo, const CompilerTools& inToolchain);
	void populateFileList(const ProjectTarget& inProject);

	struct SourceGroup
	{
		std::string pch;
		StringList list;
	};

	SourceFileGroupList getSourceFileGroupList(SourceGroup&& inFiles, const ProjectTarget& inProject, const CompilerConfig& inConfig, const bool inDumpAssembly) const;
	std::string getObjectFile(const std::string& inSource, const bool inIsMsvc) const;
	std::string getAssemblyFile(const std::string& inSource, const bool inIsMsvc) const;
	std::string getDependencyFile(const std::string& inSource) const;
	SourceType getSourceType(const std::string& inSource) const;
	StringList getObjectFilesList(const StringList& inFiles, const ProjectTarget& inProject, const bool inIsMsvc) const;
	StringList getOutputDirectoryList(const SourceGroup& inDirectoryList, const std::string& inFolder) const;
	SourceGroup getFiles(const ProjectTarget& inProject) const;
	SourceGroup getDirectories(const ProjectTarget& inProject) const;
	StringList getFileList(const ProjectTarget& inProject) const;
	StringList getDirectoryList(const ProjectTarget& inProject) const;

	const CommandLineInputs& m_inputs;
	const WorkspaceEnvironment& m_environment;

	const StringList m_cExts;
	const StringList m_cppExts;
	const StringList m_resourceExts;
	const StringList m_objectiveCExts;
	const StringList m_objectiveCppExts;

	mutable StringList m_fileListCache;
	// mutable StringList m_directoryCache;

	std::unordered_map<std::string, std::unique_ptr<SourceGroup>> m_fileList;
	StringList m_allFileExtensions;

	std::string m_buildOutputDir;
	std::string m_objDir;
	std::string m_depDir;
	std::string m_asmDir;
	std::string m_intermediateDir;

	bool m_initialized = false;
	bool m_useCache = true;
};
}

#endif // CHALET_BUILD_PATHS_HPP
