/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_VS_VCXPROJ_GEN_HPP
#define CHALET_VS_VCXPROJ_GEN_HPP

#include "State/MSVCWarningLevel.hpp"
#include "State/SourceOutputs.hpp"
#include "State/Target/BuildTargetType.hpp"
#include "Utility/Uuid.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
class BuildState;
struct SourceTarget;
struct IBuildTarget;
struct ProjectAdapterVCXProj;
struct TargetAdapterVCXProj;
struct XmlFile;
class XmlElement;

struct VSVCXProjGen
{
	explicit VSVCXProjGen(const std::vector<Unique<BuildState>>& inStates, const std::string& inExportDir, const std::string& inProjectTypeGuid, const OrderedDictionary<Uuid>& inTargetGuids);
	CHALET_DISALLOW_COPY_MOVE(VSVCXProjGen);
	~VSVCXProjGen();

	bool saveSourceTargetProjectFiles(const std::string& inProjectName);
	bool saveScriptTargetProjectFiles(const std::string& inProjectName);
	bool saveAllBuildTargetProjectFiles(const std::string& inProjectName);

private:
	std::string makeSubDirectoryAndGetProjectFile(const std::string& inName) const;

	bool saveSourceTargetProjectFile(const std::string& inName, const std::string& inFilename, XmlFile& outFiltersFile);
	bool saveScriptTargetProjectFile(const std::string& inName, const std::string& inFilename, XmlFile& outFiltersFile);
	bool saveAllTargetProjectFile(const std::string& inName, const std::string& inFilename);

	bool saveFiltersFile(XmlFile& outFile, const BuildTargetType inType);
	bool saveUserFile(const std::string& inFilename);

	void addProjectHeader(XmlElement& outNode) const;
	void addProjectConfiguration(XmlElement& outNode) const;
	void addGlobalProperties(XmlElement& outNode, const BuildTargetType inType) const;
	void addMsCppDefaultProps(XmlElement& outNode) const;
	void addConfigurationProperties(XmlElement& outNode, const BuildTargetType inType) const;
	void addMsCppProps(XmlElement& outNode) const;
	void addExtensionSettings(XmlElement& outNode) const;
	void addShared(XmlElement& outNode) const;
	void addPropertySheets(XmlElement& outNode) const;
	void addUserMacros(XmlElement& outNode) const;
	void addGeneralProperties(XmlElement& outNode, const std::string& inName, const BuildTargetType inType) const;
	void addCompileProperties(XmlElement& outNode) const;
	void addScriptProperties(XmlElement& outNode) const;
	void addSourceFiles(XmlElement& outNode, const std::string& inName, XmlFile& outFiltersFile) const;
	void addTargetFiles(XmlElement& outNode, const std::string& inName, XmlFile& outFiltersFile) const;
	void addAllTargetFiles(XmlElement& outNode) const;
	void addProjectReferences(XmlElement& outNode, const std::string& inName) const;
	void addAllProjectReferences(XmlElement& outNode) const;
	void addImportMsCppTargets(XmlElement& outNode) const;
	void addExtensionTargets(XmlElement& outNode) const;

	const SourceTarget* getProjectFromStateContext(const BuildState& inState, const std::string& inName) const;
	const IBuildTarget* getTargetFromStateContext(const BuildState& inState, const std::string& inName) const;
	std::string getWindowsTargetPlatformVersion() const;
	std::string getVisualStudioVersion() const;
	std::string getCondition(const std::string& inConfig) const;

	std::string getResolvedInputFile() const;

	const std::vector<Unique<BuildState>>& m_states;
	const std::string& m_exportDir;
	const std::string& m_projectTypeGuid;
	const OrderedDictionary<Uuid>& m_targetGuids;

	HeapDictionary<ProjectAdapterVCXProj> m_adapters;
	HeapDictionary<TargetAdapterVCXProj> m_targetAdapters;
	HeapDictionary<SourceOutputs> m_outputs;

	std::string m_currentTarget;
	std::string m_currentGuid;
};
}

#endif // CHALET_VS_VCXPROJ_GEN_HPP
