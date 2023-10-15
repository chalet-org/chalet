/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_CODEBLOCKS_CBP_GEN_HPP
#define CHALET_CODEBLOCKS_CBP_GEN_HPP

#include "Utility/Uuid.hpp"
#include "Xml/XmlFile.hpp"

namespace chalet
{
class BuildState;
struct IBuildTarget;
struct SourceTarget;
struct CompileToolchainController;

struct CodeBlocksCBPGen
{
	explicit CodeBlocksCBPGen(const std::vector<Unique<BuildState>>& inStates, const std::string& inAllBuildName);

	bool saveProjectFiles(const std::string& inDirectory);

private:
	enum class TargetGroupKind : ushort
	{
		Source,
		Script,
		BuildAll,
	};
	struct TargetGroup
	{
		// std::string path;
		// std::string outputFile;
		std::string pch;
		std::map<std::string, StringList> sources;
		StringList scripts;
		StringList dependencies;
		TargetGroupKind kind = TargetGroupKind::Script;
	};

	bool initialize();

	bool saveSourceTargetProjectFiles(const std::string& inFilename, const std::string& inName, const TargetGroup& inGroup);

	void addBuildConfigurationForTarget(XmlElement& outNode, const std::string& inName, const std::string& inConfigName) const;

	void addSourceTarget(XmlElement& outNode, const BuildState& inState, const SourceTarget& inTarget, const CompileToolchainController& inToolchain) const;
	void addSourceCompilerOptions(XmlElement& outNode, const BuildState& inState, const SourceTarget& inTarget, const CompileToolchainController& inToolchain) const;
	void addSourceLinkerOptions(XmlElement& outNode, const BuildState& inState, const SourceTarget& inTarget, const CompileToolchainController& inToolchain) const;
	void addScriptTarget(XmlElement& outNode, const BuildState& inState, const std::string& inName) const;
	void addAllBuildTarget(XmlElement& outNode, const BuildState& inState) const;
	std::string getOutputType(const SourceTarget& inTarget) const;
	std::string getResolvedPath(const std::string& inFile) const;
	std::string getResolvedLibraryPath(const std::string& inFile) const;
	std::string getResolvedObjDir(const BuildState& inState) const;
	std::string getVirtualFolder(const std::string& inFile, const std::string& inPch) const;

	const std::vector<Unique<BuildState>>& m_states;
	const std::string& m_allBuildName;

	const StringList m_headerExtensions;
	StringList m_resourceExtensions;

	std::map<std::string, TargetGroup> m_groups;
	std::map<std::string, std::vector<const IBuildTarget*>> m_configToTargets;

	std::string m_compiler;
	std::string m_cwd;
	std::string m_exportPath;
};
}

#endif // CHALET_CODEBLOCKS_CBP_GEN_HPP
