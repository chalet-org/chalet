/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Target/CMakeTarget.hpp"

#include "State/BuildState.hpp"
#include "Terminal/Path.hpp"
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
	m_state.paths.parsePathWithVariables(m_buildScript, targetName);
	m_state.paths.parsePathWithVariables(m_location, targetName);
}

/*****************************************************************************/
const StringList& CMakeTarget::defines() const noexcept
{
	return m_defines;
}

void CMakeTarget::addDefines(StringList& inList)
{
	List::forEach(inList, this, &CMakeTarget::addDefine);
}

void CMakeTarget::addDefine(std::string& inValue)
{
	List::addIfDoesNotExist(m_defines, std::move(inValue));
}

/*****************************************************************************/
const std::string& CMakeTarget::buildScript() const noexcept
{
	return m_buildScript;
}

void CMakeTarget::setBuildScript(std::string&& inValue) noexcept
{
	m_buildScript = std::move(inValue);
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
