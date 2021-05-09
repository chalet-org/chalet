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

	// TODO: no need for this during CI build
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

	const auto& rc = m_state.compilers.rc();
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

	const auto& ar = m_state.tools.ar();
	ret.push_back(ar);

	ret.push_back("-c");
	ret.push_back("-r");
	ret.push_back("-s");

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
void CompileToolchainGNU::addSourceObjects(StringList& inArgList, const StringList& sourceObjs) const
{
	for (auto& source : sourceObjs)
	{
		inArgList.push_back(source);
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addIncludes(StringList& inArgList) const
{
	const std::string prefix{ "-I" };
	for (const auto& dir : m_project.includeDirs())
	{
		if (m_quotePaths)
			inArgList.push_back(fmt::format("{}\"{}\"", prefix, dir));
		else
			inArgList.push_back(prefix + dir);
	}
	for (const auto& dir : m_project.locations())
	{
		if (m_quotePaths)
			inArgList.push_back(fmt::format("{}\"{}\"", prefix, dir));
		else
			inArgList.push_back(prefix + dir);
	}

#if !defined(CHALET_WIN32)
	// must be last
	std::string localInclude = prefix;
	if (m_quotePaths)
		localInclude += "\"/usr/local/include/\"";
	else
		localInclude += "/usr/local/include/";
	List::addIfDoesNotExist(inArgList, std::move(localInclude));
#endif
}

/*****************************************************************************/
void CompileToolchainGNU::addLibDirs(StringList& inArgList) const
{
	const std::string prefix{ "-L" };
	for (const auto& dir : m_project.libDirs())
	{
		if (m_quotePaths)
			inArgList.push_back(fmt::format("{}\"{}\"", prefix, dir));
		else
			inArgList.push_back(prefix + dir);
	}

	inArgList.push_back(prefix + m_state.paths.buildOutputDir());

#if !defined(CHALET_WIN32)
	// must be last
	std::string localLib = prefix;
	if (m_quotePaths)
		localLib += "\"/usr/local/lib/\"";
	else
		localLib += "/usr/local/lib/";
	List::addIfDoesNotExist(inArgList, std::move(localLib));
#endif
}

/*****************************************************************************/
void CompileToolchainGNU::addWarnings(StringList& inArgList) const
{
	const std::string prefix{ "-W" };
	for (auto& warning : m_project.warnings())
	{
		if (String::equals(warning, "pedantic-errors"))
		{
			inArgList.push_back("-" + warning);
			continue;
		}
		inArgList.push_back(prefix + warning);
	}

	if (m_project.usesPch())
	{
		std::string invalidPch = prefix + "invalid-pch";
		List::addIfDoesNotExist(inArgList, std::move(invalidPch));
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addDefines(StringList& inArgList) const
{
	const std::string prefix{ "-D" };
	for (auto& define : m_project.defines())
	{
		inArgList.push_back(prefix + define);
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addLinks(StringList& inArgList) const
{
	const std::string prefix{ "-l" };
	const bool hasStaticLinks = m_project.staticLinks().size() > 0;
	const bool hasDynamicLinks = m_project.links().size() > 0;

	if (hasStaticLinks)
	{
		startStaticLinkGroup(inArgList);

		for (auto& staticLink : m_project.staticLinks())
		{
			inArgList.push_back(prefix + staticLink);
		}

		endStaticLinkGroup(inArgList);
	}

	if (hasDynamicLinks)
	{
		if (hasStaticLinks)
			startExplicitDynamicLinkGroup(inArgList);

		for (auto& link : m_project.links())
		{
			inArgList.push_back(prefix + link);
		}
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addPchInclude(StringList& inArgList) const
{
	// TODO: Potential for more than one pch?
	if (m_project.usesPch())
	{
		const auto objDirPch = m_state.paths.getPrecompiledHeaderInclude(m_project);

		inArgList.push_back("-include");
		if (m_quotePaths)
			inArgList.push_back(fmt::format("\"{}\"", objDirPch));
		else
			inArgList.push_back(objDirPch);
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addOptimizationOption(StringList& inArgList) const
{
	std::string opt;
	auto& configuration = m_state.configuration;
	opt = configuration.optimizations();

	if (configuration.debugSymbols())
	{
		// force -O0 (set to anything else here would be in error)
		if (opt != "-Og" && opt != "-O0")
			opt = "-O0";
	}

	inArgList.push_back(std::move(opt));
}

/*****************************************************************************/
void CompileToolchainGNU::addRunPath(StringList& inArgList) const
{
#if defined(CHALET_LINUX)
	inArgList.push_back("-Wl,-rpath,'$$ORIGIN'"); // Note: Single quotes are required!
#else
	UNUSED(inArgList);
#endif
}

/*****************************************************************************/
void CompileToolchainGNU::addLanguageStandard(StringList& inArgList, const CxxSpecialization specialization) const
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
		inArgList.push_back(std::move(ret));
	}
#else
	static std::regex regex{ "^(((c|gnu)\\+\\+|gnu|c|iso9899:)(\\d[\\dzaxy]{1,3}|199409))$" };
	if (std::regex_match(ret, regex))
	{
		ret = "-std=" + ret;
		inArgList.push_back(std::move(ret));
	}
#endif
}

/*****************************************************************************/
void CompileToolchainGNU::addDebuggingInformationOption(StringList& inArgList) const
{
	// TODO: Control debugging information level (g, g0-g3) from configurations
	if (m_state.configuration.debugSymbols())
	{
		inArgList.push_back("-g3");
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addProfileInformationCompileOption(StringList& inArgList) const
{
	// TODO: gcc/clang distinction on mac?

	if (m_state.configuration.enableProfiling())
	{
		if (!m_project.isSharedLibrary())
			inArgList.push_back("-pg");
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addCompileOptions(StringList& inArgList) const
{
	for (auto& option : m_project.compileOptions())
	{
		List::addIfDoesNotExist(inArgList, option);
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addDiagnosticColorOption(StringList& inArgList) const
{
	List::addIfDoesNotExist(inArgList, "-fdiagnostics-color=always");
}

/*****************************************************************************/
void CompileToolchainGNU::addLibStdCppCompileOption(StringList& inArgList, const CxxSpecialization specialization) const
{
	UNUSED(inArgList, specialization);
}

/*****************************************************************************/
void CompileToolchainGNU::addPositionIndependentCodeOption(StringList& inArgList) const
{
	List::addIfDoesNotExist(inArgList, "-fPIC");
}

/*****************************************************************************/
void CompileToolchainGNU::addNoRunTimeTypeInformationOption(StringList& inArgList) const
{
	if (!m_project.rtti())
	{
		List::addIfDoesNotExist(inArgList, "-fno-rtti");
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addThreadModelCompileOption(StringList& inArgList) const
{
	if (m_project.posixThreads())
	{
		List::addIfDoesNotExist(inArgList, "-pthread");
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addStripSymbolsOption(StringList& inArgList) const
{
	if (m_state.configuration.stripSymbols())
	{
		inArgList.push_back("-s");
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addLinkerOptions(StringList& inArgList) const
{
	for (auto& option : m_project.linkerOptions())
	{
		inArgList.push_back(option);
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addProfileInformationLinkerOption(StringList& inArgList) const
{
	const bool enableProfiling = m_state.configuration.enableProfiling();
	if (enableProfiling && m_project.isExecutable())
	{
		inArgList.push_back("-Wl,--allow-multiple-definition");
		inArgList.push_back("-pg");
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addLinkTimeOptimizationOption(StringList& inArgList) const
{
	auto& configuration = m_state.configuration;
	const bool enableProfiling = configuration.enableProfiling();
	const bool debugSymbols = configuration.debugSymbols();

	if (!enableProfiling && !debugSymbols && configuration.linkTimeOptimization())
	{
		List::addIfDoesNotExist(inArgList, "-flto");
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addThreadModelLinkerOption(StringList& inArgList) const
{
	if (m_project.posixThreads())
	{
		if (m_config.isMingw() && m_project.staticLinking())
		{
			inArgList.push_back("-Wl,-Bstatic");
			inArgList.push_back("-lstdc++");
			inArgList.push_back("-lpthread");
		}
		else
		{
			List::addIfDoesNotExist(inArgList, "-pthread");
		}
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addLinkerScripts(StringList& inArgList) const
{
	const auto& linkerScript = m_project.linkerScript();
	if (!linkerScript.empty())
	{
		inArgList.push_back("-T");
		inArgList.push_back(linkerScript);
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addLibStdCppLinkerOption(StringList& inArgList) const
{
	// Not used in GCC
	UNUSED(inArgList);
}

/*****************************************************************************/
void CompileToolchainGNU::addStaticCompilerLibraryOptions(StringList& inArgList) const
{
	// List::addIfDoesNotExist(inArgList, "-libstdc++");

	if (m_project.staticLinking())
	{
		List::addIfDoesNotExist(inArgList, "-static-libgcc");
		List::addIfDoesNotExist(inArgList, "-static-libasan");
		List::addIfDoesNotExist(inArgList, "-static-libtsan");
		List::addIfDoesNotExist(inArgList, "-static-liblsan");
		List::addIfDoesNotExist(inArgList, "-static-libubsan");
		List::addIfDoesNotExist(inArgList, "-static-libstdc++");
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addPlatformGuiApplicationFlag(StringList& inArgList) const
{
	if (m_config.isMingwGcc())
	{
		const bool debugSymbols = m_state.configuration.debugSymbols();
		const ProjectKind kind = m_project.kind();
		if (kind == ProjectKind::DesktopApplication && !debugSymbols)
		{
			// TODO: check other windows specific options
			List::addIfDoesNotExist(inArgList, "-mwindows");
		}
	}
}

/*****************************************************************************/
// Linking (Misc)
/*****************************************************************************/
/*****************************************************************************/
void CompileToolchainGNU::startStaticLinkGroup(StringList& inArgList) const
{
	inArgList.push_back("-Wl,--copy-dt-needed-entries");
	inArgList.push_back("-Wl,-Bstatic");
	inArgList.push_back("-Wl,--start-group");
}

void CompileToolchainGNU::endStaticLinkGroup(StringList& inArgList) const
{
	inArgList.push_back("-Wl,--end-group");
}

void CompileToolchainGNU::startExplicitDynamicLinkGroup(StringList& inArgList) const
{
	inArgList.push_back("-Wl,-Bdynamic");
}

/*****************************************************************************/
// Objective-C / Objective-C++
/*****************************************************************************/
/*****************************************************************************/
void CompileToolchainGNU::addObjectiveCxxLink(StringList& inArgList) const
{
	const std::string prefix{ "-l" };
	if (m_project.objectiveCxx())
	{
		std::string objc = prefix + "objc";
		List::addIfDoesNotExist(inArgList, std::move(objc));
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addObjectiveCxxCompileOption(StringList& inArgList, const CxxSpecialization specialization) const
{
	// Used by AppleClang
	UNUSED(inArgList, specialization);
}

/*****************************************************************************/
void CompileToolchainGNU::addObjectiveCxxRuntimeOption(StringList& inArgList, const CxxSpecialization specialization) const
{
	const bool isObjCxx = specialization == CxxSpecialization::ObjectiveCpp || specialization == CxxSpecialization::ObjectiveC;
	if (isObjCxx)
	{
#if defined(CHALET_MACOS)
		List::addIfDoesNotExist(inArgList, "-fnext-runtime");
#else
		List::addIfDoesNotExist(inArgList, "-fgnu-runtime");
#endif
	}
}

/*****************************************************************************/
// MacOS
/*****************************************************************************/
/*****************************************************************************/
void CompileToolchainGNU::addMacosSysRootOption(StringList& inArgList) const
{
#if defined(CHALET_MACOS)
	// TODO: Test Homebrew LLVM/GCC with this
	inArgList.push_back("-isysroot");
	inArgList.push_back(m_state.tools.macosSdk());
#else
	UNUSED(inArgList);
#endif
}

/*****************************************************************************/
void CompileToolchainGNU::addMacosFrameworkOptions(StringList& inArgList) const
{
#if defined(CHALET_MACOS)
	// TODO: Test Homebrew LLVM/GCC with this
	{
		const std::string prefix = "-F";
		for (auto& path : m_project.macosFrameworkPaths())
		{
			inArgList.push_back(prefix + path);
		}
	}
	{
		// const std::string suffix = ".framework";
		for (auto& framework : m_project.macosFrameworks())
		{
			inArgList.push_back("-framework");
			inArgList.push_back(framework);
			// inArgList.push_back(framework + suffix);
		}
	}
#else
	UNUSED(inArgList);
#endif
}
}
