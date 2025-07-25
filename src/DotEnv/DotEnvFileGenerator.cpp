/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "DotEnv/DotEnvFileGenerator.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Process/Environment.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "System/Files.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

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
				List::addIfDoesNotExist(libDirs, p);

			for (auto& p : project.appleFrameworkPaths())
				List::addIfDoesNotExist(frameworks, p);
		}
	}

	const auto& sysroot = inState.environment->sysroot();

	StringList allPaths = List::combineRemoveDuplicates(libDirs, frameworks, sysroot);
	addEnvironmentPath("PATH", allPaths);

	env.setRunPaths(inState.workspace.makePathVariable(std::string(), allPaths));

#if defined(CHALET_LINUX)
	// Linux uses LD_LIBRARY_PATH to resolve the correct file dependencies at runtime
	addEnvironmentPath(Environment::getLibraryPathKey(), libDirs);
// addEnvironmentPath("LIBRARY_PATH"); // only used by gcc / ld
#elif defined(CHALET_MACOS)
	addEnvironmentPath(Environment::getLibraryPathKey(), libDirs);
	addEnvironmentPath(Environment::getFrameworkPathKey(), frameworks);
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

/*****************************************************************************/
std::string DotEnvFileGenerator::getPath() const
{
	return get("PATH");
}

std::string DotEnvFileGenerator::getRunPaths() const
{
	return get("__CHALET_RUN_PATHS");
}

/*****************************************************************************/
std::string DotEnvFileGenerator::getLibraryPath() const
{
#if defined(CHALET_LINUX) || defined(CHALET_MACOS)
	return get(Environment::getLibraryPathKey());
#else
	return std::string();
#endif
}

/*****************************************************************************/
std::string DotEnvFileGenerator::getFrameworkPath() const
{
#if defined(CHALET_MACOS)
	return get(Environment::getFrameworkPathKey());
#else
	return std::string();
#endif
}

/*****************************************************************************/
bool DotEnvFileGenerator::save(const std::string& inFilename)
{
	if (inFilename.empty())
		return false;

	std::string contents;
	for (const auto& [name, var] : m_variables)
	{
		auto line = fmt::format("{}={}\n", name, var);
		String::replaceAll(line, ' ', "\\ ");
		contents += std::move(line);
	}

	Files::ofstream(inFilename) << contents;

	return true;
}
}
