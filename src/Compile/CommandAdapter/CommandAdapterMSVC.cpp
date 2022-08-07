/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CommandAdapter/CommandAdapterMSVC.hpp"

#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CommandAdapterMSVC::CommandAdapterMSVC(const BuildState& inState, const SourceTarget& inProject) :
	m_state(inState),
	m_project(inProject)
{
	m_versionMajorMinor = m_state.toolchain.compilerCxx(m_project.language()).versionMajorMinor;
	m_versionPatch = m_state.toolchain.compilerCxx(m_project.language()).versionPatch;
}

/*****************************************************************************/
MSVCWarningLevel CommandAdapterMSVC::getWarningLevel() const
{
	const auto& warnings = m_project.warnings();

	switch (m_project.warningsPreset())
	{
		case ProjectWarningPresets::Minimal:
			return MSVCWarningLevel::Level1;

		case ProjectWarningPresets::Extra:
			return MSVCWarningLevel::Level2;

		case ProjectWarningPresets::Pedantic:
			return MSVCWarningLevel::Level3;

		case ProjectWarningPresets::Strict:
		case ProjectWarningPresets::StrictPedantic:
			return MSVCWarningLevel::Level4;

		case ProjectWarningPresets::VeryStrict:
			// return MSVCWarningLevel::LevelAll; // Note: Lots of messy compiler level warnings that break your build!
			return MSVCWarningLevel::Level4;

		case ProjectWarningPresets::None:
			break;

		default: {
			StringList veryStrict{
				"noexcept",
				"undef",
				"conversion",
				"cast-qual",
				"float-equal",
				"inline",
				"old-style-cast",
				"strict-null-sentinel",
				"overloaded-virtual",
				"sign-conversion",
				"sign-promo",
			};
			MSVCWarningLevel ret = MSVCWarningLevel::None;

			bool strictSet = false;
			for (auto& w : warnings)
			{
				if (!String::equals(veryStrict, w))
					continue;

				// ret = MSVCWarningLevel::LevelAll;
				ret = MSVCWarningLevel::Level4;
				strictSet = true;
				break;
			}

			if (!strictSet)
			{
				StringList strictPedantic{
					"unused",
					"cast-align",
					"double-promotion",
					"format=2",
					"missing-declarations",
					"missing-include-dirs",
					"non-virtual-dtor",
					"redundant-decls",
					"unreachable-code",
					"shadow",
				};
				for (auto& w : warnings)
				{
					if (!String::equals(strictPedantic, w))
						continue;

					ret = MSVCWarningLevel::Level4;
					strictSet = true;
					break;
				}
			}

			if (!strictSet)
			{
				if (List::contains<std::string>(warnings, "pedantic"))
				{
					ret = MSVCWarningLevel::Level3;
				}
				else if (List::contains<std::string>(warnings, "extra"))
				{
					ret = MSVCWarningLevel::Level2;
				}
				else if (List::contains<std::string>(warnings, "all"))
				{
					ret = MSVCWarningLevel::Level1;
				}
			}

			break;

			return ret;
		}
	}

	return MSVCWarningLevel::None;
}

/*****************************************************************************/
std::string CommandAdapterMSVC::getSubSystem() const
{
	const WindowsSubSystem subSystem = m_project.windowsSubSystem();
	switch (subSystem)
	{
		case WindowsSubSystem::Windows:
			return "windows";
		case WindowsSubSystem::Native:
			return "native";
		case WindowsSubSystem::Posix:
			return "posix";
		case WindowsSubSystem::EfiApplication:
			return "EFI_APPLICATION";
		case WindowsSubSystem::EfiBootServiceDriver:
			return "EFI_BOOT_SERVICE_DRIVER";
		case WindowsSubSystem::EfiRom:
			return "EFI_ROM";
		case WindowsSubSystem::EfiRuntimeDriver:
			return "EFI_RUNTIME_DRIVER";
		case WindowsSubSystem::BootApplication:
			return "BOOT_APPLICATION";

		default: break;
	}

	return "console";
}

/*****************************************************************************/
std::string CommandAdapterMSVC::getEntryPoint() const
{
	const SourceKind kind = m_project.kind();
	const WindowsEntryPoint entryPoint = m_project.windowsEntryPoint();

	if (kind == SourceKind::Executable)
	{
		switch (entryPoint)
		{
			case WindowsEntryPoint::MainUnicode:
				return "wmainCRTStartup";
			case WindowsEntryPoint::WinMain:
				return "WinMainCRTStartup";
			case WindowsEntryPoint::WinMainUnicode:
				return "wWinMainCRTStartup";
			default: break;
		}

		return "mainCRTStartup";
	}
	else if (kind == SourceKind::SharedLibrary)
	{
		if (entryPoint == WindowsEntryPoint::DllMain)
		{
			return "_DllMainCRTStartup";
		}
	}

	return std::string();
}
}
