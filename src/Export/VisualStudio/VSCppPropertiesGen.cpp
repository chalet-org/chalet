/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/VisualStudio/VSCppPropertiesGen.hpp"

#include "Compile/Environment/ICompileEnvironment.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Json/JsonFile.hpp"

// Reference: https://docs.microsoft.com/en-us/cpp/build/cppproperties-schema-reference?view=msvc-170

namespace chalet
{
/*****************************************************************************/
VSCppPropertiesGen::VSCppPropertiesGen(const BuildState& inState, const std::string& inCwd) :
	m_state(inState),
	m_cwd(inCwd)
{
	UNUSED(m_state, m_cwd);
}

/*****************************************************************************/
bool VSCppPropertiesGen::saveToFile(const std::string& inFilename) const
{
	Json jRoot;
	jRoot = Json::object();
	jRoot["configurations"] = Json::array();

	auto& configurations = jRoot.at("configurations");

	Json config;
	config["inheritEnvironments"] = {
		"msvc_x64_x64"
	};
	config["name"] = "ChaletExport";
	config["includePath"] = {
		"${env.INCLUDE}",
		"${workspaceRoot}\\**",
	};
	config["intelliSenseMode"] = "windows-msvc-x64";
	config["environments"] = Json::array();

	configurations.push_back(std::move(config));

	return JsonFile::saveToFile(jRoot, inFilename, 1);
}

}
