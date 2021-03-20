/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMMAND_LINE_INPUTS_HPP
#define CHALET_COMMAND_LINE_INPUTS_HPP

#include "Generator/IdeType.hpp"
#include "Router/Route.hpp"

namespace chalet
{
struct CommandLineInputs
{
	CommandLineInputs();

	static const std::string& file() noexcept;
	static void setFile(std::string&& inValue) noexcept;

	Route command() const noexcept;
	void setCommand(const Route inValue) noexcept;

	const std::string& buildConfiguration() const noexcept;
	void setBuildConfiguration(const std::string& inValue) noexcept;

	const std::string& buildFromCommandLine() const noexcept;
	void setBuildFromCommandLine(std::string&& inValue) noexcept;

	const std::string& platform() const noexcept;
	const StringList& notPlatforms() const noexcept;

	const std::string& runProject() const noexcept;
	void setRunProject(std::string&& inValue) noexcept;

	const StringList& runOptions() const noexcept;
	void setRunOptions(std::string&& inValue) noexcept;

	const std::string& appPath() const noexcept;
	void setAppPath(const std::string& inValue) noexcept;

	IdeType generator() const noexcept;
	const std::string& generatorRaw() const noexcept;
	void setGenerator(std::string&& inValue) noexcept;

	const std::string& initProjectName() const noexcept;
	void setInitProjectName(std::string&& inValue) noexcept;

	const std::string& initPath() const noexcept;
	void setInitPath(std::string&& inValue) noexcept;

	bool saveSchemaToFile() const noexcept;
	void setSaveSchemaToFile(const bool inValue) noexcept;

private:
	static std::string kFile;

	std::string getPlatform() noexcept;
	StringList getNotPlatforms() noexcept;

	IdeType getIdeTypeFromString(const std::string& inValue);

	StringList m_runOptions;
	StringList m_notPlatforms;

	std::string m_buildConfiguration;
	std::string m_buildFromCommandLine;
	std::string m_platform;
	std::string m_runProject;
	std::string m_appPath;
	std::string m_generatorRaw;

	std::string m_initProjectName;
	std::string m_initPath;

	Route m_command = Route::Unknown;
	IdeType m_generator = IdeType::None;

	bool m_saveSchemaToFile = false;
};
}

#endif // CHALET_COMMAND_LINE_INPUTS_HPP
