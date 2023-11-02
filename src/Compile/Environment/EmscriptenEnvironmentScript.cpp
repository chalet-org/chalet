/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Environment/EmscriptenEnvironmentScript.hpp"

#include "Core/CommandLineInputs.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildState.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Path.hpp"
#include "Utility/String.hpp"

namespace chalet
{
EmscriptenEnvironmentScript::EmscriptenEnvironmentScript() = default;

/*****************************************************************************/
bool EmscriptenEnvironmentScript::isPreset() noexcept
{
	return true;
}

/*****************************************************************************/
bool EmscriptenEnvironmentScript::makeEnvironment(const BuildState& inState)
{
	UNUSED(inState);

	m_pathVariable = Environment::getPath();

	if (!m_envVarsFileDeltaExists)
	{
		auto emsdkRoot = Environment::getString("EMSDK");
		if (emsdkRoot.empty())
		{
			Diagnostic::error("No suitable Emscripten compiler installation found. Please install Emscripten and set the 'EMSDK' variable before continuing.");
			return false;
		}

		if (emsdkRoot.back() == '/' || emsdkRoot.back() == '\\')
			emsdkRoot.pop_back();

#if defined(CHALET_WIN32)
		m_emsdkEnv = fmt::format("{}/emsdk_env.bat", emsdkRoot);
		Path::sanitize(m_emsdkEnv);
#else
		m_emsdkEnv = fmt::format("{}/emsdk_env.sh", emsdkRoot);
#endif
		if (!Commands::pathExists(m_emsdkEnv))
		{
			Diagnostic::error("No suitable Emscripten compiler installation found. Please install Emscripten and set the 'EMSDK' variable before continuing.");
			return false;
		}

		if (!saveEnvironmentFromScript())
		{
			Diagnostic::error("Emscripten environment could not be fetched: The expected method returned with error.");
			return false;
		}
	}

	return true;
}

/*****************************************************************************/
void EmscriptenEnvironmentScript::readEnvironmentVariablesFromDeltaFile()
{
	Dictionary<std::string> variables;

	Environment::readEnvFileToDictionary(m_envVarsFileDelta, variables);

	const auto pathKey = Environment::getPathKey();
	const char pathSep = Environment::getPathSeparator();
	for (auto& [name, var] : variables)
	{
		if (String::equals(pathKey, name))
		{
			Environment::set(name.c_str(), fmt::format("{}{}{}", m_pathVariable, pathSep, var));
		}
		else
		{
			Environment::set(name.c_str(), var);
		}
	}
}

/*****************************************************************************/
bool EmscriptenEnvironmentScript::saveEnvironmentFromScript()
{
	std::string fileContents;
	auto pathKey = Environment::getPathKey();
	auto sep = Environment::getPathSeparator();

	auto emsdkRoot = Environment::getString("EMSDK");
	auto upstream = fmt::format("{}/upstream/emscripten", emsdkRoot);
	auto upstreamBin = fmt::format("{}/upstream/bin", emsdkRoot);
#if defined(CHALET_WIN32)
	Path::sanitizeForWindows(emsdkRoot);
	Path::sanitizeForWindows(upstream);
	Path::sanitizeForWindows(upstreamBin);
#else
	Path::sanitize(emsdkRoot);
	Path::sanitize(upstream);
	Path::sanitize(upstreamBin);
#endif

	fileContents += fmt::format("{pathKey}={emsdkRoot}{sep}{upstream}{sep}{upstreamBin}\n",
		FMT_ARG(pathKey),
		FMT_ARG(sep),
		FMT_ARG(emsdkRoot),
		FMT_ARG(upstream),
		FMT_ARG(upstreamBin));

	auto nodePath = Commands::getFirstChildDirectory(fmt::format("{}/node", emsdkRoot));
	if (!nodePath.empty())
	{
		Path::sanitize(nodePath);
		nodePath += "/bin/node.exe";

		fileContents += fmt::format("EMSDK_NODE={}\n", nodePath);
	}

	auto pythonPath = Commands::getFirstChildDirectory(fmt::format("{}/python", emsdkRoot));
	if (!pythonPath.empty())
	{
		Path::sanitize(pythonPath);
		pythonPath += "/python.exe";

		fileContents += fmt::format("EMSDK_PYTHON={}\n", pythonPath);
	}

	auto javaPath = Commands::getFirstChildDirectory(fmt::format("{}/java", emsdkRoot));
	if (!javaPath.empty())
	{
		Path::sanitize(javaPath);
		fileContents += fmt::format("JAVA_HOME={}\n", javaPath);
	}

	m_envVarsFileAfter;

	fileContents.pop_back();
	return Commands::createFileWithContents(m_envVarsFileDelta, fileContents);
}

/*****************************************************************************/
StringList EmscriptenEnvironmentScript::getAllowedArchitectures()
{
	return {
		"wasm32"
	};
}
}
