/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Toolchain/CompileToolchainApple.hpp"

#include "Compile/CompilerConfig.hpp"

#include "State/AncillaryTools.hpp"
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

// TODO: Find a nice way to separate out the clang/appleclang stuff from CompileToolchainGNU

namespace chalet
{
/*****************************************************************************/
CompileToolchainApple::CompileToolchainApple(const BuildState& inState, const SourceTarget& inProject, const CompilerConfig& inConfig) :
	CompileToolchainLLVM(inState, inProject, inConfig)
{
}

/*****************************************************************************/
bool CompileToolchainApple::initialize()
{
	if (!CompileToolchainGNU::initialize())
		return false;

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
ToolchainType CompileToolchainApple::type() const noexcept
{
	return ToolchainType::Apple;
}

/*****************************************************************************/
StringList CompileToolchainApple::getDynamicLibTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase) const
{
	StringList ret;

	if (m_config.compilerExecutable().empty())
		return ret;

	addExectuable(ret, m_config.compilerExecutable());

	ret.emplace_back("-dynamiclib");
	// ret.emplace_back("-fPIC");
	// ret.emplace_back("-flat_namespace");

	UNUSED(outputFileBase);

	addStripSymbolsOption(ret);
	addLinkerOptions(ret);
	addMacosSysRootOption(ret);
	addProfileInformationLinkerOption(ret);
	addLinkTimeOptimizationOption(ret);
	addThreadModelLinkerOption(ret);
	addArchitecture(ret);
	addArchitectureOptions(ret);
	addMacosMultiArchOption(ret, std::string());

	addLinkerScripts(ret);
	addLibStdCppLinkerOption(ret);
	addStaticCompilerLibraryOptions(ret);
	addSubSystem(ret);
	addEntryPoint(ret);
	addMacosFrameworkOptions(ret);

	addLibDirs(ret);

	ret.emplace_back("-o");
	ret.push_back(outputFile);
	addSourceObjects(ret, sourceObjs);

	addLinks(ret);
	addObjectiveCxxLink(ret);

	return ret;
}

// ="-Wl,-flat_namespace,-undefined,suppress

/*****************************************************************************/
// Note: Noops mean a flag/feature isn't supported

void CompileToolchainApple::addPchInclude(StringList& outArgList) const
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
			CompileToolchainLLVM::addPchInclude(outArgList);
		}
	}
}

/*****************************************************************************/
bool CompileToolchainApple::addArchitecture(StringList& outArgList) const
{
#if defined(CHALET_MACOS)
	if (m_state.info.targetArchitecture() == Arch::Cpu::UniversalMacOS)
	{}
	else
#endif
	{
		if (!CompileToolchainLLVM::addArchitecture(outArgList))
			return false;
	}

	if (!m_osTargetVersion.empty())
	{
		if (String::equals("macosx", m_osTarget))
		{
			// -mmacosx-version-min=
			outArgList.emplace_back(fmt::format("-mmacosx-version-min={}", m_osTargetVersion));
		}
		else if (String::equals("ios", m_osTarget))
		{
			// -mmacosx-version-min=
			outArgList.emplace_back(fmt::format("-mios-version-min={}", m_osTargetVersion));
		}
	}

	return true;
}

/*****************************************************************************/
bool CompileToolchainApple::addArchitectureOptions(StringList& outArgList) const
{
#if defined(CHALET_MACOS)
	if (m_state.info.targetArchitecture() == Arch::Cpu::UniversalMacOS)
		return true;
#endif
	if (!CompileToolchainLLVM::addArchitectureOptions(outArgList))
		return false;

	return true;
}

/*****************************************************************************/
void CompileToolchainApple::addLibStdCppCompileOption(StringList& outArgList, const CxxSpecialization specialization) const
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
void CompileToolchainApple::addDiagnosticColorOption(StringList& outArgList) const
{
	// Until the compiler argument parser works correctly on mac
	std::string diagnosticColor{ "-fdiagnostics-color=always" };
	// if (isFlagSupported(diagnosticColor))
	List::addIfDoesNotExist(outArgList, std::move(diagnosticColor));
}

/*****************************************************************************/
// Linking
/*****************************************************************************/
/*****************************************************************************/
void CompileToolchainApple::addStripSymbolsOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void CompileToolchainApple::addThreadModelLinkerOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void CompileToolchainApple::addProfileInformationLinkerOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void CompileToolchainApple::addLibStdCppLinkerOption(StringList& outArgList) const
{
	if (m_project.language() == CodeLanguage::CPlusPlus)
	{
		std::string flag{ "-stdlib=libc++" };
		// if (isFlagSupported(flag))
		List::addIfDoesNotExist(outArgList, std::move(flag));

		// TODO: Apple has a "-stdlib=libstdc++" flag that is pre-C++11 for compatibility
	}
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
void CompileToolchainApple::addObjectiveCxxRuntimeOption(StringList& outArgList, const CxxSpecialization specialization) const
{
	// Unused in AppleClang
	UNUSED(outArgList, specialization);
}

/*****************************************************************************/
// MacOS
/*****************************************************************************/
/*****************************************************************************/
void CompileToolchainApple::addMacosMultiArchOption(StringList& outArgList, const std::string& inArch) const
{
#if defined(CHALET_MACOS)
	if (m_state.info.targetArchitecture() != Arch::Cpu::UniversalMacOS)
		return;
#endif

	if (!inArch.empty())
	{
		outArgList.emplace_back("-arch");
		outArgList.emplace_back(inArch);
	}
	else
	{
		for (auto& arch : m_state.info.universalArches())
		{
			outArgList.emplace_back("-arch");
			outArgList.emplace_back(arch);
		}
	}
}

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

	outArgList.emplace_back("-isysroot");
	outArgList.push_back(m_state.tools.applePlatformSdk(sdk));
}

}
