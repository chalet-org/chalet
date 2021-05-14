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
	m_processorCount(std::thread::hardware_concurrency()),
	m_maxJobs(m_processorCount)
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
	if (String::equals("makefile", inValue))
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
const std::string& BuildEnvironment::externalDepDir() const noexcept
{
	return m_externalDepDir;
}

void BuildEnvironment::setExternalDepDir(const std::string& inValue) noexcept
{
	if (inValue.empty())
		return;

	m_externalDepDir = inValue;

	if (m_externalDepDir.back() == '/')
		m_externalDepDir.pop_back();
}

/*****************************************************************************/
uint BuildEnvironment::maxJobs() const noexcept
{
	return m_maxJobs;
}

void BuildEnvironment::setMaxJobs(const uint inValue) noexcept
{
	m_maxJobs = std::min(inValue, processorCount());
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
	String::replaceAll(inValue, "${externalDepDir}", m_externalDepDir);
	Path::sanitize(inValue);
	List::addIfDoesNotExist(m_path, std::move(inValue));
}

/*****************************************************************************/
std::string BuildEnvironment::makePathVariable(const std::string& inRootPath)
{
	auto separator = Path::getSeparator();

	StringList outList;

	for (auto& p : m_path)
	{
		if (!Commands::pathExists(p))
			continue;

		auto path = Commands::getCanonicalPath(p); // for any relative paths

		if (!String::contains(path, inRootPath))
			outList.push_back(std::move(path));
	}

	outList.push_back(inRootPath);

	std::string ret = String::join(outList, separator);
	Path::sanitize(ret);

	return ret;
}

}