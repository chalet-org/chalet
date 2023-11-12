/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "BuildEnvironment/Script/EmscriptenEnvironmentScript.hpp"

#include "Core/CommandLineInputs.hpp"
#include "Process/Environment.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildState.hpp"
#include "Terminal/Files.hpp"
#include "Utility/Path.hpp"
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
		Path::toUnix(m_emsdkEnv);
#else
		m_emsdkEnv = fmt::format("{}/emsdk_env.sh", emsdkRoot);
#endif
		if (!Files::pathExists(m_emsdkEnv))
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
	Path::toWindows(emsdkRoot);
	Path::toWindows(upstream);
	Path::toWindows(upstreamBin);
#else
	Path::toUnix(emsdkRoot);
	Path::toUnix(upstream);
	Path::toUnix(upstreamBin);
#endif

	auto nodePath = Files::getFirstChildDirectory(fmt::format("{}/node", emsdkRoot));
	if (!nodePath.empty())
	{
		Path::toUnix(nodePath);
		nodePath += "/bin/node";
#if defined(CHALET_WIN32)
		nodePath += ".exe";
#endif
	}
	else
	{
		nodePath = Files::which("node");
		if (nodePath.empty())
		{
			Diagnostic::error("node could not be found.");
			return false;
		}
	}

	auto pythonPath = Files::getFirstChildDirectory(fmt::format("{}/python", emsdkRoot));
	if (!pythonPath.empty())
	{
		Path::toUnix(pythonPath);

#if defined(CHALET_WIN32)
		pythonPath += "/python.exe";
#else
		pythonPath += "/bin/python3";
#endif
	}
	else
	{
		pythonPath = Files::which("python3");
		if (pythonPath.empty())
		{
			Diagnostic::error("python could not be found.");
			return false;
		}
	}

	std::string javaPath;
	auto javaHome = Files::getFirstChildDirectory(fmt::format("{}/java", emsdkRoot));
	if (!javaHome.empty())
	{
		Path::toUnix(javaHome);

		javaPath += "/bin/java";
#if defined(CHALET_WIN32)
		javaPath += ".exe";
#endif
	}
	else
	{
		javaPath = Files::which("java");
		if (javaPath.empty())
			javaPath = "java";
	}

	Path::toUnix(nodePath);
	Path::toUnix(pythonPath);
	Path::toUnix(javaPath);

	auto emrunPort = Environment::getString("EMRUN_PORT");
	if (emrunPort.empty())
	{
		emrunPort = "6931"; // default
	}

	fileContents += fmt::format("{pathKey}={emsdkRoot}{sep}{upstream}{sep}{upstreamBin}\n",
		FMT_ARG(pathKey),
		FMT_ARG(sep),
		FMT_ARG(emsdkRoot),
		FMT_ARG(upstream),
		FMT_ARG(upstreamBin));

#if defined(CHALET_WIN32)
	Path::toUnix(upstream);
	Path::toUnix(upstreamBin);
#endif

	fileContents += fmt::format("EMSDK_UPSTREAM_EMSCRIPTEN={}\n", upstream);
	fileContents += fmt::format("EMSDK_UPSTREAM_BIN={}\n", upstreamBin);

	fileContents += fmt::format("EMSDK_NODE={}\n", nodePath);
	fileContents += fmt::format("EMSDK_PYTHON={}\n", pythonPath);
	fileContents += fmt::format("JAVA_HOME={}\n", javaHome);
	fileContents += fmt::format("EMSDK_JAVA={}\n", javaPath);
	fileContents += fmt::format("EMRUN_PORT={}\n", emrunPort);

	// TODO: .emscripten file?
	//   https://emscripten.org/docs/tools_reference/emsdk.html#emscripten-compiler-configuration-file-emscripten
	//   Would set EM_CONFIG

	fileContents.pop_back();
	return Files::createFileWithContents(m_envVarsFileDelta, fileContents);
}

/*****************************************************************************/
StringList EmscriptenEnvironmentScript::getAllowedArchitectures()
{
	return {
		"wasm32"
	};
}
}
