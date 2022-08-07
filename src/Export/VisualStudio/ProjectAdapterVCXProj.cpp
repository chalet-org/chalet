/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/VisualStudio/ProjectAdapterVCXProj.hpp"

#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
ProjectAdapterVCXProj::ProjectAdapterVCXProj(const BuildState& inState, const SourceTarget& inProject) :
	m_state(inState),
	m_project(inProject),
	m_msvcAdapter(inState, inProject)
{
	m_versionMajorMinor = m_state.toolchain.compilerCxx(m_project.language()).versionMajorMinor;
	m_versionPatch = m_state.toolchain.compilerCxx(m_project.language()).versionPatch;
}

/*****************************************************************************/
bool ProjectAdapterVCXProj::supportsConformanceMode() const
{
	return m_versionMajorMinor >= 1910;
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getBoolean(const bool inValue) const
{
	return std::string(inValue ? "true" : "false");
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getPlatformToolset() const
{
	// TODO: Need a nice way to get this
	return "v143";
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getCharacterSet() const
{
	const auto& executionCharset = m_project.executionCharset();
	if (String::equals({ "UTF-8", "utf-8" }, executionCharset))
	{
		return "Unicode";
	}

	// TODO: More conversions
	return std::string();
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getWarningLevel() const
{
	switch (m_msvcAdapter.getWarningLevel())
	{
		case MSVCWarningLevel::Level1:
			return "Level1";
		case MSVCWarningLevel::Level2:
			return "Level2";
		case MSVCWarningLevel::Level3:
			return "Level3";
		case MSVCWarningLevel::Level4:
			return "Level4";
		case MSVCWarningLevel::LevelAll:
			return "LevelAll";

		default:
			break;
	}

	return std::string();
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getPreprocessorDefinitions() const
{
	auto defines = String::join(m_project.defines(), ';');
	if (!defines.empty())
		defines += ';';

	return defines;
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getLanguageStandardCpp() const
{
	auto standard = m_msvcAdapter.getLanguageStandardCpp();
	if (standard.empty())
		return standard;

	String::replaceAll(standard, '+', 'p');
	return fmt::format("std{}", standard);
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getLanguageStandardC() const
{
	auto standard = m_msvcAdapter.getLanguageStandardCpp();
	if (standard.empty())
		return standard;

	return fmt::format("std{}", standard);
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getSubSystem() const
{
	const WindowsSubSystem subSystem = m_project.windowsSubSystem();
	switch (subSystem)
	{
		case WindowsSubSystem::Windows:
			return "Windows";
		case WindowsSubSystem::Native:
			return "Native";
		case WindowsSubSystem::Posix:
			return "POSIX";
		case WindowsSubSystem::EfiApplication:
			return "EFI Application";
		case WindowsSubSystem::EfiBootServiceDriver:
			return "EFI Boot Service Driver";
		case WindowsSubSystem::EfiRom:
			return "EFI ROM";
		case WindowsSubSystem::EfiRuntimeDriver:
			return "EFI Runtime";

		case WindowsSubSystem::BootApplication:
		default:
			break;
	}

	return "Console";
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getEntryPoint() const
{
	return m_msvcAdapter.getEntryPoint();
}

/*****************************************************************************/
StringList ProjectAdapterVCXProj::getSourceExtensions() const
{
	StringList ret{
		"asmx",
		"asm",
		"bat",
		"hpj",
		"idl",
		"odl",
		"def",
		"ixx",
		"cppm",
		"c++",
		"cxx",
		"cc",
		"c",
		"cpp",
	};

	const auto& fileExtensions = m_state.paths.allFileExtensions();
	const auto& resourceExtensions = m_state.paths.resourceExtensions();

	for (auto& ext : fileExtensions)
	{
		if (String::equals(resourceExtensions, ext))
			continue;

		List::addIfDoesNotExist(ret, ext);
	}

	// MS defaults
	std::reverse(ret.begin(), ret.end());

	return ret;
}

/*****************************************************************************/
StringList ProjectAdapterVCXProj::getHeaderExtensions() const
{
	// MS defaults
	return {
		"h",
		"hh",
		"hpp",
		"hxx",
		"h++",
		"hm",
		"inl",
		"inc",
		"ipp",
		"xsd",
	};
}

/*****************************************************************************/
StringList ProjectAdapterVCXProj::getResourceExtensions() const
{
	// MS defaults
	return {
		"rc",
		"ico",
		"cur",
		"bmp",
		"dlg",
		"rc2",
		"rct",
		"bin",
		"rgs",
		"gif",
		"jpg",
		"jpeg",
		"jpe",
		"resx",
		"tiff",
		"tif",
		"png",
		"wav",
		"mfcribbon-ms",
	};
}

}
