/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/ModuleStrategy/ModuleStrategyGCC.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Process/Environment.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "System/Files.hpp"
#include "Utility/List.hpp"
#include "Utility/Path.hpp"
#include "Utility/String.hpp"
#include "Json/JsonComments.hpp"
#include "Json/JsonKeys.hpp"

namespace chalet
{
/*****************************************************************************/
ModuleStrategyGCC::ModuleStrategyGCC(BuildState& inState, CompileCommandsGenerator& inCompileCommandsGenerator) :
	IModuleStrategy(inState, inCompileCommandsGenerator)
{
}

/*****************************************************************************/
bool ModuleStrategyGCC::initialize()
{
	const auto& systemIncludeDir = m_state.toolchain.compilerCxxAny().includeDir;
	u32 versionMajor = m_state.toolchain.versionMajor();
	auto cppFolder = fmt::format("{}/c++", systemIncludeDir);
	if (Files::pathIsSymLink(cppFolder))
	{
		cppFolder = fmt::format("{}/{}", systemIncludeDir, Files::resolveSymlink(cppFolder));
	}

	cppFolder = Files::getCanonicalPath(fmt::format("{}/{}", cppFolder, versionMajor));
	if (!Files::pathExists(cppFolder))
	{
		Diagnostic::error("Could not resolve system include directory");
		return false;
	}

	m_systemHeaderDirectory = std::move(cppFolder);

	return true;
}

/*****************************************************************************/
bool ModuleStrategyGCC::isSystemModuleFile(const std::string& inFile) const
{
	LOG(inFile);
	return true;
}

/*****************************************************************************/
bool ModuleStrategyGCC::readModuleDependencies(const SourceOutputs& inOutputs, Dictionary<ModuleLookup>& outModules)
{
	UNUSED(inOutputs, outModules);
	LOG("readModuleDependencies");

	StringList systemHeaders{
		"iostream",
		"cstdlib"
	};
	for (auto& group : inOutputs.groups)
	{
		if (group->type != SourceType::CPlusPlus)
			continue;

		auto name = String::getPathBaseName(group->sourceFile);
		LOG(name);
		outModules[name].source = group->sourceFile;
		outModules[name].importedHeaderUnits = systemHeaders;
	}

	for (auto& systemModule : systemHeaders)
	{
		outModules[systemModule].source = fmt::format("{}/{}", m_systemHeaderDirectory, systemModule);
	}

	return true;
}

/*****************************************************************************/
bool ModuleStrategyGCC::readIncludesFromDependencyFile(const std::string& inFile, StringList& outList)
{
	LOG("readIncludesFromDependencyFile:", inFile);

	UNUSED(outList);

	return true;
}

/*****************************************************************************/
std::string ModuleStrategyGCC::getBuildOutputForFile(const SourceFileGroup& inFile, const bool inIsObject) const
{
	std::string ret = inIsObject ? inFile.sourceFile : inFile.dependencyFile;

	return ret;
}

/*****************************************************************************/
Dictionary<std::string> ModuleStrategyGCC::getSystemModules() const
{
	Dictionary<std::string> ret;

	return ret;
}

}
