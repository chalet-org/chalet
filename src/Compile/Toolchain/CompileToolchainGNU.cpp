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
}

/*****************************************************************************/
ToolchainType CompileToolchainGNU::type() const
{
	return ToolchainType::GNU;
}

/*****************************************************************************/
std::string CompileToolchainGNU::getAsmGenerateCommand(const std::string& inputFile, const std::string& outputFile)
{
	// TODO: Customizations for these commands
#if defined(CHALET_MACOS)
	const auto& otool = m_state.tools.otool();
	return fmt::format("{otool} -tvV {inputFile} | c++filt > {outputFile}",
		FMT_ARG(otool),
		FMT_ARG(inputFile),
		FMT_ARG(outputFile));
#else
	return fmt::format("objdump -d -C -Mintel {inputFile} > {outputFile}",
		FMT_ARG(inputFile),
		FMT_ARG(outputFile));
#endif
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
	addCompileFlags(ret, true, specialization);
	addDefines(ret);
	addIncludes(ret);

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

	addCompileFlags(ret, false, specialization);
	addDefines(ret);
	addIncludes(ret);

	if (specialization == CxxSpecialization::C || specialization == CxxSpecialization::Cpp)
		addPchInclude(ret);

	ret.push_back("-o");
	ret.push_back(outputFile);
	ret.push_back("-c");
	ret.push_back(inputFile);

	return ret;
}

/*****************************************************************************/
// TODO: sourceObs needs to be a list
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

	addLinkerOptions(ret);
	addLibDirs(ret);

	ret.push_back("-o");
	ret.push_back(outputFile);
	addSourceObjects(ret, sourceObjs);

	addLinks(ret);

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
	ret.push_back("-undefined");
	ret.push_back("suppress");
	ret.push_back("-flat_namespace");

	addLinkerOptions(ret);
	addLibDirs(ret);

	ret.push_back("-o");
	ret.push_back(outputFile);
	addSourceObjects(ret, sourceObjs);

	addLinks(ret);

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

	addLinkerOptions(ret);
	addLibDirs(ret);

	ret.push_back("-o");
	ret.push_back(outputFile);
	addSourceObjects(ret, sourceObjs);

	addLinks(ret);

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
	addLinkerOptions(ret);

	return ret;
}

/*****************************************************************************/
void CompileToolchainGNU::addSourceObjects(StringList& inArgList, const StringList& sourceObjs)
{
	for (auto& source : sourceObjs)
	{
		inArgList.push_back(source);
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addIncludes(StringList& inArgList)
{
	const std::string prefix = "-I";
	for (auto& dir : m_project.includeDirs())
	{
		inArgList.push_back(prefix + dir);
	}
	for (auto& dir : m_project.locations())
	{
		inArgList.push_back(prefix + dir);
	}

#if !defined(CHALET_WIN32)
	if (!m_config.isMingw())
	{
		// must be last
		std::string localInclude = prefix + "/usr/local/include";
		List::addIfDoesNotExist(inArgList, std::move(localInclude));
	}
#endif

#if defined(CHALET_MACOS)
	inArgList.push_back("-isysroot");
	inArgList.push_back(m_state.tools.macosSdk());
#endif
}

/*****************************************************************************/
void CompileToolchainGNU::addLibDirs(StringList& inArgList)
{
	const std::string prefix = "-L";
	for (auto& dir : m_project.libDirs())
	{
		inArgList.push_back(prefix + dir);
	}

	inArgList.push_back(prefix + m_state.paths.buildOutputDir());

#if !defined(CHALET_WIN32)
	if (!m_config.isMingw())
	{
		// must be last
		std::string localLib = prefix + "/usr/local/lib";
		List::addIfDoesNotExist(inArgList, std::move(localLib));
	}
#endif
}

/*****************************************************************************/
void CompileToolchainGNU::addWarnings(StringList& inArgList)
{
	const std::string prefix = "-W";
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
void CompileToolchainGNU::addDefines(StringList& inArgList)
{
	const std::string prefix = "-D";
	for (auto& define : m_project.defines())
	{
		inArgList.push_back(prefix + define);
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addLinks(StringList& inArgList)
{
	const std::string prefix = "-l";
	bool hasStatic = false;

	if (m_project.staticLinks().size() > 0)
	{
		if (!m_config.isAppleClang())
		{
			inArgList.push_back("-Wl,--copy-dt-needed-entries");
			inArgList.push_back("-Wl,-Bstatic");
			inArgList.push_back("-Wl,--start-group");
			hasStatic = true;
		}

		for (auto& staticLink : m_project.staticLinks())
		{
			inArgList.push_back(prefix + staticLink);
		}

		if (hasStatic)
			inArgList.push_back("-Wl,--end-group");
	}

	if (m_project.links().size() > 0)
	{
		if (hasStatic)
			inArgList.push_back("-Wl,-Bdynamic");

		for (auto& link : m_project.links())
		{
			inArgList.push_back(prefix + link);
		}
	}

	if (m_project.objectiveCxx() && !m_config.isAppleClang())
	{
		std::string objc = prefix + "objc";
		List::addIfDoesNotExist(inArgList, std::move(objc));
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addPchInclude(StringList& inArgList)
{
	// TODO: Potential for more than one pch?
	if (m_project.usesPch())
	{
		const auto objDirPch = m_state.paths.getPrecompiledHeaderInclude(m_project);
		inArgList.push_back("-include");
		inArgList.push_back(objDirPch);
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addOptimizationFlag(StringList& inArgList)
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
void CompileToolchainGNU::addStripSymbols(StringList& inArgList)
{
	if (!m_config.isClang())
	{
		if (m_state.configuration.stripSymbols())
		{
			inArgList.push_back("-s");
		}
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addRunPath(StringList& inArgList)
{
#if defined(CHALET_LINUX)
	inArgList.push_back("-Wl,-R$$ORIGIN"); // TODO: This originally had single quotes around it... might be necessary
#else
	UNUSED(inArgList);
#endif
}

/*****************************************************************************/
void CompileToolchainGNU::addLanguageStandard(StringList& inArgList, const CxxSpecialization specialization)
{
	const bool useC = m_project.language() == CodeLanguage::C || specialization == CxxSpecialization::ObjectiveC;
	const auto& langStandard = useC ? m_project.cStandard() : m_project.cppStandard();
	std::string ret = String::toLowerCase(langStandard);

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
void CompileToolchainGNU::addCompileFlags(StringList& inArgList, const bool forPch, const CxxSpecialization specialization)
{
	addOptimizationFlag(inArgList);

	const bool debugSymbols = m_state.configuration.debugSymbols();
	const bool enableProfiling = m_state.configuration.enableProfiling();

	if (debugSymbols)
	{
		inArgList.push_back("-g3");
	}

	addLanguageStandard(inArgList, specialization);
	addWarnings(inArgList);
	if (!forPch)
		addObjectiveCxxCompileFlag(inArgList, specialization);

	addOtherCompileOptions(inArgList, specialization);

	if (enableProfiling)
	{
		if (!m_config.isAppleClang())
		{
			// -pg not supported in apple clang
			// TODO: gcc/clang distinction on mac?

			if (!m_project.isSharedLibrary())
				inArgList.push_back("-pg");
		}
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addObjectiveCxxCompileFlag(StringList& inArgList, const CxxSpecialization specialization)
{
	const bool isObjCpp = specialization == CxxSpecialization::ObjectiveCpp;
	const bool isObjC = specialization == CxxSpecialization::ObjectiveC;
	const bool isObjCxx = specialization == CxxSpecialization::ObjectiveCpp || specialization == CxxSpecialization::ObjectiveC;
	if (m_project.objectiveCxx() && m_config.isAppleClang() && isObjCxx)
	{
		inArgList.push_back("-x");
		if (isObjCpp)
			inArgList.push_back("objective-c++");
		else if (isObjC)
			inArgList.push_back("objective-c");
	}
}

/*****************************************************************************/
void CompileToolchainGNU::addOtherCompileOptions(StringList& inArgList, const CxxSpecialization specialization)
{
	if (m_config.isGcc())
		List::addIfDoesNotExist(inArgList, "-fPIC");

	for (auto& option : m_project.compileOptions())
	{
		if (String::equals(option, "-fPIC"))
			continue;

		inArgList.push_back(option);
	}

	const bool isObjCxx = specialization == CxxSpecialization::ObjectiveCpp || specialization == CxxSpecialization::ObjectiveC;
	if (isObjCxx && !m_config.isAppleClang())
	{
#if defined(CHALET_MACOS)
		List::addIfDoesNotExist(inArgList, "-fnext-runtime");
#else
		List::addIfDoesNotExist(inArgList, "-fgnu-runtime");
#endif
	}

	List::addIfDoesNotExist(inArgList, "-fdiagnostics-color=always");

	if (!m_project.rtti() && !isObjCxx)
	{
		List::addIfDoesNotExist(inArgList, "-fno-rtti");
	}

	// #if defined(CHALET_LINUX)
	if (m_config.isGcc())
	{
		if (m_project.posixThreads())
			List::addIfDoesNotExist(inArgList, "-pthread");
	}
	// #endif
}

/*****************************************************************************/
void CompileToolchainGNU::addLinkerOptions(StringList& inArgList)
{
	auto& configuration = m_state.configuration;
	const bool debugSymbols = configuration.debugSymbols();
	const bool enableProfiling = m_state.configuration.enableProfiling();

	addStripSymbols(inArgList);

	for (auto& option : m_project.linkerOptions())
	{
		inArgList.push_back(option);
	}

#if defined(CHALET_MACOS)
	// TODO: Test Homebrew LLVM/GCC with this
	inArgList.push_back("-isysroot");
	inArgList.push_back(m_state.tools.macosSdk());
#endif

	if (enableProfiling)
	{
		if (!m_config.isAppleClang() && m_project.isExecutable())
		{
			inArgList.push_back("-Wl,--allow-multiple-definition");
			List::addIfDoesNotExist(inArgList, "-pg");
		}
	}
	else if (!debugSymbols && !m_config.isAppleClang())
	{
		if (configuration.linkTimeOptimization())
		{
			List::addIfDoesNotExist(inArgList, "-flto");
		}
	}

	// #if defined(CHALET_LINUX)
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
	// #endif

	// TODO: Check if there's a clang/apple clang version of this
	if (!m_config.isClang())
	{
		const auto& linkerScript = m_project.linkerScript();
		if (!linkerScript.empty())
		{
			inArgList.push_back("-T");
			inArgList.push_back(linkerScript);
		}
	}

	if (m_project.staticLinking())
	{
		// List::addIfDoesNotExist(inArgList, "-libstdc++");

		if (m_config.isClang())
		{
			List::addIfDoesNotExist(inArgList, "-static-libsan");

			// TODO: Investigate for other -static candidates on clang/mac
		}
		else
		{
			List::addIfDoesNotExist(inArgList, "-static-libgcc");
			List::addIfDoesNotExist(inArgList, "-static-libasan");
			List::addIfDoesNotExist(inArgList, "-static-libtsan");
			List::addIfDoesNotExist(inArgList, "-static-liblsan");
			List::addIfDoesNotExist(inArgList, "-static-libubsan");
			List::addIfDoesNotExist(inArgList, "-static-libstdc++");
		}
	}

	if (m_config.isMingwGcc())
	{
		const ProjectKind kind = m_project.kind();
		if (kind == ProjectKind::DesktopApplication && !debugSymbols)
		{
			// TODO: check other windows specific options
			List::addIfDoesNotExist(inArgList, "-mwindows");
		}
	}

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
		const std::string suffix = ".framework";
		for (auto& framework : m_project.macosFrameworks())
		{
			inArgList.push_back("-framework");
			inArgList.push_back(framework + suffix);
		}
	}
#endif
}
}
