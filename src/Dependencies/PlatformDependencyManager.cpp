/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Dependencies/PlatformDependencyManager.hpp"

#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"
#include "Utility/String.hpp"
#include "Json/JsonKeys.hpp"

namespace chalet
{
/*****************************************************************************/
PlatformDependencyManager::PlatformDependencyManager(const BuildState& inState) :
	m_state(inState)
{
	UNUSED(m_state);
}

/*****************************************************************************/
void PlatformDependencyManager::addRequiredPlatformDependency(const std::string& inKind, std::string&& inValue)
{
	if (m_platformRequires.find(inKind) == m_platformRequires.end())
		m_platformRequires.emplace(inKind, StringList{ std::move(inValue) });
	else
		m_platformRequires.at(inKind).emplace_back(std::move(inValue));
}

/*****************************************************************************/
void PlatformDependencyManager::addRequiredPlatformDependency(const std::string& inKind, StringList&& inValue)
{
	if (m_platformRequires.find(inKind) == m_platformRequires.end())
		m_platformRequires.emplace(inKind, std::move(inValue));
	else
		m_platformRequires.at(inKind) = std::move(inValue);
}

/*****************************************************************************/
bool PlatformDependencyManager::hasRequired()
{
	if (m_platformRequires.empty())
		return true;

	static const char prefix[] = "Verifying required ";
	static const char suffix[] = " packages";

	StringList errors;

	for (auto& [key, list] : m_platformRequires)
	{
#if defined(CHALET_WINDOWS)
		if (String::equals(Keys::ReqWindowsMSYS2, key))
		{
			LOG("msys2", '-', String::join(list, ','));
		}
#elif defined(CHALET_MACOS)
		if (String::equals(Keys::ReqMacOSHomebrew, key))
		{
			Diagnostic::info("{}Homebrew{}", prefix, suffix);
			for (auto& item : list)
			{
				Diagnostic::infoEllipsis("{}", item);
				// LOG("homebrew", '-', String::join(list, ','));

				auto path = fmt::format("/usr/local/Cellar/{}", item);
				bool exists = Commands::pathExists(path) && Commands::pathIsDirectory(path);

				if (!exists)
				{
					errors.push_back(fmt::format("Homebrew dependency '{}' was not found.", item));
				}

				Diagnostic::printDone();
			}
		}
		else if (String::equals(Keys::ReqMacOSMacPorts, key))
		{
			LOG("macports", '-', String::join(list, ','));
		}
#else
		if (String::equals(Keys::ReqUbuntuSystem, key))
		{
			LOG("ubuntu", '-', String::join(list, ','));
		}
#endif
	}

	if (!errors.empty())
	{
		for (auto& error : errors)
		{
			Diagnostic::error(error);
		}
		return false;
	}

	m_platformRequires.clear();

	return true;
}
}
