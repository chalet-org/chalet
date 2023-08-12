/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Core/DotEnvFileGenerator.hpp"

#include "Compile/Environment/ICompileEnvironment.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Utility/List.hpp"

namespace chalet
{
/*****************************************************************************/
DotEnvFileGenerator DotEnvFileGenerator::make(const BuildState& inState)
{
	DotEnvFileGenerator env;

	auto addEnvironmentPath = [&inState, &env](const char* inKey, const StringList& inAdditionalPaths = StringList()) {
		auto path = Environment::getString(inKey);
		auto outPath = inState.workspace.makePathVariable(path, inAdditionalPaths);

		// if (!String::equals(outPath, path))
		env.set(inKey, outPath);
	};

	StringList libDirs;
	StringList frameworks;
	for (auto& target : inState.targets)
	{
		if (target->isSources())
		{
			auto& project = static_cast<SourceTarget&>(*target);
			for (auto& p : project.libDirs())
			{
				List::addIfDoesNotExist(libDirs, p);
			}
			for (auto& p : project.appleFrameworkPaths())
			{
				List::addIfDoesNotExist(frameworks, p);
			}
		}
	}

	std::string rootPath;

#if defined(CHALET_LINUX)
	const auto& sysroot = inState.environment->sysroot();
	if (!sysroot.empty())
	{
		rootPath = inState.environment->sysroot();
	}
#endif

	StringList allPaths = List::combine(libDirs, frameworks, rootPath);
	addEnvironmentPath("PATH", allPaths);

	env.setRunPaths(inState.workspace.makePathVariable(std::string(), allPaths));

#if defined(CHALET_LINUX)
	// Linux uses LD_LIBRARY_PATH to resolve the correct file dependencies at runtime
	addEnvironmentPath("LD_LIBRARY_PATH", libDirs);
// addEnvironmentPath("LIBRARY_PATH"); // only used by gcc / ld
#elif defined(CHALET_MACOS)
	addEnvironmentPath("DYLD_FALLBACK_LIBRARY_PATH", libDirs);
	addEnvironmentPath("DYLD_FALLBACK_FRAMEWORK_PATH", frameworks);
#endif

	return env;
}

/*****************************************************************************/
void DotEnvFileGenerator::set(const std::string& inKey, const std::string& inValue)
{
	m_variables[inKey] = inValue;
}

void DotEnvFileGenerator::setRunPaths(const std::string& inValue)
{
	set("__CHALET_RUN_PATHS", inValue);
}

/*****************************************************************************/
std::string DotEnvFileGenerator::get(const std::string& inKey) const
{
	if (m_variables.find(inKey) != m_variables.end())
		return m_variables.at(inKey);

	return std::string();
}

std::string DotEnvFileGenerator::getRunPaths() const
{
	return get("__CHALET_RUN_PATHS");
}

/*****************************************************************************/
bool DotEnvFileGenerator::save(const std::string& inFilename)
{
	if (inFilename.empty())
		return false;

	std::string contents;
	for (auto& [name, var] : m_variables)
	{
		contents += fmt::format("{}={}\n", name, var);
	}

	std::ofstream(inFilename) << contents;

	return true;
}
}
