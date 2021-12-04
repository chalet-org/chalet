/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CompilerCxx/CompilerCxxAppleClang.hpp"

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
bool CompilerCxxAppleClang::addMacosSysRootOption(StringList& outArgList, const BuildState& inState)
{
	const auto& osTarget = inState.info.osTarget();
	std::string sdk{ "macosx" };
	if (String::equals("ios", osTarget))
	{
		sdk = "iphoneos";
	}
	else if (String::equals("watchos", osTarget))
	{
		sdk = "watchos";
	}
	else if (String::equals("tvos", osTarget))
	{
		sdk = "appletvos";
	}

	outArgList.emplace_back("-isysroot");
	outArgList.push_back(inState.tools.applePlatformSdk(sdk));

	return true;
}

/*****************************************************************************/
bool CompilerCxxAppleClang::addArchitectureToCommand(StringList& outArgList, const BuildState& inState)
{
	const auto& osTarget = inState.info.osTarget();
	const auto& osTargetVersion = inState.info.osTargetVersion();
	if (!osTargetVersion.empty())
	{
		if (String::equals("macosx", osTarget))
		{
			// -mmacosx-version-min=
			outArgList.emplace_back(fmt::format("-mmacosx-version-min={}", osTargetVersion));
		}
		else if (String::equals("ios", osTarget))
		{
			// -mmacosx-version-min=
			outArgList.emplace_back(fmt::format("-mios-version-min={}", osTargetVersion));
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
		for (auto& arch : inState.info.universalArches())
		{
			outArgList.emplace_back("-arch");
			outArgList.emplace_back(arch);
		}
	}

	return true;
}

/*****************************************************************************/
void CompilerCxxAppleClang::addSanitizerOptions(StringList& outArgList, const BuildState& inState)
{
	// TODO: others?

	// Note: memory,leaks not supported in AppleClang

	StringList sanitizers;
	if (inState.configuration.sanitizeAddress())
	{
		sanitizers.emplace_back("address");
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
	if (inState.configuration.sanitizeUndefined())
	{
		sanitizers.emplace_back("undefined");
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
void CompilerCxxAppleClang::addPchInclude(StringList& outArgList) const
{
	if (m_project.usesPch())
	{
#if defined(CHALET_MACOS)
		if (m_state.info.targetArchitecture() == Arch::Cpu::UniversalMacOS)
		{
			const auto objDirPch = m_state.paths.getPrecompiledHeaderInclude(m_project);

			auto baseFolder = String::getPathFolder(objDirPch);
			auto filename = String::getPathFilename(objDirPch);

			outArgList.emplace_back("-include");
			outArgList.emplace_back(std::move(filename));

			for (auto& arch : m_state.info.universalArches())
			{
				auto pchPath = fmt::format("{}_{}", baseFolder, arch);

				outArgList.emplace_back(fmt::format("-Xarch_{}", arch));
				outArgList.emplace_back(getPathCommand("-I", pchPath));
			}
		}
		else
#endif
		{
			CompilerCxxClang::addPchInclude(outArgList);
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

		if (!CompilerCxxAppleClang::addArchitectureToCommand(outArgList, m_state))
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
void CompilerCxxAppleClang::addLibStdCppCompileOption(StringList& outArgList, const CxxSpecialization specialization) const
{
	if (specialization != CxxSpecialization::ObjectiveC)
	{
		if (m_project.language() == CodeLanguage::CPlusPlus)
		{
			std::string flag{ "-stdlib=libc++" };
			// if (isFlagSupported(flag))
			List::addIfDoesNotExist(outArgList, std::move(flag));
		}
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
void CompilerCxxAppleClang::addObjectiveCxxCompileOption(StringList& outArgList, const CxxSpecialization specialization) const
{
	const bool isObjCpp = specialization == CxxSpecialization::ObjectiveCPlusPlus;
	const bool isObjC = specialization == CxxSpecialization::ObjectiveC;
	const bool isObjCxx = specialization == CxxSpecialization::ObjectiveCPlusPlus || specialization == CxxSpecialization::ObjectiveC;
	if (m_project.objectiveCxx() && isObjCxx)
	{
		outArgList.emplace_back("-x");
		if (isObjCpp)
			outArgList.emplace_back("objective-c++");
		else if (isObjC)
			outArgList.emplace_back("objective-c");
	}
}

/*****************************************************************************/
void CompilerCxxAppleClang::addObjectiveCxxRuntimeOption(StringList& outArgList, const CxxSpecialization specialization) const
{
	// Unused in AppleClang
	UNUSED(outArgList, specialization);
}

}
