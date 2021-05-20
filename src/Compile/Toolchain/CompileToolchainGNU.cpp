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
CompileToolchainGNU::CompileToolchainGNU(const BuildState& inState, const ProjectConfiguration& inProject, const CompilerConfig& inConfig) :
	m_state(inState),
	m_project(inProject),
	m_config(inConfig),
	m_compilerType(m_config.compilerType())
{
	m_quotePaths = m_state.environment.strategy() != StrategyType::Native;
}

/*****************************************************************************/
ToolchainType CompileToolchainGNU::type() const
{
	return ToolchainType::GNU;
}

/*****************************************************************************/
StringList CompileToolchainGNU::getPchCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency)
{
	StringList ret;

	const auto& cc = m_config.compilerExecutable();
	ret.push_back(cc);

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

	const auto& rc = m_state.compilerTools.rc();
	ret.push_back(rc);

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

	const auto& cc = m_config.compilerExecutable();
	ret.push_back(cc);

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
			// TODO: any difference in MinGW Clang vs GCC
			if (m_config.isMingw())
				return getMingwDllTargetCommand(outputFile, sourceObjs, outputFileBase);
			else if (m_config.isClang())
				return getDylibTargetCommand(outputFile, sourceObjs);
			else
				return getDynamicLibTargetCommand(outputFile, sourceObjs);
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
StringList CompileToolchainGNU::getMingwDllTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase)
{
	StringList ret;

	const auto& cc = m_config.compilerExecutable();
	ret.push_back(cc);

	ret.push_back("-shared");
	{
		std::string mingwLinkerOptions;
		if (m_project.windowsOutputDef())
		{
			mingwLinkerOptions = fmt::format("-Wl,--output-def={}.def", outputFileBase);
		}
		mingwLinkerOptions += fmt::format("-Wl,--out-implib={}.a", outputFileBase);
		ret.push_back(std::move(mingwLinkerOptions));
	}
	ret.push_back("-Wl,--dll");

	addStripSymbolsOption(ret);
	addLinkerOptions(ret);
	addMacosSysRootOption(ret);
	addProfileInformationLinkerOption(ret);
	addLinkTimeOptimizationOption(ret);
	addThreadModelLinkerOption(ret);
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
StringList CompileToolchainGNU::getDylibTargetCommand(const std::string& outputFile, const StringList& sourceObjs)
{
	StringList ret;

	const auto& cc = m_config.compilerExecutable();
	ret.push_back(cc);

	ret.push_back("-dynamiclib");
	// ret.push_back("-fPIC");
	// ret.push_back("-flat_namespace");

	addStripSymbolsOption(ret);
	addLinkerOptions(ret);
	addMacosSysRootOption(ret);
	addProfileInformationLinkerOption(ret);
	addLinkTimeOptimizationOption(ret);
	addThreadModelLinkerOption(ret);
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
StringList CompileToolchainGNU::getDynamicLibTargetCommand(const std::string& outputFile, const StringList& sourceObjs)
{
	StringList ret;

	const auto& cc = m_config.compilerExecutable();
	ret.push_back(cc);

	ret.push_back("-shared");
	ret.push_back("-fPIC");

	addStripSymbolsOption(ret);
	addLinkerOptions(ret);
	addMacosSysRootOption(ret);
	addProfileInformationLinkerOption(ret);
	addLinkTimeOptimizationOption(ret);
	addThreadModelLinkerOption(ret);
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
StringList CompileToolchainGNU::getStaticLibTargetCommand(const std::string& outputFile, const StringList& sourceObjs)
{
	StringList ret;

	auto& archiver = m_state.compilerTools.archiver();
	ret.push_back(archiver);

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
StringList CompileToolchainGNU::getExecutableTargetCommand(const std::string& outputFile, const StringList& sourceObjs)
{
	StringList ret;

	const auto& cc = m_config.compilerExecutable();
	ret.push_back(cc);

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
		if (m_quotePaths)
			outArgList.push_back(fmt::format("{}\"{}\"", prefix, dir));
		else
			outArgList.push_back(prefix + dir);
	}
	for (const auto& dir : m_project.locations())
	{
		if (m_quotePaths)
			outArgList.push_back(fmt::format("{}\"{}\"", prefix, dir));
		else
			outArgList.push_back(prefix + dir);
	}

#if !defined(CHALET_WIN32)
	// must be last
	std::string localInclude = prefix;
	if (m_quotePaths)
		localInclude += "\"/usr/local/include/\"";
	else
		localInclude += "/usr/local/include/";
	List::addIfDoesNotExist(outArgList, std::move(localInclude));
#endif
}

/*****************************************************************************/
void CompileToolchainGNU::addLibDirs(StringList& outArgList) const
{
	const std::string prefix{ "-L" };
	for (const auto& dir : m_project.libDirs())
	{
		if (m_quotePaths)
			outArgList.push_back(fmt::format("{}\"{}\"", prefix, dir));
		else
			outArgList.push_back(prefix + dir);
	}

	outArgList.push_back(prefix + m_state.paths.buildOutputDir());

#if !defined(CHALET_WIN32)
	// must be last
	std::string localLib = prefix;
	if (m_quotePaths)
		localLib += "\"/usr/local/lib/\"";
	else
		localLib += "/usr/local/lib/";
	List::addIfDoesNotExist(outArgList, std::move(localLib));
#endif
}

/*****************************************************************************/
void CompileToolchainGNU::addWarnings(StringList& outArgList) const
{
	const std::string prefix{ "-W" };
	for (auto& warning : m_project.warnings())
	{
		if (String::equals(warning, "pedantic-errors"))
		{
			outArgList.push_back("-" + warning);
			continue;
		}
		outArgList.push_back(prefix + warning);
	}

	if (m_project.usesPch())
	{
		std::string invalidPch = prefix + "invalid-pch";
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
		if (m_quotePaths)
			outArgList.push_back(fmt::format("\"{}\"", objDirPch));
		else
			outArgList.push_back(objDirPch);
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
	{
		ret = "-std=" + ret;
		outArgList.push_back(std::move(ret));
	}
#else
	static std::regex regex{ "^(((c|gnu)\\+\\+|gnu|c|iso9899:)(\\d[\\dzaxy]{1,3}|199409))$" };
	if (std::regex_match(ret, regex))
	{
		ret = "-std=" + ret;
		outArgList.push_back(std::move(ret));
	}
#endif
}

/*****************************************************************************/
void CompileToolchainGNU::addDebuggingInformationOption(StringList& outArgList) const
{
	// TODO: Control debugging information level (g, g0-g3) from configurations
	if (m_state.configuration.debugSymbols())
	{
		outArgList.push_back("-g3");
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addProfileInformationCompileOption(StringList& outArgList) const
{
	// TODO: gcc/clang distinction on mac?

	if (m_state.configuration.enableProfiling())
	{
		if (!m_project.isSharedLibrary())
			outArgList.push_back("-pg");
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
	List::addIfDoesNotExist(outArgList, "-fdiagnostics-color=always");
}

/*****************************************************************************/
void CompileToolchainGNU::addLibStdCppCompileOption(StringList& outArgList, const CxxSpecialization specialization) const
{
	UNUSED(outArgList, specialization);
}

/*****************************************************************************/
void CompileToolchainGNU::addPositionIndependentCodeOption(StringList& outArgList) const
{
	List::addIfDoesNotExist(outArgList, "-fPIC");
}

/*****************************************************************************/
void CompileToolchainGNU::addNoRunTimeTypeInformationOption(StringList& outArgList) const
{
	if (!m_project.rtti())
	{
		List::addIfDoesNotExist(outArgList, "-fno-rtti");
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addThreadModelCompileOption(StringList& outArgList) const
{
	if (m_project.posixThreads())
	{
		List::addIfDoesNotExist(outArgList, "-pthread");
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addStripSymbolsOption(StringList& outArgList) const
{
	if (m_state.configuration.stripSymbols())
	{
		outArgList.push_back("-s");
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addLinkerOptions(StringList& outArgList) const
{
	for (auto& option : m_project.linkerOptions())
	{
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
		outArgList.push_back("-pg");
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
		List::addIfDoesNotExist(outArgList, "-flto");
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addThreadModelLinkerOption(StringList& outArgList) const
{
	if (m_project.posixThreads())
	{
		if (m_config.isMingw() && m_project.staticLinking())
		{
			outArgList.push_back("-Wl,-Bstatic");
			outArgList.push_back("-lstdc++");
			outArgList.push_back("-lpthread");
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
		List::addIfDoesNotExist(outArgList, "-static-libgcc");
		List::addIfDoesNotExist(outArgList, "-static-libasan");
		List::addIfDoesNotExist(outArgList, "-static-libtsan");
		List::addIfDoesNotExist(outArgList, "-static-liblsan");
		List::addIfDoesNotExist(outArgList, "-static-libubsan");
		List::addIfDoesNotExist(outArgList, "-static-libstdc++");
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
			List::addIfDoesNotExist(outArgList, "-mwindows");
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

void CompileToolchainGNU::startExplicitDynamicLinkGroup(StringList& outArgList) const
{
	outArgList.push_back("-Wl,-Bdynamic");
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
		List::addIfDoesNotExist(outArgList, "-fnext-runtime");
#else
		List::addIfDoesNotExist(outArgList, "-fgnu-runtime");
#endif
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
	outArgList.push_back(m_state.tools.macosSdk());
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
}
