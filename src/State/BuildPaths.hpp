/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_BUILD_PATHS_HPP
#define CHALET_BUILD_PATHS_HPP

#include "State/SourceOutputs.hpp"
#include "State/Target/SourceTarget.hpp"

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

	const std::string& rootDirectory() const noexcept;
	const std::string& outputDirectory() const noexcept;
	const std::string& buildOutputDir() const noexcept;
	const std::string& objDir() const noexcept;
	const std::string& depDir() const noexcept;
	const std::string& asmDir() const noexcept;
	const std::string& intermediateDir() const noexcept;

	const StringList& allFileExtensions() const noexcept;
	const StringList& cExtensions() const noexcept;
	const StringList& cppExtensions() const noexcept;
	StringList cxxExtensions() const noexcept;
	const StringList& objectiveCExtensions() const noexcept;
	const StringList& objectiveCppExtensions() const noexcept;
	StringList objectiveCxxExtensions() const noexcept;
	const StringList& resourceExtensions() const noexcept;

	std::string getTargetFilename(const SourceTarget& inProject) const;
	std::string getTargetBasename(const SourceTarget& inProject) const;
	std::string getPrecompiledHeader(const SourceTarget& inProject) const;
	std::string getPrecompiledHeaderTarget(const SourceTarget& inProject, const bool inPchExtension = true) const;
	std::string getPrecompiledHeaderInclude(const SourceTarget& inProject) const;
	std::string getWindowsManifestFilename(const SourceTarget& inProject) const;
	std::string getWindowsManifestResourceFilename(const SourceTarget& inProject) const;
	std::string getWindowsIconResourceFilename(const SourceTarget& inProject) const;

	void setBuildDirectoriesBasedOnProjectKind(const SourceTarget& inProject);
	SourceOutputs getOutputs(const SourceTarget& inProject, const CompilerConfig& inConfig, const bool inDumpAssembly) const;
	void setBuildEnvironment(const SourceOutputs& inOutput, const std::string& inHash) const;

	void replaceVariablesInPath(std::string& outPath, const std::string& inName = std::string()) const;

private:
	friend class BuildState;

	explicit BuildPaths(const CommandLineInputs& inInputs, const WorkspaceEnvironment& inEnvironment);

	bool initialize(const BuildInfo& inInfo, const CompilerTools& inToolchain);
	void populateFileList(const SourceTarget& inProject);

	struct SourceGroup
	{
		std::string pch;
		StringList list;
	};

	SourceFileGroupList getSourceFileGroupList(SourceGroup&& inFiles, const SourceTarget& inProject, const CompilerConfig& inConfig, const bool inDumpAssembly) const;
	std::string getObjectFile(const std::string& inSource, const bool inIsMsvc) const;
	std::string getAssemblyFile(const std::string& inSource, const bool inIsMsvc) const;
	std::string getDependencyFile(const std::string& inSource) const;
	SourceType getSourceType(const std::string& inSource) const;
	StringList getObjectFilesList(const StringList& inFiles, const SourceTarget& inProject, const bool inIsMsvc) const;
	StringList getOutputDirectoryList(const SourceGroup& inDirectoryList, const std::string& inFolder) const;
	SourceGroup getFiles(const SourceTarget& inProject) const;
	SourceGroup getDirectories(const SourceTarget& inProject) const;
	StringList getFileList(const SourceTarget& inProject) const;
	StringList getDirectoryList(const SourceTarget& inProject) const;

	const CommandLineInputs& m_inputs;
	const WorkspaceEnvironment& m_environment;

	const StringList m_cExts;
	const StringList m_cppExts;
	const StringList m_resourceExts;
	const StringList m_objectiveCExts;
	const StringList m_objectiveCppExts;

	mutable StringList m_fileListCache;
	mutable StringList m_fileListCacheShared;
	// mutable StringList m_directoryCache;

	HeapDictionary<SourceGroup> m_fileList;
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
