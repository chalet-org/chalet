/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Toolchain/CompileToolchainApple.hpp"

#include "Libraries/Format.hpp"
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

// TODO: Find a nice way to separate out the clang/appleclang stuff from CompileToolchainGNU

namespace chalet
{
/*****************************************************************************/
CompileToolchainApple::CompileToolchainApple(const BuildState& inState, const ProjectTarget& inProject, const CompilerConfig& inConfig) :
	CompileToolchainLLVM(inState, inProject, inConfig)
{
}

/*****************************************************************************/
bool CompileToolchainApple::initialize()
{
	const auto& targetArchString = m_state.info.targetArchitectureString();
	std::string macosVersion;
	auto triple = String::split(targetArchString, '-');
	if (triple.size() == 3)
	{
		auto& sys = triple.back();
		sys = String::toLowerCase(sys);

		for (auto& target : StringList{ "macos", "ios", "watchos", "tvos" })
		{
			if (String::startsWith(target, sys))
			{
				m_osTarget = target;
				m_osTargetVersion = sys.substr(target.size());
				break;
			}
		}
	}

	return true;
}

/*****************************************************************************/
ToolchainType CompileToolchainApple::type() const
{
	return ToolchainType::Apple;
}

// ="-Wl,-flat_namespace,-undefined,suppress

/*****************************************************************************/
// Note: Noops mean a flag/feature isn't supported

bool CompileToolchainApple::addArchitecture(StringList& outArgList) const
{
	if (!CompileToolchainLLVM::addArchitecture(outArgList))
		return false;

	if (!m_osTargetVersion.empty())
	{
		if (String::equals("macosx", m_osTarget))
		{
			// -mmacosx-version-min=
			outArgList.push_back(fmt::format("-mmacosx-version-min={}", m_osTargetVersion));
		}
		else if (String::equals("ios", m_osTarget))
		{
			// -mmacosx-version-min=
			outArgList.push_back(fmt::format("-mios-version-min={}", m_osTargetVersion));
		}
	}

	return true;
}

/*****************************************************************************/
// Linking
/*****************************************************************************/
/*****************************************************************************/
void CompileToolchainApple::addProfileInformationLinkerOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void CompileToolchainApple::addLibStdCppLinkerOption(StringList& outArgList) const
{
	CompileToolchainLLVM::addLibStdCppLinkerOption(outArgList);

	// TODO: Apple has a "-stdlib=libstdc++" flag that is pre-C++11 for compatibility
}

/*****************************************************************************/
// Objective-C / Objective-C++
/*****************************************************************************/
/*****************************************************************************/
void CompileToolchainApple::addObjectiveCxxLink(StringList& outArgList) const
{
	// Unused in AppleClang
	UNUSED(outArgList);
}

/*****************************************************************************/
void CompileToolchainApple::addObjectiveCxxCompileOption(StringList& outArgList, const CxxSpecialization specialization) const
{
	const bool isObjCpp = specialization == CxxSpecialization::ObjectiveCpp;
	const bool isObjC = specialization == CxxSpecialization::ObjectiveC;
	const bool isObjCxx = specialization == CxxSpecialization::ObjectiveCpp || specialization == CxxSpecialization::ObjectiveC;
	if (m_project.objectiveCxx() && isObjCxx)
	{
		outArgList.push_back("-x");
		if (isObjCpp)
			outArgList.push_back("objective-c++");
		else if (isObjC)
			outArgList.push_back("objective-c");
	}
}

/*****************************************************************************/
void CompileToolchainApple::addObjectiveCxxRuntimeOption(StringList& outArgList, const CxxSpecialization specialization) const
{
	// Unused in AppleClang
	UNUSED(outArgList, specialization);
}

/*****************************************************************************/
// MacOS
/*****************************************************************************/
/*****************************************************************************/
void CompileToolchainApple::addMacosSysRootOption(StringList& outArgList) const
{
	std::string sdk{ "macosx" };
	if (String::equals("ios", m_osTarget))
	{
		sdk = "iphoneos";
	}
	else if (String::equals("watchos", m_osTarget))
	{
		sdk = "watchos";
	}
	else if (String::equals("tvos", m_osTarget))
	{
		sdk = "appletvos";
	}

	outArgList.push_back("-isysroot");
	outArgList.push_back(m_state.tools.applePlatformSdk(sdk));
}

}
