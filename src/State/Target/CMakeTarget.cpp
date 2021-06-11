/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Target/CMakeTarget.hpp"

#include "Libraries/Format.hpp"
#include "State/BuildState.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/List.hpp"

namespace chalet
{
/*****************************************************************************/
CMakeTarget::CMakeTarget(const BuildState& inState) :
	IBuildTarget(inState, BuildTargetType::CMake),
	m_defines(getDefaultCmakeDefines())
{
}

/*****************************************************************************/
void CMakeTarget::initialize()
{
	const auto& targetName = this->name();
	m_state.paths.replaceVariablesInPath(m_buildFile, targetName);
	m_state.paths.replaceVariablesInPath(m_location, targetName);
}

/*****************************************************************************/
bool CMakeTarget::validate()
{
	const auto& targetName = this->name();

	bool result = true;
	if (!Commands::pathExists(m_location))
	{
		Diagnostic::error(fmt::format("location for CMake target '{}' doesn't exist: {}", targetName, m_location));
		result = false;
	}

	if (!m_buildFile.empty() && !Commands::pathExists(fmt::format("{}/{}", m_location, m_buildFile)))
	{
		Diagnostic::error(fmt::format("buildFile '{}' for CMake target '{}' was not found in the location: {}", m_buildFile, targetName, m_location));
		result = false;
	}

	if (!m_state.tools.cmakeAvailable())
	{
		Diagnostic::error(fmt::format("CMake was requsted for the project '{}' but was not found.", this->name()));
		result = false;
	}

	std::string buildConfiguration = m_state.info.buildConfiguration();
	if (m_state.configuration.enableProfiling())
	{
		buildConfiguration = "Debug";
	}

	if (!List::contains({ "Release", "Debug", "RelWithDebInfo", "MinSizeRel" }, buildConfiguration))
	{
		// https://cmake.org/cmake/help/v3.0/variable/CMAKE_BUILD_TYPE.html
		Diagnostic::error(fmt::format("Build '{}' not recognized by CMake.", buildConfiguration));
		result = false;
	}

	return result;
}

/*****************************************************************************/
const StringList& CMakeTarget::defines() const noexcept
{
	return m_defines;
}

void CMakeTarget::addDefines(StringList&& inList)
{
	List::forEach(inList, this, &CMakeTarget::addDefine);
}

void CMakeTarget::addDefine(std::string&& inValue)
{
	List::addIfDoesNotExist(m_defines, std::move(inValue));
}

/*****************************************************************************/
const std::string& CMakeTarget::buildFile() const noexcept
{
	return m_buildFile;
}

void CMakeTarget::setBuildFile(std::string&& inValue) noexcept
{
	m_buildFile = std::move(inValue);
}

/*****************************************************************************/
const std::string& CMakeTarget::toolset() const noexcept
{
	return m_toolset;
}
void CMakeTarget::setToolset(std::string&& inValue) noexcept
{
	m_toolset = std::move(inValue);
}

/*****************************************************************************/
const std::string& CMakeTarget::location() const noexcept
{
	return m_location;
}

void CMakeTarget::setLocation(std::string&& inValue) noexcept
{
	m_location = std::move(inValue);
}

/*****************************************************************************/
bool CMakeTarget::recheck() const noexcept
{
	return m_recheck;
}

void CMakeTarget::setRecheck(const bool inValue) noexcept
{
	m_recheck = inValue;
}

/*****************************************************************************/
StringList CMakeTarget::getDefaultCmakeDefines()
{
	// TODO: Only if using bash ... this define might not be needed at all
	return { "CMAKE_SH=\"CMAKE_SH-NOTFOUND\"" };
}

}
