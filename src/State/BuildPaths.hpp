/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "State/SourceOutputs.hpp"

namespace chalet
{
struct CompilerTools;
struct BuildInfo;
struct SourceTarget;
struct CMakeTarget;
struct WorkspaceEnvironment;
class BuildState;
struct IBuildTarget;

struct BuildPaths
{
	const std::string& homeDirectory() const noexcept;

	const std::string& rootDirectory() const noexcept;
	const std::string& outputDirectory() const noexcept;
	const std::string& buildOutputDir() const;
	const std::string& externalBuildDir() const;
	const std::string& objDir() const;
	const std::string& depDir() const;
	const std::string& asmDir() const;
	std::string intermediateDir(const SourceTarget& inProject) const;
	std::string intermediateIncludeDir(const SourceTarget& inProject) const;
	std::string bundleObjDir(const std::string& inName) const;
	std::string currentCompileCommands() const;
	StringList getBuildDirectories(const SourceTarget& inProject) const;

	std::string getExternalDir(const std::string& inName) const;
	std::string getExternalBuildDir(const std::string& inName) const;

	const StringList& windowsResourceExtensions() const noexcept;
	const StringList& objectiveCExtensions() const noexcept;
	const std::string& objectiveCppExtension() const;

	std::string getTargetFilename(const SourceTarget& inProject) const;
	std::string getTargetFilename(const CMakeTarget& inProject) const;
	std::string getTargetBasename(const SourceTarget& inProject) const;
	std::string getExecutableTargetPath(const IBuildTarget& inTarget) const;
	std::string getPrecompiledHeaderTarget(const SourceTarget& inProject) const;
	std::string getPrecompiledHeaderObject(const std::string& inTarget) const;
	std::string getPrecompiledHeaderInclude(const SourceTarget& inProject) const;
	std::string getWindowsManifestFilename(const SourceTarget& inProject) const;
	std::string getWindowsManifestResourceFilename(const SourceTarget& inProject) const;
	std::string getWindowsIconResourceFilename(const SourceTarget& inProject) const;
	std::string getUnityBuildSourceFilename(const SourceTarget& inProject) const;

	std::string getNormalizedOutputPath(const std::string& inPath) const;
	std::string getNormalizedDirectoryPath(const std::string& inPath) const;
	std::string getBuildOutputPath(std::string inPath) const;

	void setBuildDirectoriesBasedOnProjectKind(const SourceTarget& inProject);
	Unique<SourceOutputs> getOutputs(const SourceTarget& inProject, StringList& outFileCache);

	SourceType getSourceType(const std::string& inSource) const;

private:
	friend class BuildState;

	explicit BuildPaths(const BuildState& inState);

	bool initialize();

	struct SourceGroup
	{
		std::string pch;
		StringList list;
	};

	void normalizedPath(std::string& outPath) const;

	SourceFileGroupList getSourceFileGroupList(const SourceGroup& inFiles, const SourceTarget& inProject, StringList& outFileCache);
	std::string getObjectFile(const std::string& inSource) const;
	std::string getAssemblyFile(const std::string& inSource) const;
	StringList getObjectFilesList(const StringList& inFiles, const SourceTarget& inProject) const;
	StringList getOutputDirectoryList(const SourceGroup& inDirectoryList, const std::string& inFolder) const;
	Ref<SourceGroup> getFiles(const SourceTarget& inProject) const;
	SourceGroup getDirectories(const SourceTarget& inProject) const;
	StringList getFileList(const SourceTarget& inProject) const;
	StringList getDirectoryList(const SourceTarget& inProject) const;

	const BuildState& m_state;

	const StringList m_resourceExts;
	const StringList m_objectiveCExts;
	const std::string m_objectiveCppExt;

	std::string m_buildOutputDir;
	std::string m_externalBuildDir;
	std::string m_objDir;
	std::string m_depDir;
	std::string m_asmDir;
	std::string m_intermediateDir;
	std::string m_intermediateDirWithPathSep;

	std::string m_parentCwd;

	bool m_initialized = false;
};
}
