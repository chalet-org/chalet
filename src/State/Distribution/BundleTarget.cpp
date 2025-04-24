/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Distribution/BundleTarget.hpp"

#include "Core/CommandLineInputs.hpp"
#include "FileTemplates/PlatformFileTemplates.hpp"
#include "Process/Environment.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "State/TargetMetadata.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "System/Files.hpp"
#include "Utility/List.hpp"
#include "Utility/Path.hpp"
#include "Utility/String.hpp"

namespace chalet
{
#if defined(CHALET_MACOS)
namespace
{
/*****************************************************************************/
Dictionary<MacOSBundleType> getBundleTypes()
{
	return {
		{ "app", MacOSBundleType::Application },
		{ "framework", MacOSBundleType::Framework },
		{ "plugin", MacOSBundleType::Plugin },
		{ "kext", MacOSBundleType::KernelExtension },
	};
}

/*****************************************************************************/
MacOSBundleType getBundleTypeFromString(const std::string& inValue)
{
	auto bundleTypes = getBundleTypes();
	if (bundleTypes.find(inValue) != bundleTypes.end())
	{
		return bundleTypes.at(inValue);
	}

	return MacOSBundleType::None;
}
}
#endif

/*****************************************************************************/
BundleTarget::BundleTarget(const BuildState& inState) :
	IDistTarget(inState, DistTargetType::DistributionBundle)
{
}

/*****************************************************************************/
bool BundleTarget::initialize()
{
	if (!IDistTarget::initialize())
		return false;

	const auto globMessage = "Check that they exist and glob patterns can be resolved";
	if (!expandGlobPatternsInMap(m_includes, GlobMatch::FilesAndFolders))
	{
		Diagnostic::error("There was a problem resolving the included paths for the '{}' target. {}.", this->name(), globMessage);
		return false;
	}

#if defined(CHALET_WIN32)
	List::addIfDoesNotExist(m_excludes, "**/Thumbs.db");
#elif defined(CHALET_MACOS)
	List::addIfDoesNotExist(m_excludes, "**/.DS_Store");
#endif

	if (!replaceVariablesInPathList(m_excludes))
		return false;

#if defined(CHALET_MACOS)
	if (!m_state.replaceVariablesInString(m_macosBundleInfoPropertyList, this))
		return false;

	if (!m_state.replaceVariablesInString(m_macosBundleEntitlementsPropertyList, this))
		return false;

	if (!m_state.replaceVariablesInString(m_macosBundleIcon, this))
		return false;

#elif defined(CHALET_LINUX)
	if (!m_state.replaceVariablesInString(m_linuxDesktopEntryTemplate, this))
		return false;

	if (!m_state.replaceVariablesInString(m_linuxDesktopEntryIcon, this))
		return false;
#endif

	if (!processIncludeExceptions(m_includes))
		return false;

	return true;
}

/*****************************************************************************/
bool BundleTarget::validate()
{
	bool result = true;

	if (m_buildTargets.empty() && m_includes.empty())
	{
		Diagnostic::error("bundle.include or bundle.buildTargets must be defined, but neither were found.");
		result = false;
	}

#if defined(CHALET_MACOS)
	if (!m_macosBundleIcon.empty())
	{
		if (!String::endsWith(StringList{ ".png", ".icns", ".iconset" }, m_macosBundleIcon))
		{
			Diagnostic::error("bundle.macosBundle.icon must end with '.png', '.icns' or '.iconset', but was '{}'.", m_macosBundleIcon);
			result = false;
		}
		else if (!Files::pathExists(m_macosBundleIcon))
		{
			Diagnostic::error("bundle.macosBundle.icon '{}' was not found.", m_macosBundleIcon);
			result = false;
		}
	}

	if (!m_macosBundleInfoPropertyList.empty())
	{
		if (!String::endsWith(StringList{ ".plist", ".json" }, m_macosBundleInfoPropertyList))
		{
			Diagnostic::error("bundle.macosBundle.infoPropertyList must end with '.plist' or '.json', but was '{}'.", m_macosBundleInfoPropertyList);
			result = false;
		}
		else if (!Files::pathExists(m_macosBundleInfoPropertyList))
		{
			if (String::endsWith(".plist", m_macosBundleInfoPropertyList))
			{
				Diagnostic::error("bundle.macosBundle.infoPropertyList '{}' was not found.", m_macosBundleInfoPropertyList);
				result = false;
			}
			else
			{
				Files::ofstream(m_macosBundleInfoPropertyList) << PlatformFileTemplates::macosInfoPlist();
			}
		}
	}
	else if (m_macosBundleInfoPropertyListContent.empty())
	{
		m_macosBundleInfoPropertyListContent = PlatformFileTemplates::macosInfoPlist();
	}

	if (!m_macosBundleEntitlementsPropertyList.empty())
	{
		if (!String::endsWith(StringList{ ".plist", ".json", ".xml" }, m_macosBundleEntitlementsPropertyList))
		{
			Diagnostic::error("bundle.macosBundle.entitlementsPropertyList must end with '.plist' or '.json', but was '{}'.", m_macosBundleEntitlementsPropertyList);
			result = false;
		}
		else if (!Files::pathExists(m_macosBundleEntitlementsPropertyList))
		{
			Diagnostic::error("bundle.macosBundle.entitlementsPropertyList '{}' was not found.", m_macosBundleEntitlementsPropertyList);
			result = false;
		}
	}
#elif defined(CHALET_LINUX)
	if (!m_linuxDesktopEntryIcon.empty())
	{
		if (!String::endsWith(StringList{ ".png", ".svg" }, m_linuxDesktopEntryIcon))
		{
			Diagnostic::error("bundle.linuxDesktopEntry.icon must end with '.png' or '.svg', but was '{}'.", m_linuxDesktopEntryIcon);
			result = false;
		}
		else if (!Files::pathExists(m_linuxDesktopEntryIcon))
		{
			Diagnostic::error("bundle.linuxDesktopEntry.icon '{}' was not found.", m_linuxDesktopEntryIcon);
			result = false;
		}
	}

	if (!m_linuxDesktopEntryTemplate.empty())
	{
		if (!String::endsWith(".desktop", m_linuxDesktopEntryTemplate))
		{
			Diagnostic::error("bundle.linuxDesktopEntry must end with '.desktop', but was '{}'.", m_linuxDesktopEntryTemplate);
			result = false;
		}
		else if (!Files::pathExists(m_linuxDesktopEntryTemplate))
		{
			Files::ofstream(m_linuxDesktopEntryTemplate) << PlatformFileTemplates::linuxDesktopEntry();
		}
	}
#endif

	return result;
}

/*****************************************************************************/
std::vector<const SourceTarget*> BundleTarget::getRequiredBuildTargets() const
{
	std::vector<const SourceTarget*> ret;

	for (auto& target : m_state.targets)
	{
		if (target->isSources())
		{
			auto project = static_cast<const SourceTarget*>(target.get());
			if (!List::contains(m_buildTargets, project->name()))
			{
				auto outputFilePath = m_state.paths.getTargetFilename(*project);
				if (m_includes.find(outputFilePath) == m_includes.end())
					continue;
			}

			ret.emplace_back(project);
		}
	}

	return ret;
}

/*****************************************************************************/
std::string BundleTarget::getMainExecutable() const
{
	std::string result;

	auto buildTargets = getRequiredBuildTargets();
	auto& mainExec = this->mainExecutable();

	// Match mainExecutable if defined, otherwise get first executable
	for (auto& project : buildTargets)
	{
		if (project->isStaticLibrary())
			continue;

		result = project->outputFile();

		if (!project->isExecutable())
			continue;

		if (!mainExec.empty() && !String::equals(mainExec, project->name()))
			continue;

		return project->outputFile();
	}

	return result;
}

/*****************************************************************************/
const TargetMetadata& BundleTarget::getMainExecutableMetadata() const
{
	auto buildTargets = getRequiredBuildTargets();
	auto& mainExec = this->mainExecutable();

	// Match mainExecutable if defined, otherwise get first executable
	for (auto& project : buildTargets)
	{
		if (!project->isExecutable())
			continue;

		bool hasMetadata = project->hasMetadata();
		if (!mainExec.empty() && !String::equals(mainExec, project->name()))
			continue;

		if (hasMetadata)
			return project->metadata();
		else
			break;
	}

	return m_state.workspace.metadata();
}

/*****************************************************************************/
std::string BundleTarget::getMainExecutableVersion() const
{
	const auto& metadata = getMainExecutableMetadata();
	return metadata.versionString();
}

/*****************************************************************************/
std::string BundleTarget::getMainExecutableVersionShort() const
{
	const auto& version = getMainExecutableMetadata().version();
	return version.majorMinor();
}

/*****************************************************************************/
bool BundleTarget::updateRPaths() const noexcept
{
	return m_updateRPaths;
}

void BundleTarget::setUpdateRPaths(const bool inValue) noexcept
{
	m_updateRPaths = inValue;
}

/*****************************************************************************/
const std::string& BundleTarget::subdirectory() const noexcept
{
	return m_subdirectory;
}

void BundleTarget::setSubdirectory(std::string&& inValue)
{
	m_subdirectory = std::move(inValue);
	Path::toUnix(m_subdirectory);
}

/*****************************************************************************/
const std::string& BundleTarget::mainExecutable() const noexcept
{
	return m_mainExecutable;
}

void BundleTarget::setMainExecutable(std::string&& inValue)
{
	m_mainExecutable = std::move(inValue);
}

/*****************************************************************************/
bool BundleTarget::includeDependentSharedLibraries() const noexcept
{
	return m_includeDependentSharedLibraries;
}

void BundleTarget::setIncludeDependentSharedLibraries(const bool inValue) noexcept
{
	m_includeDependentSharedLibraries = inValue;
}

/*****************************************************************************/
const StringList& BundleTarget::buildTargets() const noexcept
{
	return m_buildTargets;
}

void BundleTarget::addBuildTargets(StringList&& inList)
{
	List::forEach(inList, this, &BundleTarget::addBuildTarget);
}

void BundleTarget::addBuildTarget(std::string&& inValue)
{
	if (String::equals("*", inValue))
	{
		m_hasAllBuildTargets = true;
	}
	else
	{
		Path::toUnix(inValue);
		List::addIfDoesNotExist(m_buildTargets, std::move(inValue));
	}
}

/*****************************************************************************/
bool BundleTarget::hasAllBuildTargets() const noexcept
{
	return m_hasAllBuildTargets;
}

/*****************************************************************************/
const StringList& BundleTarget::excludes() const noexcept
{
	return m_excludes;
}

void BundleTarget::addExcludes(StringList&& inList)
{
	List::forEach(inList, this, &BundleTarget::addExclude);
}

void BundleTarget::addExclude(std::string&& inValue)
{
	Path::toUnix(inValue);
	List::addIfDoesNotExist(m_excludes, std::move(inValue));
}

/*****************************************************************************/
const IDistTarget::IncludeMap& BundleTarget::includes() const noexcept
{
	return m_includes;
}

void BundleTarget::addIncludes(StringList&& inList)
{
	List::forEach(inList, this, &BundleTarget::addInclude);
}

void BundleTarget::addInclude(std::string&& inValue)
{
	Path::toUnix(inValue);

	if (m_includes.find(inValue) == m_includes.end())
		m_includes.emplace(inValue, std::string());
}

void BundleTarget::addInclude(const std::string& inKey, std::string&& inValue)
{
	std::string key = inKey;
	Path::toUnix(key);

	if (m_includes.find(key) == m_includes.end())
		m_includes.emplace(key, std::move(inValue));
}

/*****************************************************************************/
bool BundleTarget::windowsIncludeRuntimeDlls() const noexcept
{
	return m_windowsIncludeRuntimeDlls;
}
void BundleTarget::setWindowsIncludeRuntimeDlls(const bool inValue) noexcept
{
	m_windowsIncludeRuntimeDlls = inValue;
}

#if defined(CHALET_MACOS)
/*****************************************************************************/
MacOSBundleType BundleTarget::macosBundleType() const noexcept
{
	return m_macosBundleType;
}

void BundleTarget::setMacosBundleType(std::string&& inName)
{
	m_macosBundleType = getBundleTypeFromString(inName);

	if (m_macosBundleType != MacOSBundleType::None)
		m_macosBundleExtension = std::move(inName);
}

/*****************************************************************************/
bool BundleTarget::isMacosBundle() const noexcept
{
	return m_macosBundleType != MacOSBundleType::None;
}
bool BundleTarget::isMacosAppBundle() const noexcept
{
	return m_macosBundleType == MacOSBundleType::Application;
}

/*****************************************************************************/
const std::string& BundleTarget::macosBundleExtension() const noexcept
{
	return m_macosBundleExtension;
}

/*****************************************************************************/
const std::string& BundleTarget::macosBundleName() const noexcept
{
	return m_macosBundleName;
}

void BundleTarget::setMacosBundleName(const std::string& inValue)
{
	// bundleName is used specifically for CFBundleName
	// https://developer.apple.com/documentation/bundleresources/information_property_list/cfbundlename
	m_macosBundleName = inValue.substr(0, 15);
}

/*****************************************************************************/
const std::string& BundleTarget::macosBundleIcon() const noexcept
{
	return m_macosBundleIcon;
}

void BundleTarget::setMacosBundleIcon(std::string&& inValue)
{
	m_macosBundleIcon = std::move(inValue);
}

/*****************************************************************************/
MacOSBundleIconMethod BundleTarget::macosBundleIconMethod() const noexcept
{
	return m_macosBundleIconMethod;
}
void BundleTarget::setMacosBundleIconMethod(std::string&& inValue)
{
	if (String::equals("sips", inValue))
		m_macosBundleIconMethod = MacOSBundleIconMethod::Sips;
	else
		m_macosBundleIconMethod = MacOSBundleIconMethod::Actool;
}

/*****************************************************************************/
const std::string& BundleTarget::macosBundleInfoPropertyList() const noexcept
{
	return m_macosBundleInfoPropertyList;
}

void BundleTarget::setMacosBundleInfoPropertyList(std::string&& inValue)
{
	m_macosBundleInfoPropertyList = std::move(inValue);
}

/*****************************************************************************/
const std::string& BundleTarget::macosBundleInfoPropertyListContent() const noexcept
{
	return m_macosBundleInfoPropertyListContent;
}

void BundleTarget::setMacosBundleInfoPropertyListContent(std::string&& inValue)
{
	m_macosBundleInfoPropertyListContent = std::move(inValue);
}

/*****************************************************************************/
const std::string& BundleTarget::macosBundleEntitlementsPropertyList() const noexcept
{
	return m_macosBundleEntitlementsPropertyList;
}

void BundleTarget::setMacosBundleEntitlementsPropertyList(std::string&& inValue)
{
	m_macosBundleEntitlementsPropertyList = std::move(inValue);
}

/*****************************************************************************/
const std::string& BundleTarget::macosBundleEntitlementsPropertyListContent() const noexcept
{
	return m_macosBundleEntitlementsPropertyListContent;
}

void BundleTarget::setMacosBundleEntitlementsPropertyListContent(std::string&& inValue)
{
	m_macosBundleEntitlementsPropertyListContent = std::move(inValue);
}

/*****************************************************************************/
bool BundleTarget::macosCopyToApplications() const noexcept
{
	return m_macosCopyToApplications;
}
void BundleTarget::setMacosCopyToApplications(const bool inValue) noexcept
{
	m_macosCopyToApplications = inValue;
}

/*****************************************************************************/
bool BundleTarget::willHaveMacosInfoPlist() const noexcept
{
	return !m_macosBundleInfoPropertyList.empty() || !m_macosBundleInfoPropertyListContent.empty();
}

/*****************************************************************************/
bool BundleTarget::willHaveMacosEntitlementsPlist() const noexcept
{
	return !m_macosBundleEntitlementsPropertyList.empty() || !m_macosBundleEntitlementsPropertyListContent.empty();
}

#elif defined(CHALET_LINUX)
/*****************************************************************************/
const std::string& BundleTarget::linuxDesktopEntryIcon() const noexcept
{
	return m_linuxDesktopEntryIcon;
}

void BundleTarget::setLinuxDesktopEntryIcon(std::string&& inValue)
{
	m_linuxDesktopEntryIcon = std::move(inValue);
}

/*****************************************************************************/
const std::string& BundleTarget::linuxDesktopEntryTemplate() const noexcept
{
	return m_linuxDesktopEntryTemplate;
}

void BundleTarget::setLinuxDesktopEntryTemplate(std::string&& inValue)
{
	m_linuxDesktopEntryTemplate = std::move(inValue);
}

/*****************************************************************************/
bool BundleTarget::linuxCopyToApplications() const noexcept
{
	return m_linuxCopyToApplications;
}
void BundleTarget::setLinuxCopyToApplications(const bool inValue) noexcept
{
	m_linuxCopyToApplications = inValue;
}

/*****************************************************************************/
bool BundleTarget::hasLinuxDesktopEntry() const noexcept
{
	return !m_linuxDesktopEntryTemplate.empty();
}

#endif
}
