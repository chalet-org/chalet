/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Distribution/BundleTarget.hpp"

#include "FileTemplates/PlatformFileTemplates.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Path.hpp"
#include "Utility/List.hpp"
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
	replaceVariablesInPathList(m_rawIncludes);
	replaceVariablesInPathList(m_excludes);

	return true;
}

/*****************************************************************************/
bool BundleTarget::validate()
{
	bool result = true;

	if (m_buildTargets.empty() && m_rawIncludes.empty())
	{
		Diagnostic::error("bundle.include or bundle.buildTargets must be defined, but neither were found.");
		result = false;
	}

#if defined(CHALET_MACOS)
	if (!m_macosBundleIcon.empty())
	{
		if (!String::endsWith(StringList{ ".png", ".icns" }, m_macosBundleIcon))
		{
			Diagnostic::error("bundle.macosBundleIcon must end with '.png' or '.icns', but was '{}'.", m_macosBundleIcon);
			result = false;
		}
		else if (!Commands::pathExists(m_macosBundleIcon))
		{
			Diagnostic::error("bundle.macosBundleIcon '{}' was not found.", m_macosBundleIcon);
			result = false;
		}
	}

	if (!m_macosBundleInfoPropertyList.empty())
	{
		if (!String::endsWith(StringList{ ".plist", ".json" }, m_macosBundleInfoPropertyList))
		{
			Diagnostic::error("bundle.macosBundleInfoPropertyList must end with '.plist' or '.json', but was '{}'.", m_macosBundleInfoPropertyList);
			result = false;
		}
		else if (!Commands::pathExists(m_macosBundleInfoPropertyList))
		{
			if (String::endsWith(".plist", m_macosBundleInfoPropertyList))
			{
				Diagnostic::error("bundle.macosBundleInfoPropertyList '{}' was not found.", m_macosBundleInfoPropertyList);
				result = false;
			}
			else
			{
				std::ofstream(m_macosBundleInfoPropertyList) << PlatformFileTemplates::macosInfoPlist();
			}
		}
	}
#elif defined(CHALET_LINUX)
	if (!m_linuxDesktopEntryIcon.empty())
	{
		if (!String::endsWith(StringList{ ".png", ".svg" }, m_linuxDesktopEntryIcon))
		{
			Diagnostic::error("bundle.linuxDesktopEntryIcon must end with '.png' or '.svg', but was '{}'.", m_linuxDesktopEntryIcon);
			result = false;
		}
		else if (!Commands::pathExists(m_linuxDesktopEntryIcon))
		{
			Diagnostic::error("bundle.linuxDesktopEntryIcon '{}' was not found.", m_linuxDesktopEntryIcon);
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
		else if (!Commands::pathExists(m_linuxDesktopEntryTemplate))
		{
			std::ofstream(m_linuxDesktopEntryTemplate) << PlatformFileTemplates::linuxDesktopEntry();
		}
	}
#endif

	return result;
}

/*****************************************************************************/
bool BundleTarget::resolveIncludesFromState(const BuildState& inState)
{
	const auto add = [this](std::string in) {
		Path::sanitize(in);
		List::addIfDoesNotExist(m_includes, std::move(in));
	};

	for (auto& dependency : m_rawIncludes)
	{
		if (Commands::pathExists(dependency))
		{
			add(dependency);
			continue;
		}

		/*std::string resolved = fmt::format("{}/{}", inState.paths.buildOutputDir(), inValue);
		if (Commands::pathExists(resolved))
		{
			add(resolved);
			continue;
		}*/

		std::string resolved;
		bool found = false;
		for (auto& target : inState.targets)
		{
			if (target->isSources())
			{
				auto& project = static_cast<const SourceTarget&>(*target);

				const auto& compilerPathBin = inState.toolchain.compilerCxx(project.language()).binDir;

				resolved = fmt::format("{}/{}", compilerPathBin, dependency);
				if (Commands::pathExists(resolved))
				{
					add(std::move(resolved));
					found = true;
					break;
				}

				// LOG(resolved, ' ', project.outputFile());
				if (String::contains(project.outputFile(), resolved))
				{
					add(std::move(resolved));
					found = true;
					break;
				}
			}
		}

		if (!found)
		{

			for (auto& path : inState.workspace.searchPaths())
			{
				resolved = fmt::format("{}/{}", path, dependency);
				if (Commands::pathExists(resolved))
				{
					add(std::move(resolved));
					found = true;
					break;
				}
			}
		}
		if (!found)
		{
			resolved = Commands::which(dependency);
			if (!resolved.empty())
			{
				add(std::move(resolved));
			}
			else
			{
				Diagnostic::warn("Included path '{}' for distribution target '{}' was not found.", dependency, this->name());
			}
		}
	}

	m_includesResolved = true;
	return true;
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
	Path::sanitize(m_subdirectory);
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
		Path::sanitize(inValue);
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
	Path::sanitize(inValue);
	List::addIfDoesNotExist(m_excludes, std::move(inValue));
}

/*****************************************************************************/
const StringList& BundleTarget::includes() const noexcept
{
	chalet_assert(m_includesResolved, "BundleTarget includes not resolved");
	return m_includes;
}

void BundleTarget::addIncludes(StringList&& inList)
{
	List::forEach(inList, this, &BundleTarget::addInclude);
}

void BundleTarget::addInclude(std::string&& inValue)
{
	Path::sanitize(inValue);
	List::addIfDoesNotExist(m_rawIncludes, std::move(inValue));
}

/*****************************************************************************/
void BundleTarget::sortIncludes()
{
	List::sort(m_includes);
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
bool BundleTarget::hasLinuxDesktopEntry() const noexcept
{
	return !m_linuxDesktopEntryTemplate.empty();
}

#endif
}
