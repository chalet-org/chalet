/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Dependencies/PlatformDependencyManager.hpp"

#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"
#include "Utility/List.hpp"
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
		m_platformRequires.emplace(inKind, StringList{});

	auto& list = m_platformRequires.at(inKind);
	List::addIfDoesNotExist(list, std::move(inValue));
}

/*****************************************************************************/
void PlatformDependencyManager::addRequiredPlatformDependency(const std::string& inKind, StringList&& inValue)
{
	if (m_platformRequires.find(inKind) == m_platformRequires.end())
		m_platformRequires.emplace(inKind, StringList{});

	auto& list = m_platformRequires.at(inKind);
	for (auto&& item : inValue)
	{
		List::addIfDoesNotExist(list, std::move(item));
	}
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
				if (item.empty())
					continue;

				Diagnostic::subInfoEllipsis("{}", item);

				auto path = fmt::format("/usr/local/Cellar/{}", item);
				bool exists = Commands::pathExists(path) && Commands::pathIsDirectory(path);
				Diagnostic::printFound(exists);

				if (!exists)
				{
					errors.push_back(fmt::format("Homebrew dependency '{}' was not found.", item));
				}
			}
		}
		else if (String::equals(Keys::ReqMacOSMacPorts, key))
		{
			Diagnostic::info("{}MacPorts{}", prefix, suffix);

			auto port = Commands::which("port");
			if (!port.empty())
			{
				auto installed = Commands::subprocessOutput({ port, "installed" });
				if (!installed.empty())
				{
					for (auto& item : list)
					{
						if (item.empty())
							continue;

						Diagnostic::subInfoEllipsis("{}", item);

						auto find = fmt::format("\n  {} ", item);

						bool exists = String::contains(find, installed);
						Diagnostic::printFound(exists);

						if (!exists)
						{
							errors.push_back(fmt::format("MacPorts dependency '{}' was not found.", item));
						}
					}
				}
			}
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
		// for (auto it = errors.rbegin(); it != errors.rend(); ++it)
		// {
		// 	Diagnostic::error(*it);
		// }
		Diagnostic::error("One or more required platform dependencies were not found.");
		return false;
	}

	m_platformRequires.clear();

	return true;
}
}
