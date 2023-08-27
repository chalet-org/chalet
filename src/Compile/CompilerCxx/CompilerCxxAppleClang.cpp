/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CompilerCxx/CompilerCxxAppleClang.hpp"

#include "Core/CommandLineInputs.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

/*
	Some example Apple arch details, from here: https://github.com/rust-lang/rust/issues/48862
	macOS
		MacOSX
		i386,x86_64
		x86_64-apple-macosx10.13.0
		-mmacosx-version-min or -mmacos-version-min
	iOS
		iPhoneOS
		arm64,armv7,armv7s
		arm64-apple-ios11.2.0
		-miphoneos-version-min or -mios-version-min
	iOS Simulator
		iPhoneOSSimulator
		i386,x86_64
		x86_64-apple-ios11.2.0
		-miphonesimulator-version-min or -mios-simulator-version-min
	watchOS
		WatchOS
		armv7k
		thumbv7k-apple-watchos4.2.0
		-mwatchos-version-min
	watchOS Simulator
		WatchSimulator
		i386,x86_64
		x86_64-apple-watchos4.2.0
		-mwatchsimulator-version-min or -mwatchos-simulator-version-min
	tvOS
		AppleTVOS
		arm64
		arm64-apple-tvos11.2.0
		-mappletvos-version-min or -mtvos-version-min
	tvOS Simulator
		AppleTVSimulator
		x86_64
		x86_64-apple-tvos11.2.0
		-mappletvsimulator-version-min or -mtvos-simulator-version-min
*/

namespace chalet
{
/*****************************************************************************/
CompilerCxxAppleClang::CompilerCxxAppleClang(const BuildState& inState, const SourceTarget& inProject) :
	CompilerCxxClang(inState, inProject)
{
}

/*****************************************************************************/
StringList CompilerCxxAppleClang::getAllowedSDKTargets()
{
	StringList ret{
		"macosx",
		"iphoneos",
		"iphonesimulator",
		"watchos",
		"watchsimulator",
		"appletvos",
		"appletvsimulator",
		"xros",
		"xrsimulator"
	};
	return ret;
}

/*****************************************************************************/
bool CompilerCxxAppleClang::addSystemRootOption(StringList& outArgList, const BuildState& inState)
{
	const auto& osTargetName = inState.inputs.osTargetName();
	if (!osTargetName.empty())
	{
		auto kAllowedTargets = getAllowedSDKTargets();
		if (String::equals(kAllowedTargets, osTargetName))
		{
			auto sdkPath = inState.tools.getApplePlatformSdk(osTargetName);
			if (!sdkPath.empty())
			{
				// Note: If -m(sdk)-version-min= isn't specified, the version is inferred from the SDK,
				//   which has its own minimum version ("MacOSX13.3.sdk" is 13.0 for instance)
				outArgList.emplace_back("-isysroot");
				outArgList.push_back(IToolchainExecutableBase::getQuotedPath(inState, sdkPath));
			}
		}
	}

	return true;
}

/*****************************************************************************/
bool CompilerCxxAppleClang::addArchitectureToCommand(StringList& outArgList, const BuildState& inState, const uint inVersionMajorMinor)
{
	const auto& osTargetName = inState.inputs.osTargetName();
	const auto& osTargetVersion = inState.inputs.osTargetVersion();
	if (!osTargetVersion.empty())
	{
		auto kAllowedTargets = getAllowedSDKTargets();
		if (String::equals(kAllowedTargets, osTargetName))
		{
			auto outputTargetName = osTargetName;
			String::replaceAll(outputTargetName, "simulator", "os");
			String::replaceAll(outputTargetName, "iphone", "i");

			UNUSED(inVersionMajorMinor);
			// if (inVersionMajorMinor >= 1400)
			if (String::startsWith("xr", osTargetName))
			{

				outArgList.emplace_back(fmt::format("-mtargetos={}{}", outputTargetName, osTargetVersion));
			}
			else
			{
				// Example: -mmacosx-version-min=13.1
				outArgList.emplace_back(fmt::format("-m{}-version-min={}", outputTargetName, osTargetVersion));
			}
		}
	}

	return true;
}

/*****************************************************************************/
bool CompilerCxxAppleClang::addMultiArchOptionsToCommand(StringList& outArgList, const std::string& inArch, const BuildState& inState)
{
	if (!inArch.empty())
	{
		outArgList.emplace_back("-arch");
		outArgList.emplace_back(inArch);
	}
	else
	{
		for (auto& arch : inState.inputs.universalArches())
		{
			outArgList.emplace_back("-arch");
			outArgList.emplace_back(arch);
		}
	}

	return true;
}

/*****************************************************************************/
void CompilerCxxAppleClang::addProfileInformation(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void CompilerCxxAppleClang::addSanitizerOptions(StringList& outArgList, const BuildState& inState)
{
	// Note: memory,leaks not supported in AppleClang

	StringList sanitizers;
	if (inState.configuration.sanitizeAddress())
	{
		sanitizers.emplace_back("address");
	}
	if (inState.configuration.sanitizeHardwareAddress())
	{
		sanitizers.emplace_back("hwaddress");
	}
	if (inState.configuration.sanitizeThread())
	{
		sanitizers.emplace_back("thread");
	}
	/*if (inState.configuration.sanitizeMemory())
	{
		sanitizers.emplace_back("memory");
	}
	if (inState.configuration.sanitizeLeaks())
	{
		sanitizers.emplace_back("leak");
	}*/
	if (inState.configuration.sanitizeUndefinedBehavior())
	{
		sanitizers.emplace_back("undefined");
		sanitizers.emplace_back("integer");
	}

	if (!sanitizers.empty())
	{
		auto list = String::join(sanitizers, ',');
		outArgList.emplace_back(fmt::format("-fsanitize={}", list));
	}
}

/*****************************************************************************/
void CompilerCxxAppleClang::addSanitizerOptions(StringList& outArgList) const
{
	if (m_state.configuration.enableSanitizers())
	{
		CompilerCxxAppleClang::addSanitizerOptions(outArgList, m_state);
	}
}

/*****************************************************************************/
// Note: Noops mean a flag/feature isn't supported
//
void CompilerCxxAppleClang::addPchInclude(StringList& outArgList, const SourceType derivative) const
{
	if (precompiledHeaderAllowedForSourceType(derivative))
	{
#if defined(CHALET_MACOS)
		if (m_state.info.targetArchitecture() == Arch::Cpu::UniversalMacOS)
		{
			const auto objDirPch = m_state.paths.getPrecompiledHeaderInclude(m_project);

			auto baseFolder = String::getPathFolder(objDirPch);
			auto filename = String::getPathFilename(objDirPch);

			outArgList.emplace_back("-include");
			outArgList.emplace_back(std::move(filename));

			for (auto& arch : m_state.inputs.universalArches())
			{
				auto pchPath = fmt::format("{}_{}", baseFolder, arch);

				outArgList.emplace_back(fmt::format("-Xarch_{}", arch));
				outArgList.emplace_back(getPathCommand("-I", pchPath));
			}
		}
		else
#endif
		{
			CompilerCxxClang::addPchInclude(outArgList, derivative);
		}
	}
}

/*****************************************************************************/
bool CompilerCxxAppleClang::addArchitecture(StringList& outArgList, const std::string& inArch) const
{
#if defined(CHALET_MACOS)
	if (m_state.info.targetArchitecture() != Arch::Cpu::UniversalMacOS)
#endif
	{
		if (!CompilerCxxClang::addArchitecture(outArgList, inArch))
			return false;

		if (!CompilerCxxAppleClang::addArchitectureToCommand(outArgList, m_state, m_versionMajorMinor))
			return false;
	}
#if defined(CHALET_MACOS)
	else
	{
		if (!CompilerCxxAppleClang::addMultiArchOptionsToCommand(outArgList, inArch, m_state))
			return false;
	}
#endif

	return true;
}

/*****************************************************************************/
void CompilerCxxAppleClang::addLibStdCppCompileOption(StringList& outArgList, const SourceType derivative) const
{
	auto language = m_project.language();
	bool validPchType = derivative == SourceType::CxxPrecompiledHeader && (language == CodeLanguage::CPlusPlus || language == CodeLanguage::ObjectiveCPlusPlus);
	if (validPchType || derivative == SourceType::CPlusPlus || derivative == SourceType::ObjectiveCPlusPlus)
	{
		auto flag = fmt::format("-stdlib={}", m_clangAdapter.getCxxLibrary());
		// if (isFlagSupported(flag))
		List::addIfDoesNotExist(outArgList, std::move(flag));
	}
}

/*****************************************************************************/
void CompilerCxxAppleClang::addDiagnosticColorOption(StringList& outArgList) const
{
	std::string diagnosticColor{ "-fdiagnostics-color=always" };
	// if (isFlagSupported(diagnosticColor))
	List::addIfDoesNotExist(outArgList, std::move(diagnosticColor));
}

/*****************************************************************************/
bool CompilerCxxAppleClang::addSystemRootOption(StringList& outArgList) const
{
	return CompilerCxxAppleClang::addSystemRootOption(outArgList, m_state);
}

/*****************************************************************************/
bool CompilerCxxAppleClang::addSystemIncludes(StringList& outArgList) const
{
	UNUSED(outArgList);
	return true;
}

/*****************************************************************************/
void CompilerCxxAppleClang::addObjectiveCxxRuntimeOption(StringList& outArgList, const SourceType derivative) const
{
	// Unused in AppleClang
	UNUSED(outArgList, derivative);
}

}
