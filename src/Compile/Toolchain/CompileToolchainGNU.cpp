/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Toolchain/CompileToolchainGNU.hpp"

#include "Libraries/Format.hpp"
#include "Libraries/Regex.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CompileToolchainGNU::CompileToolchainGNU(const BuildState& inState, const ProjectTarget& inProject, const CompilerConfig& inConfig) :
	ICompileToolchain(inState),
	m_project(inProject),
	m_config(inConfig)
{
}

/*****************************************************************************/
ToolchainType CompileToolchainGNU::type() const noexcept
{
	return ToolchainType::GNU;
}

/*****************************************************************************/
bool CompileToolchainGNU::initialize()
{
	const auto& targetArchString = m_state.info.targetArchitectureString();
	if (!String::contains('-', targetArchString))
	{
		Diagnostic::error(fmt::format("Target architecture expected to be a triple, but was '{}'", targetArchString));
		return false;
	}

	std::string macosVersion;
	auto triple = String::split(targetArchString, '-');
	if (triple.size() == 3)
	{
		m_arch = triple.front();
	}
	else
	{
		m_arch = targetArchString;
	}

	initializeArchPresets();
	// initializeSupportedLinks();

	return true;
}

/*****************************************************************************/
StringList CompileToolchainGNU::getPchCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency)
{
	StringList ret;

	addExectuable(ret, m_config.compilerExecutable());

	if (generateDependency)
	{
		ret.push_back("-MT");
		ret.push_back(outputFile);
		ret.push_back("-MMD");
		ret.push_back("-MP");
		ret.push_back("-MF");
		ret.push_back(dependency);
	}

	const auto specialization = m_project.language() == CodeLanguage::CPlusPlus ? CxxSpecialization::Cpp : CxxSpecialization::C;
	addOptimizationOption(ret);
	addLanguageStandard(ret, specialization);
	addWarnings(ret);

	addLibStdCppCompileOption(ret, specialization);
	addPositionIndependentCodeOption(ret);
	addCompileOptions(ret);
	addObjectiveCxxRuntimeOption(ret, specialization);
	addDiagnosticColorOption(ret);
	addNoRunTimeTypeInformationOption(ret);
	addThreadModelCompileOption(ret);
	addArchitecture(ret);

	addDebuggingInformationOption(ret);
	addProfileInformationCompileOption(ret);

	addDefines(ret);
	addIncludes(ret);
	addMacosSysRootOption(ret);

	ret.push_back("-o");
	ret.push_back(outputFile);
	ret.push_back("-c");
	ret.push_back(inputFile);

	return ret;
}

/*****************************************************************************/
StringList CompileToolchainGNU::getRcCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency)
{
	StringList ret;

	addExectuable(ret, m_state.compilerTools.rc());

	ret.push_back("-J");
	ret.push_back("rc");
	ret.push_back("-O");
	ret.push_back("coff");

	if (generateDependency)
	{
		// Note: The dependency generation args have to be passed into the preprocessor
		//   The underlying preprocessor command is "gcc -E -xc-header -DRC_INVOKED"
		//   This runs in C mode, so we don't want any c++ flags passed in
		//   See: https://sourceware.org/binutils/docs/binutils/windres.html

		ret.push_back("--preprocessor-arg=-MT");
		ret.push_back(fmt::format("--preprocessor-arg={}", outputFile));
		ret.push_back("--preprocessor-arg=-MMD");
		ret.push_back("--preprocessor-arg=-MP");
		ret.push_back("--preprocessor-arg=-MF");
		ret.push_back(fmt::format("--preprocessor-arg={}", dependency));
	}

	addDefines(ret);
	addIncludes(ret);
	addMacosSysRootOption(ret);

	ret.push_back("-i");
	ret.push_back(inputFile);
	ret.push_back("-o");
	ret.push_back(outputFile);

	return ret;
}

/*****************************************************************************/
StringList CompileToolchainGNU::getCxxCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency, const CxxSpecialization specialization)
{
	StringList ret;

	addExectuable(ret, m_config.compilerExecutable());

	if (generateDependency)
	{
		ret.push_back("-MT");
		ret.push_back(outputFile);
		ret.push_back("-MMD");
		ret.push_back("-MP");
		ret.push_back("-MF");
		ret.push_back(dependency);
	}

	addOptimizationOption(ret);
	addLanguageStandard(ret, specialization);
	addWarnings(ret);
	addObjectiveCxxCompileOption(ret, specialization);

	addLibStdCppCompileOption(ret, specialization);
	addPositionIndependentCodeOption(ret);
	addCompileOptions(ret);
	addObjectiveCxxRuntimeOption(ret, specialization);
	addDiagnosticColorOption(ret);
	addNoRunTimeTypeInformationOption(ret);
	addThreadModelCompileOption(ret);
	addArchitecture(ret);

	addDebuggingInformationOption(ret);
	addProfileInformationCompileOption(ret);

	addDefines(ret);
	addIncludes(ret);
	addMacosSysRootOption(ret);

	if (specialization == CxxSpecialization::C || specialization == CxxSpecialization::Cpp)
		addPchInclude(ret);

	ret.push_back("-o");
	ret.push_back(outputFile);
	ret.push_back("-c");
	ret.push_back(inputFile);

	return ret;
}

/*****************************************************************************/
// TODO: This treats clang as a dylib target which is wrong... (only on apple)
StringList CompileToolchainGNU::getLinkerTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase)
{
	switch (m_project.kind())
	{
		case ProjectKind::SharedLibrary: {
			return getDynamicLibTargetCommand(outputFile, sourceObjs, outputFileBase);
		}

		case ProjectKind::StaticLibrary: {
			return getStaticLibTargetCommand(outputFile, sourceObjs);
		}

		case ProjectKind::DesktopApplication:
		case ProjectKind::ConsoleApplication:
		default:
			break;
	}

	return getExecutableTargetCommand(outputFile, sourceObjs);
}

/*****************************************************************************/
StringList CompileToolchainGNU::getDynamicLibTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase) const
{
	StringList ret;

	addExectuable(ret, m_config.compilerExecutable());

	ret.push_back("-shared");
	if (m_config.isMingw())
	{
		std::string mingwLinkerOptions;
		if (m_project.windowsOutputDef())
		{
			mingwLinkerOptions = fmt::format("-Wl,--output-def={}.def", outputFileBase);
		}
		mingwLinkerOptions += fmt::format("-Wl,--out-implib={}.a", outputFileBase);
		ret.push_back(std::move(mingwLinkerOptions));

		ret.push_back("-Wl,--dll");
	}
	else
	{
		addPositionIndependentCodeOption(ret);
	}

	addStripSymbolsOption(ret);
	addLinkerOptions(ret);
	addMacosSysRootOption(ret);
	addProfileInformationLinkerOption(ret);
	addLinkTimeOptimizationOption(ret);
	addThreadModelLinkerOption(ret);
	addArchitecture(ret);
	addLinkerScripts(ret);
	addLibStdCppLinkerOption(ret);
	addStaticCompilerLibraryOptions(ret);
	addPlatformGuiApplicationFlag(ret);
	addMacosFrameworkOptions(ret);

	addLibDirs(ret);

	ret.push_back("-o");
	ret.push_back(outputFile);
	addSourceObjects(ret, sourceObjs);

	addLinks(ret);
	addObjectiveCxxLink(ret);

	return ret;
}

/*****************************************************************************/
StringList CompileToolchainGNU::getStaticLibTargetCommand(const std::string& outputFile, const StringList& sourceObjs) const
{
	StringList ret;

	addExectuable(ret, m_state.compilerTools.archiver());

	if (m_state.compilerTools.isArchiverLibTool())
	{
		ret.push_back("-static");
		ret.push_back("-no_warning_for_no_symbols");
		ret.push_back("-o");
	}
	else
	{
		ret.push_back("-c");
		ret.push_back("-r");
		ret.push_back("-s");
	}

	ret.push_back(outputFile);
	addSourceObjects(ret, sourceObjs);

	return ret;
}

/*****************************************************************************/
StringList CompileToolchainGNU::getExecutableTargetCommand(const std::string& outputFile, const StringList& sourceObjs) const
{
	StringList ret;

	addExectuable(ret, m_config.compilerExecutable());

	addLibDirs(ret);

	ret.push_back("-o");
	ret.push_back(outputFile);

	addRunPath(ret);
	addSourceObjects(ret, sourceObjs);

	addLinks(ret);

	addStripSymbolsOption(ret);
	addLinkerOptions(ret);
	addMacosSysRootOption(ret);
	addProfileInformationLinkerOption(ret);
	addLinkTimeOptimizationOption(ret);
	addThreadModelLinkerOption(ret);
	addArchitecture(ret);
	addLinkerScripts(ret);
	addLibStdCppLinkerOption(ret);
	addStaticCompilerLibraryOptions(ret);
	addPlatformGuiApplicationFlag(ret);
	addMacosFrameworkOptions(ret);

	return ret;
}

/*****************************************************************************/
StringList CompileToolchainGNU::getLinkExclusions() const
{
	return {};
}

/*****************************************************************************/
bool CompileToolchainGNU::isFlagSupported(const std::string& inFlag) const
{
	if (m_config.isGcc())
	{
		if (String::contains('=', inFlag))
		{
			auto cutoff = inFlag.find('=');
			std::string flag = inFlag.substr(cutoff + 1);
			return m_config.isFlagSupported(flag);
		}
		else
		{
			return m_config.isFlagSupported(inFlag);
		}
	}

	return true;
}

/*****************************************************************************/
bool CompileToolchainGNU::isLinkSupported(const std::string& inLink) const
{
	if (m_supportedLinksInitialized && m_config.isGcc())
	{
		return m_supportedLinks.find(inLink) != m_supportedLinks.end();
	}

	return true;
}

/*****************************************************************************/
std::string CompileToolchainGNU::getPathCommand(std::string_view inCmd, const std::string& inPath) const
{
	if (m_quotePaths)
		return fmt::format("{}\"{}\"", inCmd, inPath);
	else
		return fmt::format("{}{}", inCmd, inPath);
}

/*****************************************************************************/
void CompileToolchainGNU::addSourceObjects(StringList& outArgList, const StringList& sourceObjs) const
{
	for (auto& source : sourceObjs)
	{
		outArgList.push_back(source);
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addIncludes(StringList& outArgList) const
{
	const std::string prefix{ "-I" };
	for (const auto& dir : m_project.includeDirs())
	{
		outArgList.push_back(getPathCommand(prefix, dir));
	}
	for (const auto& dir : m_project.locations())
	{
		outArgList.push_back(getPathCommand(prefix, dir));
	}

#if !defined(CHALET_WIN32)
	// must be last
	List::addIfDoesNotExist(outArgList, getPathCommand(prefix, "/usr/local/include/"));
#endif
}

/*****************************************************************************/
void CompileToolchainGNU::addLibDirs(StringList& outArgList) const
{
	const std::string prefix{ "-L" };
	for (const auto& dir : m_project.libDirs())
	{
		outArgList.push_back(getPathCommand(prefix, dir));
	}

	outArgList.push_back(getPathCommand(prefix, m_state.paths.buildOutputDir()));

#if !defined(CHALET_WIN32)
	// must be last
	List::addIfDoesNotExist(outArgList, getPathCommand(prefix, "/usr/local/lib/"));
#endif
}

/*****************************************************************************/
void CompileToolchainGNU::addWarnings(StringList& outArgList) const
{
	const std::string prefix{ "-W" };
	for (auto& warning : m_project.warnings())
	{
		std::string out;
		if (String::equals(warning, "pedantic-errors"))
		{
			out = "-" + warning;
		}
		else
		{
			out = prefix + warning;
		}

		if (isFlagSupported(out))
			outArgList.push_back(std::move(out));
	}

	if (m_project.usesPch())
	{
		std::string invalidPch = prefix + "invalid-pch";
		if (isFlagSupported(invalidPch))
			List::addIfDoesNotExist(outArgList, std::move(invalidPch));
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addDefines(StringList& outArgList) const
{
	const std::string prefix{ "-D" };
	for (auto& define : m_project.defines())
	{
		outArgList.push_back(prefix + define);
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addLinks(StringList& outArgList) const
{
	const std::string prefix{ "-l" };
	const bool hasStaticLinks = m_project.staticLinks().size() > 0;
	const bool hasDynamicLinks = m_project.links().size() > 0;

	if (hasStaticLinks)
	{
		startStaticLinkGroup(outArgList);

		for (auto& staticLink : m_project.staticLinks())
		{
			if (isLinkSupported(staticLink))
				outArgList.push_back(prefix + staticLink);
		}

		endStaticLinkGroup(outArgList);
	}

	if (hasDynamicLinks)
	{
		if (hasStaticLinks)
			startExplicitDynamicLinkGroup(outArgList);

		auto excludes = getLinkExclusions();

		for (auto& link : m_project.links())
		{
			if (List::contains(excludes, link))
				continue;

			if (isLinkSupported(link))
				outArgList.push_back(prefix + link);
		}
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addPchInclude(StringList& outArgList) const
{
	// TODO: Potential for more than one pch?
	if (m_project.usesPch())
	{
		const auto objDirPch = m_state.paths.getPrecompiledHeaderInclude(m_project);

		outArgList.push_back("-include");
		outArgList.push_back(getPathCommand("", objDirPch));
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addOptimizationOption(StringList& outArgList) const
{
	std::string opt;
	auto& configuration = m_state.configuration;

	OptimizationLevel level = configuration.optimizations();

	if (configuration.debugSymbols()
		&& level != OptimizationLevel::Debug
		&& level != OptimizationLevel::None
		&& level != OptimizationLevel::CompilerDefault)
	{
		// force -O0 (anything else would be in error)
		opt = "-O0";
	}
	else
	{
		switch (level)
		{
			case OptimizationLevel::L1:
				opt = "-O1";
				break;

			case OptimizationLevel::L2:
				opt = "-O2";
				break;

			case OptimizationLevel::L3:
				opt = "-O3";
				break;

			case OptimizationLevel::Debug:
				opt = "-Og";
				break;

			case OptimizationLevel::Size:
				opt = "-Os";
				break;

			case OptimizationLevel::Fast:
				opt = "-Ofast";
				break;

			case OptimizationLevel::None:
				opt = "-O0";
				break;

			case OptimizationLevel::CompilerDefault:
			default:
				break;
		}
	}

	if (opt.empty())
		return;

	outArgList.push_back(std::move(opt));
}

/*****************************************************************************/
void CompileToolchainGNU::addRunPath(StringList& outArgList) const
{
#if defined(CHALET_LINUX)
	outArgList.push_back("-Wl,-rpath,'$$ORIGIN'"); // Note: Single quotes are required!
#else
	UNUSED(outArgList);
#endif
}

/*****************************************************************************/
void CompileToolchainGNU::addLanguageStandard(StringList& outArgList, const CxxSpecialization specialization) const
{
	const bool useC = m_project.language() == CodeLanguage::C || specialization == CxxSpecialization::ObjectiveC;
	const auto& langStandard = useC ? m_project.cStandard() : m_project.cppStandard();
	std::string ret = String::toLowerCase(langStandard);

	// TODO: Make this "dumber" so it only the allowed strings used by each compiler

#ifndef CHALET_MSVC
	static constexpr auto regex = ctll::fixed_string{ "^(((c|gnu)\\+\\+|gnu|c|iso9899:)(\\d[\\dzaxy]{1,3}|199409))$" };
	if (auto m = ctre::match<regex>(ret))
#else
	static std::regex regex{ "^(((c|gnu)\\+\\+|gnu|c|iso9899:)(\\d[\\dzaxy]{1,3}|199409))$" };
	if (std::regex_match(ret, regex))
#endif
	{
		ret = "-std=" + ret;

		outArgList.push_back(std::move(ret));
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addDebuggingInformationOption(StringList& outArgList) const
{
	// TODO: Control debugging information level (g, g0-g3) from configurations
	if (m_state.configuration.debugSymbols())
	{
		std::string debugInfo{ "-g3" };
		if (isFlagSupported(debugInfo))
			outArgList.push_back(std::move(debugInfo));
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addProfileInformationCompileOption(StringList& outArgList) const
{
	// TODO: gcc/clang distinction on mac?

	if (m_state.configuration.enableProfiling())
	{
		if (!m_project.isSharedLibrary())
		{
			std::string profileInfo{ "-pg" };
			if (isFlagSupported(profileInfo))
				outArgList.push_back(std::move(profileInfo));
		}
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addCompileOptions(StringList& outArgList) const
{
	for (auto& option : m_project.compileOptions())
	{
		List::addIfDoesNotExist(outArgList, option);
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addDiagnosticColorOption(StringList& outArgList) const
{
	std::string diagnosticColor{ "-fdiagnostics-color=always" };
	if (isFlagSupported(diagnosticColor))
		List::addIfDoesNotExist(outArgList, std::move(diagnosticColor));
}

/*****************************************************************************/
void CompileToolchainGNU::addLibStdCppCompileOption(StringList& outArgList, const CxxSpecialization specialization) const
{
	UNUSED(outArgList, specialization);
}

/*****************************************************************************/
void CompileToolchainGNU::addPositionIndependentCodeOption(StringList& outArgList) const
{
	if (!m_config.isMingw())
	{
		std::string fpic{ "-fPIC" };
		if (isFlagSupported(fpic))
			List::addIfDoesNotExist(outArgList, std::move(fpic));
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addNoRunTimeTypeInformationOption(StringList& outArgList) const
{
	if (!m_project.rtti())
	{
		std::string noRtti{ "-fno-rtti" };
		if (isFlagSupported(noRtti))
			List::addIfDoesNotExist(outArgList, std::move(noRtti));
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addThreadModelCompileOption(StringList& outArgList) const
{
	auto threadType = m_project.threadType();
	if (threadType == ThreadType::Posix || threadType == ThreadType::Auto)
	{
		std::string pthread{ "-pthread" };
		if (isFlagSupported(pthread))
			List::addIfDoesNotExist(outArgList, std::move(pthread));
	}
}

/*****************************************************************************/
bool CompileToolchainGNU::addArchitecture(StringList& outArgList) const
{
	auto hostArch = m_state.info.hostArchitecture();
	auto targetArch = m_state.info.targetArchitecture();

#if defined(CHALET_WIN32)
	if (String::equals({ "arm", "arm64" }, m_arch))
	{
		// don't do anything yet
		return false;
	}
#endif

	if (hostArch == targetArch && targetArch != Arch::Cpu::Unknown)
		return false;

	// https://gcc.gnu.org/onlinedocs/gcc/x86-Options.html
	// gcc -Q --help=target

	std::string arch;
	std::string tune;
	std::string flags;
	if (String::equals({ "intel", "generic" }, m_arch))
	{
		tune = m_arch;
		flags = "-m64";
	}
	else if (String::equals({ "x86_64", "x64" }, m_arch))
	{
		arch = "x86-64";
		tune = "generic";
		flags = "-m64";
	}
	else if (String::equals("x86", m_arch))
	{
		arch = tune = "i686";
		flags = "-m32";
	}
	else if (String::equals(m_arch86, m_arch))
	{
		arch = tune = m_arch;
		flags = "-m32";
	}
	else
	{
		arch = tune = m_arch;
		flags = "-m64";
	}

	if (!arch.empty())
	{
		auto archFlag = fmt::format("-march={}", arch);
		if (isFlagSupported(archFlag))
			outArgList.push_back(std::move(archFlag));
	}

	if (!tune.empty())
	{
		auto tuneFlag = fmt::format("-mtune={}", tune);
		if (isFlagSupported(tuneFlag))
			outArgList.push_back(std::move(tuneFlag));
	}

	if (!flags.empty())
	{
		if (isFlagSupported(flags))
			outArgList.push_back(std::move(flags));
	}

	return true;
}

/*****************************************************************************/
void CompileToolchainGNU::addStripSymbolsOption(StringList& outArgList) const
{
	if (m_state.configuration.stripSymbols())
	{
		std::string strip{ "-s" };
		if (isFlagSupported(strip))
			outArgList.push_back(std::move(strip));
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addLinkerOptions(StringList& outArgList) const
{
	for (auto& option : m_project.linkerOptions())
	{
		if (isFlagSupported(option))
			outArgList.push_back(option);
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addProfileInformationLinkerOption(StringList& outArgList) const
{
	const bool enableProfiling = m_state.configuration.enableProfiling();
	if (enableProfiling && m_project.isExecutable())
	{
		outArgList.push_back("-Wl,--allow-multiple-definition");

		std::string profileInfo{ "-pg" };
		if (isFlagSupported(profileInfo))
			outArgList.push_back(std::move(profileInfo));
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addLinkTimeOptimizationOption(StringList& outArgList) const
{
	auto& configuration = m_state.configuration;
	const bool enableProfiling = configuration.enableProfiling();
	const bool debugSymbols = configuration.debugSymbols();

	if (!enableProfiling && !debugSymbols && configuration.linkTimeOptimization())
	{
		std::string lto{ "-flto" };
		if (isFlagSupported(lto))
			List::addIfDoesNotExist(outArgList, std::move(lto));
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addThreadModelLinkerOption(StringList& outArgList) const
{
	auto threadType = m_project.threadType();
	if (threadType == ThreadType::Posix || threadType == ThreadType::Auto)
	{
		if (m_config.isMingw() && m_project.staticLinking())
		{
			outArgList.push_back("-Wl,-Bstatic,--whole-archive");
			outArgList.push_back("-lwinpthread");
			outArgList.push_back("-Wl,--no-whole-archive");
		}
		else
		{
			List::addIfDoesNotExist(outArgList, "-pthread");
		}
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addLinkerScripts(StringList& outArgList) const
{
	const auto& linkerScript = m_project.linkerScript();
	if (!linkerScript.empty())
	{
		outArgList.push_back("-T");
		outArgList.push_back(linkerScript);
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addLibStdCppLinkerOption(StringList& outArgList) const
{
	// Not used in GCC
	UNUSED(outArgList);
}

/*****************************************************************************/
void CompileToolchainGNU::addStaticCompilerLibraryOptions(StringList& outArgList) const
{
	// List::addIfDoesNotExist(outArgList, "-libstdc++");

	if (m_project.staticLinking())
	{
		auto addFlag = [&](std::string flag) {
			if (isFlagSupported(flag))
				List::addIfDoesNotExist(outArgList, std::move(flag));
		};

		addFlag("-static-libgcc");
		addFlag("-static-libasan");
		addFlag("-static-libtsan");
		addFlag("-static-liblsan");
		addFlag("-static-libubsan");
		addFlag("-static-libstdc++");
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addPlatformGuiApplicationFlag(StringList& outArgList) const
{
	if (m_config.isMingwGcc())
	{
		const bool debugSymbols = m_state.configuration.debugSymbols();
		const ProjectKind kind = m_project.kind();
		if (kind == ProjectKind::DesktopApplication && !debugSymbols)
		{
			// TODO: check other windows specific options
			std::string mWindows{ "-mwindows" };
			if (isFlagSupported(mWindows))
				List::addIfDoesNotExist(outArgList, std::move(mWindows));
		}
	}
}

/*****************************************************************************/
// Linking (Misc)
/*****************************************************************************/
/*****************************************************************************/
void CompileToolchainGNU::startStaticLinkGroup(StringList& outArgList) const
{
	outArgList.push_back("-Wl,--copy-dt-needed-entries");
	outArgList.push_back("-Wl,-Bstatic");
	outArgList.push_back("-Wl,--start-group");
}

void CompileToolchainGNU::endStaticLinkGroup(StringList& outArgList) const
{
	outArgList.push_back("-Wl,--end-group");
}

/*****************************************************************************/
void CompileToolchainGNU::startExplicitDynamicLinkGroup(StringList& outArgList) const
{
	outArgList.push_back("-Wl,-Bdynamic");
}

/*****************************************************************************/
void CompileToolchainGNU::addCompilerSearchPaths(StringList& outArgList) const
{
	// Same as addLinks, but with -B, so far, just used in specific gcc calls
	const std::string prefix{ "-B" };
	for (const auto& dir : m_project.libDirs())
	{
		outArgList.push_back(getPathCommand(prefix, dir));
	}

	outArgList.push_back(getPathCommand(prefix, m_state.paths.buildOutputDir()));

#if !defined(CHALET_WIN32)
	// must be last
	List::addIfDoesNotExist(outArgList, getPathCommand(prefix, "/usr/local/lib/"));
#endif
}

/*****************************************************************************/
// Objective-C / Objective-C++
/*****************************************************************************/
/*****************************************************************************/
void CompileToolchainGNU::addObjectiveCxxLink(StringList& outArgList) const
{
	const std::string prefix{ "-l" };
	if (m_project.objectiveCxx())
	{
		std::string objc = prefix + "objc";
		List::addIfDoesNotExist(outArgList, std::move(objc));
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addObjectiveCxxCompileOption(StringList& outArgList, const CxxSpecialization specialization) const
{
	// Used by AppleClang
	UNUSED(outArgList, specialization);
}

/*****************************************************************************/
void CompileToolchainGNU::addObjectiveCxxRuntimeOption(StringList& outArgList, const CxxSpecialization specialization) const
{
	const bool isObjCxx = specialization == CxxSpecialization::ObjectiveCpp || specialization == CxxSpecialization::ObjectiveC;
	if (isObjCxx)
	{
#if defined(CHALET_MACOS)
		std::string objcRuntime{ "-fnext-runtime" };
#else
		std::string objcRuntime{ "-fgnu-runtime" };
#endif
		if (isFlagSupported(objcRuntime))
			List::addIfDoesNotExist(outArgList, std::move(objcRuntime));
	}
}

/*****************************************************************************/
// MacOS
/*****************************************************************************/
/*****************************************************************************/
void CompileToolchainGNU::addMacosSysRootOption(StringList& outArgList) const
{
#if defined(CHALET_MACOS)
	// TODO: Test Homebrew LLVM/GCC with this
	outArgList.push_back("-isysroot");
	outArgList.push_back(m_state.tools.applePlatformSdk("macosx"));
#else
	UNUSED(outArgList);
#endif
}

/*****************************************************************************/
void CompileToolchainGNU::addMacosFrameworkOptions(StringList& outArgList) const
{
#if defined(CHALET_MACOS)
	// TODO: Test Homebrew LLVM/GCC with this
	{
		const std::string prefix = "-F";
		for (auto& path : m_project.macosFrameworkPaths())
		{
			outArgList.push_back(prefix + path);
		}
	}
	{
		// const std::string suffix = ".framework";
		for (auto& framework : m_project.macosFrameworks())
		{
			outArgList.push_back("-framework");
			outArgList.push_back(framework);
			// outArgList.push_back(framework + suffix);
		}
	}
#else
	UNUSED(outArgList);
#endif
}

/*****************************************************************************/
void CompileToolchainGNU::initializeArchPresets()
{
	// https://gcc.gnu.org/onlinedocs/gcc/x86-Options.html
	if (m_arch86.empty())
	{
		m_arch86 = {
			"i386",
			"i486",
			"i586",
			"pentium",
			"lakemont",
			"ptentium-mmx",
			"pentiumpro",
			"i686",
			"pentium2",
			"pentium3",
			"pentium3m",
			"pentium-m",
			"pentium4m",
			"prescott",
			"k6",
			"k6-2",
			"k6-3",
			"athlon",
			"athlon-tbird",
			"athlon-4",
			"athlon-xp",
			"athlon-mp",
			"winchip-c6",
			"winchip2",
			"c3",
			"c3-2",
			"c7",
			"samuel-2",
			"nehemiah",
			"esther",
			"geode",
		};
	}
	/*if (m_arch86_64.empty())
	{
		m_arch86_64 = {
			"x86-64",
			"x86-64-v2",
			"x86-64-v3",
			"x86-64-v4",
			"k8",
			"opteron",
			"athlon64",
			"athlon-fx",
			"k8-sse3",
			"opteron-sse3",
			"athlon64-sse3",
			"amdfam10",
			"barcelona",
			"bdver1",
			"bdver2",
			"bdver3",
			"bdver4",
			"znver1",
			"znver2",
			"znver3",
			"btver1",
			"btver2",
			"eden-x2",
			"eden-x4",
			"nano",
			"nano-1000",
			"nano-2000",
			"nano-3000",
			"nano-x2",
			"nano-x4",
		};
	}*/
}

/*****************************************************************************/
void CompileToolchainGNU::initializeSupportedLinks()
{
	bool oldQuotePaths = m_quotePaths;
	m_quotePaths = false;

	// TODO: Links coming from CMake projects

	StringList cmakeProjects;
	StringList projectLinks;
	for (auto& target : m_state.targets)
	{
		if (target->isProject())
		{
			auto& project = static_cast<const ProjectTarget&>(*target);
			if (project.isExecutable())
				continue;

			auto file = project.name();
			if (project.isStaticLibrary())
			{
				file += "-s";
			}
			projectLinks.push_back(std::move(file));
		}
		else if (target->isCMake())
		{
			// Try to guess based on the project name
			cmakeProjects.push_back(target->name());
		}
	}

	StringList libDirs;
	addCompilerSearchPaths(libDirs); // do not quote paths for this

	auto excludes = getLinkExclusions();

	for (auto& staticLink : m_project.staticLinks())
	{
		if (m_config.isLinkSupported(staticLink, libDirs) || List::contains(projectLinks, staticLink) || String::contains(cmakeProjects, staticLink))
			m_supportedLinks.emplace(staticLink, true);
	}
	for (auto& link : m_project.links())
	{
		if (List::contains(excludes, link))
			continue;

		if (m_config.isLinkSupported(link, libDirs) || List::contains(projectLinks, link) || String::contains(cmakeProjects, link))
			m_supportedLinks.emplace(link, true);
	}

	m_quotePaths = oldQuotePaths;

	m_supportedLinksInitialized = true;
}
}
