/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "BuildJson/BuildEnvironment.hpp"

#include <thread>

#include "Libraries/FileSystem.hpp"
#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
BuildEnvironment::BuildEnvironment(const std::string& inBuildConfig) :
	m_buildConfiguration(inBuildConfig),
	m_processorCount(std::thread::hardware_concurrency())
{
	// LOG("Processor count: ", m_processorCount);
}

/*****************************************************************************/
uint BuildEnvironment::processorCount() const noexcept
{
	return m_processorCount;
}

/*****************************************************************************/
StrategyType BuildEnvironment::strategy() const noexcept
{
	return m_strategy;
}

void BuildEnvironment::setStrategy(const std::string& inValue) noexcept
{
	if (String::equals(inValue, "makefile"))
	{
		m_strategy = StrategyType::Makefile;
	}
	else if (String::equals(inValue, "native-experimental"))
	{
		m_strategy = StrategyType::Native;
	}
	else if (String::equals(inValue, "ninja-experimental"))
	{
		m_strategy = StrategyType::Ninja;
	}
	else
	{
		chalet_assert(false, "Invalid strategy type");
	}
}

/*****************************************************************************/
const std::string& BuildEnvironment::platform() const noexcept
{
	return m_platform;
}

void BuildEnvironment::setPlatform(const std::string& inValue) noexcept
{
	m_platform = inValue;
}

/*****************************************************************************/
const std::string& BuildEnvironment::modulePath() const noexcept
{
	return m_modulePath;
}

void BuildEnvironment::setModulePath(const std::string& inValue) noexcept
{
	if (inValue.empty())
		return;

	m_modulePath = inValue;

	if (m_modulePath.back() == '/')
		m_modulePath.pop_back();
}

/*****************************************************************************/
bool BuildEnvironment::showCommands() const noexcept
{
	return m_showCommands;
}

void BuildEnvironment::setShowCommands(const bool inValue) noexcept
{
	m_showCommands = inValue;
}

bool BuildEnvironment::cleanOutput() const noexcept
{
	return !m_showCommands;
}

/*****************************************************************************/
const StringList& BuildEnvironment::path() const noexcept
{
	return m_path;
}

void BuildEnvironment::addPaths(StringList& inList)
{
	List::forEach(inList, this, &BuildEnvironment::addPath);
}

void BuildEnvironment::addPath(std::string& inValue)
{
	if (inValue.back() == '/')
		inValue.pop_back();

	String::replaceAll(inValue, "${configuration}", m_buildConfiguration);
	Path::sanitize(inValue);
	List::addIfDoesNotExist(m_path, std::move(inValue));
}

/*****************************************************************************/
const std::string& BuildEnvironment::getPathString(const CompilerConfig& inCompilerConfig)
{
	if (m_pathString.empty())
	{
		StringList outList;

		m_originalPath = Environment::getPath();
		std::string compilerPathBin = inCompilerConfig.compilerPathBin();

		//
		if (!compilerPathBin.empty())
		{
#if defined(CHALET_WIN32)
			Path::sanitize(compilerPathBin);
#endif
			if (!String::contains(compilerPathBin, m_originalPath))
				outList.push_back(compilerPathBin);
		}

		//
		for (auto& p : m_path)
		{
			// TODO: function for this:
			if (!Commands::pathExists(p))
				continue;

			std::string path = Commands::getCanonicalPath(p);

			if (!String::contains(path, m_originalPath))
				outList.push_back(std::move(path));
		}

		StringList osPaths = getDefaultPaths();
		for (auto& p : osPaths)
		{
			if (!Commands::pathExists(p))
				continue;

			std::string path = Commands::getCanonicalPath(p);

			if (!String::contains(path, m_originalPath))
				outList.push_back(std::move(path));
		}

		//
		if (!m_originalPath.empty())
		{
#if defined(CHALET_WIN32)
			Path::sanitize(m_originalPath);
#endif
			outList.push_back(m_originalPath);
		}

		const std::string del = Path::getSeparator();

		m_pathString = String::join(outList, del);
		Path::sanitize(m_pathString);
	}

	return m_pathString;
}

/*****************************************************************************/
StringList BuildEnvironment::getDefaultPaths()
{
#if !defined(CHALET_WIN32)
	return {
		"/usr/local/sbin",
		"/usr/local/bin",
		"/usr/sbin",
		"/usr/bin",
		"/sbin",
		"/bin"
	};
#endif

	return {};
}

const std::string& BuildEnvironment::originalPath() const noexcept
{
	return m_originalPath;
}

/*****************************************************************************/
void BuildEnvironment::setPathVariable(const CompilerConfig& inCompilerConfig)
{
	if (m_lastLanguage == inCompilerConfig.language())
		return;

	Environment::set("PATH", getPathString(inCompilerConfig));
	// LOG(Environment::getPath());

	m_lastLanguage = inCompilerConfig.language();
}

}