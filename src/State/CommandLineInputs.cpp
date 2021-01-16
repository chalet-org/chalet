/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/CommandLineInputs.hpp"

#include "Builder/BuildManager.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
std::string CommandLineInputs::kFile = "build.json";

/*****************************************************************************/
CommandLineInputs::CommandLineInputs() :
	m_platform(getPlatform())
{
}

/*****************************************************************************/
const std::string& CommandLineInputs::file() noexcept
{
	return kFile;
}

void CommandLineInputs::setFile(std::string&& inValue) noexcept
{
	kFile = std::move(inValue);
}

/*****************************************************************************/
Route CommandLineInputs::command() const noexcept
{
	return m_command;
}
void CommandLineInputs::setCommand(const Route inValue) noexcept
{
	m_command = inValue;
}

/*****************************************************************************/
const std::string& CommandLineInputs::buildConfiguration() const noexcept
{
	return m_buildConfiguration;
}

void CommandLineInputs::setBuildConfiguration(const std::string& inValue) noexcept
{
	m_buildConfiguration = inValue;
}

/*****************************************************************************/
const std::string& CommandLineInputs::buildFromCommandLine() const noexcept
{
	return m_buildFromCommandLine;
}
void CommandLineInputs::setBuildFromCommandLine(std::string&& inValue) noexcept
{
	m_buildFromCommandLine = std::move(inValue);

	setBuildConfiguration(m_buildFromCommandLine);
}

/*****************************************************************************/
const std::string& CommandLineInputs::platform() const noexcept
{
	return m_platform;
}

/*****************************************************************************/
const std::string& CommandLineInputs::runProject() const noexcept
{
	return m_runProject;
}

void CommandLineInputs::setRunProject(std::string&& inValue) noexcept
{
	m_runProject = std::move(inValue);
}

/*****************************************************************************/
const std::string& CommandLineInputs::runOptions() const noexcept
{
	return m_runOptions;
}

void CommandLineInputs::setRunOptions(std::string&& inValue) noexcept
{
	m_runOptions = std::move(inValue);

	if (m_runOptions.front() == '\'' && m_runOptions.back() == '\'')
		m_runOptions = m_runOptions.substr(1, m_runOptions.size() - 2);
}

/*****************************************************************************/
const std::string& CommandLineInputs::appPath() const noexcept
{
	return m_appPath;
}

void CommandLineInputs::setAppPath(const std::string& inValue) noexcept
{
	m_appPath = inValue;
}

/*****************************************************************************/
const std::string& CommandLineInputs::initProjectName() const noexcept
{
	return m_initProjectName;
}

void CommandLineInputs::setInitProjectName(std::string&& inValue) noexcept
{
	m_initProjectName = std::move(inValue);
}

/*****************************************************************************/
const std::string& CommandLineInputs::initPath() const noexcept
{
	return m_initPath;
}

void CommandLineInputs::setInitPath(std::string&& inValue) noexcept
{
	m_initPath = std::move(inValue);
}

/*****************************************************************************/
std::string CommandLineInputs::getPlatform() noexcept
{
#if defined(CHALET_WIN32)
	return "windows";
#elif defined(CHALET_MACOS)
	return "macos";
#elif defined(CHALET_LINUX)
	return "linux";
#else
	return "unknown";
#endif
}
}
