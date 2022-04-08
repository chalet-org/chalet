/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_BUILD_PATHS_HPP
#define CHALET_BUILD_PATHS_HPP

#include "State/SourceOutputs.hpp"

namespace chalet
{
struct CompilerTools;
struct BuildInfo;
struct SourceTarget;
struct WorkspaceEnvironment;
class BuildState;

struct BuildPaths
{
	const std::string& homeDirectory() const noexcept;

	const std::string& rootDirectory() const noexcept;
	const std::string& outputDirectory() const noexcept;
	const std::string& buildOutputDir() const;
	const std::string& objDir() const;
	const std::string& depDir() const;
	const std::string& asmDir() const;
	std::string intermediateDir(const SourceTarget& inProject) const;
	StringList buildDirectories() const;

	const StringList& allFileExtensions() const noexcept;
	StringList objectiveCxxExtensions() const noexcept;
	const StringList& resourceExtensions() const noexcept;
	const std::string& cxxExtension() const;

	std::string getTargetFilename(const SourceTarget& inProject) const;
	std::string getTargetBasename(const SourceTarget& inProject) const;
	std::string getPrecompiledHeaderTarget(const SourceTarget& inProject) const;
	std::string getPrecompiledHeaderInclude(const SourceTarget& inProject) const;
	std::string getWindowsManifestFilename(const SourceTarget& inProject) const;
	std::string getWindowsManifestResourceFilename(const SourceTarget& inProject) const;
	std::string getWindowsIconResourceFilename(const SourceTarget& inProject) const;

	void setBuildDirectoriesBasedOnProjectKind(const SourceTarget& inProject);
	void clearOutputCaches();
	Unique<SourceOutputs> getOutputs(const SourceTarget& inProject, StringList& outFileCache, const bool inDumpAssembly);

private:
	friend class BuildState;

	explicit BuildPaths(const BuildState& inState);

	bool initialize();
	void populateFileList(const SourceTarget& inProject);

	struct SourceGroup
	{
		std::string pch;
		StringList list;
	};

	SourceFileGroupList getSourceFileGroupList(SourceGroup&& inFiles, const SourceTarget& inProject, StringList& outFileCache, const bool inDumpAssembly);
	std::string getObjectFile(const std::string& inSource) const;
	std::string getAssemblyFile(const std::string& inSource) const;
	SourceType getSourceType(const std::string& inSource) const;
	StringList getObjectFilesList(const StringList& inFiles, const SourceTarget& inProject) const;
	StringList getOutputDirectoryList(const SourceGroup& inDirectoryList, const std::string& inFolder) const;
	SourceGroup getFiles(const SourceTarget& inProject) const;
	SourceGroup getDirectories(const SourceTarget& inProject) const;
	StringList getFileList(const SourceTarget& inProject) const;
	StringList getDirectoryList(const SourceTarget& inProject) const;

	const BuildState& m_state;

	const StringList m_cExts;
	const StringList m_resourceExts;
	const StringList m_objectiveCExts;
	const StringList m_objectiveCppExts;

	HeapDictionary<SourceGroup> m_fileList;
	StringList m_allFileExtensions;

	std::string m_buildOutputDir;
	std::string m_objDir;
	std::string m_depDir;
	std::string m_asmDir;
	std::string m_intermediateDir;

	mutable std::string m_cxxExtension;

	bool m_initialized = false;
};
}

#endif // CHALET_BUILD_PATHS_HPP
