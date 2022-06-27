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
	config["name"] = "Debug";
	config["intelliSenseMode"] = "windows-msvc-x64";

	std::string cStandard;
	std::string cppStandard;
	StringList defines{
		"_WIN32"
	};
	StringList includePath{
		"${env.INCLUDE}",
	};
	StringList forcedInclude;

	bool hasProjects = false;
	for (auto& target : m_state.targets)
	{
		if (target->isSources())
		{
			auto& project = static_cast<const SourceTarget&>(*target);

			if (cStandard.empty())
				cStandard = project.cStandard();

			if (cppStandard.empty())
				cppStandard = project.cppStandard();

			if (project.usesPrecompiledHeader())
			{
				auto path = project.precompiledHeader();
				if (Commands::pathExists(fmt::format("{}/{}", m_cwd, path)))
				{
					path = fmt::format("${{workspaceRoot}}/{}", path);
				}
				List::addIfDoesNotExist(forcedInclude, path);
			}

			for (auto path : project.includeDirs())
			{
				if (path.back() == '/')
					path.pop_back();

				if (Commands::pathExists(fmt::format("{}/{}", m_cwd, path)) || String::equals(path, m_state.paths.intermediateDir(project)))
				{
					path = fmt::format("${{workspaceRoot}}/{}", path);
				}

				List::addIfDoesNotExist(includePath, path);
			}

			for (const auto& define : project.defines())
			{
				List::addIfDoesNotExist(defines, define);
			}

			if (String::equals(String::toLowerCase(project.executionCharset()), "utf-8"))
			{
				List::addIfDoesNotExist(defines, std::string("UNICODE"));
				List::addIfDoesNotExist(defines, std::string("_UNICODE"));
			}

			hasProjects = true;
		}
	}

	if (!hasProjects)
		return false;

	config["compilers"] = Json::object();

	auto& compilers = config.at("compilers");

	compilers["c"] = Json::object();
	compilers["c"]["path"] = m_state.toolchain.compilerC().path;
	compilers["c"]["standard"] = std::move(cStandard);

	compilers["cpp"] = Json::object();
	compilers["cpp"]["path"] = m_state.toolchain.compilerCpp().path;
	compilers["cpp"]["standard"] = std::move(cppStandard);

	config["defines"] = std::move(defines);
	config["forcedInclude"] = std::move(forcedInclude);
	config["includePath"] = std::move(includePath);
	config["environments"] = Json::array();

	configurations.push_back(std::move(config));

	return JsonFile::saveToFile(jRoot, inFilename, 1);
}

}
