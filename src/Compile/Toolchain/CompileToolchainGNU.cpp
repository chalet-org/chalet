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
const std::string& CompileToolchainGNU::getIncludes()
{
	if (m_includes.empty())
	{
		StringList includes = m_project.includeDirs();

		for (auto loc : m_project.locations())
		{
			if (loc.back() != '/')
				loc += "/";

			includes.push_back(loc);
		}

#if !defined(CHALET_WIN32)
		if (!m_config.isMingw())
		{
			// must be last
			std::string localInclude = "/usr/local/include";
			List::addIfDoesNotExist<std::string>(includes, std::move(localInclude));
		}
#endif

		m_includes = String::getPrefixed(includes, "-I");

#if defined(CHALET_MACOS)
		m_includes += fmt::format(" -isysroot {}", m_state.tools.macosSdk());
#endif
	}

	return m_includes;
}

/*****************************************************************************/
const std::string& CompileToolchainGNU::getLibDirs()
{
	if (m_libDirs.empty())
	{
		StringList libs = m_project.libDirs();

		const auto& buildOutputDir = m_state.paths.buildOutputDir();
		libs.push_back(buildOutputDir);

#if !defined(CHALET_WIN32)
		if (!m_config.isMingw())
		{
			// must be last
			std::string localLib = "/usr/local/lib";
			List::addIfDoesNotExist<std::string>(libs, std::move(localLib));
		}
#endif

		m_libDirs = String::getPrefixed(libs, "-L");
	}

	return m_libDirs;
}

/*****************************************************************************/
const std::string& CompileToolchainGNU::getWarnings()
{
	if (m_warnings.empty())
	{
		auto warnings = m_project.warnings();

		if (m_project.usesPch())
		{
			List::addIfDoesNotExist<std::string>(warnings, "invalid-pch");
		}

		m_warnings = String::getPrefixed(warnings, "-W");

		// Special
		String::replaceAll(m_warnings, "-Wpedantic-errors", "-pedantic-errors");
	}

	return m_warnings;
}

/*****************************************************************************/
const std::string& CompileToolchainGNU::getDefines()
{
	if (m_defines.empty())
	{
		m_defines = String::getPrefixed(m_project.defines(), "-D");
	}

	return m_defines;
}

/*****************************************************************************/
std::string CompileToolchainGNU::getLangStandard(const bool treatAsC)
{
	const bool useC = m_project.language() == CodeLanguage::C || treatAsC;
	const auto& langStandard = useC ? m_project.cStandard() : m_project.cppStandard();
	std::string ret = String::toLowerCase(langStandard);

#ifndef CHALET_MSVC
	static constexpr auto regex = ctll::fixed_string{ "^(((c|gnu)\\+\\+|gnu|c|iso9899:)(\\d[\\dzaxy]{1,3}|199409))$" };
	if (auto m = ctre::match<regex>(ret))
	{
		ret = "-std=" + ret;
		return ret;
	}
#else
	static std::regex regex{ "^(((c|gnu)\\+\\+|gnu|c|iso9899:)(\\d[\\dzaxy]{1,3}|199409))$" };
	if (std::regex_match(ret, regex))
	{
		ret = "-std=" + ret;
		return ret;
	}
#endif

	Diagnostic::errorAbort(fmt::format("Unrecognized or invalid value for langugage standard (cppStandard|cStandard): {}", langStandard));
	return ret;
}

/*****************************************************************************/
const std::string& CompileToolchainGNU::getLinks()
{
	if (m_links.empty())
	{
		m_links = String::getPrefixed(m_project.links(), "-l");

		if (m_project.objectiveCxx() && !m_config.isAppleClang())
		{
			m_links += " -lobjc";
		}

		const auto staticLinks = String::getPrefixed(m_project.staticLinks(), "-l");
		if (m_project.staticLinks().size() > 0)
		{
			if (!m_config.isAppleClang())
			{
				if (m_project.links().size() > 0)
					m_links = fmt::format("-Wl,--copy-dt-needed-entries -Wl,-Bstatic -Wl,--start-group {} -Wl,--end-group -Wl,-Bdynamic {}", staticLinks, m_links);
				else
					m_links = fmt::format("-Wl,--copy-dt-needed-entries -Wl,-Bstatic -Wl,--start-group {} -Wl,--end-group", staticLinks);
			}
			else
			{
				m_links += " " + staticLinks;
			}
		}
	}

	return m_links;
}

/*****************************************************************************/
std::string CompileToolchainGNU::getCompileFlags(const bool objectiveC)
{
	const bool debugSymbols = m_state.configuration.debugSymbols();

	const auto& optimizations = getOptimizations();
	const auto& langStandard = getLangStandard(objectiveC);
	const auto& warnings = getWarnings();
	std::string compileOptions = getCompileOptions();

	if (objectiveC)
	{
		// TODO: This is more of a c language thing
		String::replaceAll(compileOptions, "-fno-rtti", "");
	}

	std::string cFlags;
	if (debugSymbols)
	{
		cFlags += " -g3";

		// Add -pg here-ish
#if defined(CHALET_MACOS)
		// -pg not supported in apple clang
		// TODO: gcc/clang distinction on mac?
#else
		// cFlags = fmt::format("-g3 {} -pg", cFlags);
#endif
	}

	return fmt::format("{} {} {} {} {}", optimizations, cFlags, langStandard, warnings, compileOptions);
}

/*****************************************************************************/
const std::string& CompileToolchainGNU::getCompileOptions()
{
	if (m_compileOptions.empty())
	{
		if (m_project.objectiveCxx() && !m_config.isAppleClang())
		{

#if defined(CHALET_MACOS)
			m_links += " -fnext-runtime";
#else
			m_links += " -fgnu-runtime";
#endif
		}

		m_compileOptions = String::join(m_project.compileOptions());

		if (!String::contains("-fdiagnostics-color=always", m_compileOptions))
			m_compileOptions += " -fdiagnostics-color=always";

		if (!m_project.rtti() && !String::contains("-fno-rtti", m_compileOptions))
			m_compileOptions += " -fno-rtti";

		// #if defined(CHALET_LINUX)
		if (m_config.isGcc())
		{
			if (!String::contains("-fPIC", m_compileOptions))
				m_compileOptions = "-fPIC " + m_compileOptions;

			if (m_project.posixThreads() && !String::contains("-pthread", m_compileOptions))
				m_compileOptions += " -pthread";
		}
		// #endif

#if defined(CHALET_MACOS)
		// TODO: Test Homebrew LLVM/GCC with this
		const auto macosFrameworkPaths = String::getPrefixed(m_project.macosFrameworkPaths(), "-F");
		const auto macosFrameworks = String::getPrefixedAndSuffixed(m_project.macosFrameworks(), "-framework ", ".framework");

		m_compileOptions = fmt::format("{} {} {}", m_compileOptions, macosFrameworkPaths, macosFrameworks);
#endif
	}

	return m_compileOptions;
}

/*****************************************************************************/
const std::string& CompileToolchainGNU::getLinkerOptions()
{
	if (m_linkerOptions.empty())
	{
		auto& configuration = m_state.configuration;
		const bool debugSymbols = configuration.debugSymbols();
		const auto& stripSymbols = getStripSymbols();

		std::string ret = String::join(m_project.linkerOptions());

#if defined(CHALET_MACOS)
		// TODO: Test Homebrew LLVM/GCC with this
		ret += fmt::format("-isysroot {}", m_state.tools.macosSdk());
#endif

		if (debugSymbols)
		{
			if (m_config.isClang())
			{
				// -pg not supported in apple clang
				// TODO: gcc/clang distinction on mac?
				if (String::contains("-pg", ret))
					String::replaceAll(ret, "-pg", "");
			}
			else
			{
				// if (!String::contains("-pg", ret))
				// 	ret += " -pg";
			}
		}
		else if (!m_config.isAppleClang())
		{
			// No lto on mac & not in debug
			if (configuration.linkTimeOptimization() && !String::contains("-flto", ret))
				ret = fmt::format("-flto {}", ret);
		}

		// #if defined(CHALET_LINUX)
		if (m_project.posixThreads() && !String::contains("-pthread", ret))
		{
			if (m_config.isMingw() && m_project.staticLinking())
				ret += " -Wl,-Bstatic -lstdc++ -lpthread";
			else
				ret += " -pthread";
		}
		// #endif

		// TODO: Check if there's a clang/apple clang version of this
		if (!m_config.isClang())
		{
			const auto& linkerScript = m_project.linkerScript();
			if (!linkerScript.empty())
			{
				ret += fmt::format(" -T {}", linkerScript);
			}
		}

		if (m_project.staticLinking())
		{
			// if (String::contains("-libstdc++", ret))
			// 	String::replaceAll(ret, "-libstdc++", "");
			if (m_config.isClang())
			{
				if (!String::contains("-static-libsan", ret))
					ret += " -static-libsan";

				// TODO: Investigate for other -static candidates on clang/mac
			}
			else
			{
				if (!String::contains("-static-libgcc", ret))
					ret += " -static-libgcc";

				if (!String::contains("-static-libasan", ret))
					ret += " -static-libasan";

				if (!String::contains("-static-libtsan", ret))
					ret += " -static-libtsan";

				if (!String::contains("-static-liblsan", ret))
					ret += " -static-liblsan";

				if (!String::contains("-static-libubsan", ret))
					ret += " -static-libubsan";

				if (!String::contains("-static-libstdc++", ret))
					ret += " -static-libstdc++";
			}
		}

		if (m_config.isMingwGcc())
		{
			const ProjectKind kind = m_project.kind();
			if (kind == ProjectKind::DesktopApplication && !debugSymbols)
			{
				// TODO: check other windows specific options
				if (!String::contains("-mwindows", ret))
					ret += " -mwindows";
			}
		}

		m_linkerOptions = fmt::format("{} {}", stripSymbols, ret);

#if defined(CHALET_MACOS)
		// TODO: Test Homebrew LLVM/GCC with this
		const auto macosFrameworkPaths = String::getPrefixed(m_project.macosFrameworkPaths(), "-F");
		const auto macosFrameworks = String::getPrefixedAndSuffixed(m_project.macosFrameworks(), "-framework ", ".framework");

		m_linkerOptions = fmt::format("{} {} {}", m_linkerOptions, macosFrameworkPaths, macosFrameworks);
#endif
	}

	return m_linkerOptions;
}

/*****************************************************************************/
const std::string& CompileToolchainGNU::getStripSymbols()
{
	if (!m_config.isClang())
	{
		if (m_stripSymbols.empty() && m_state.configuration.stripSymbols())
		{
			m_stripSymbols = "-s";
		}
	}
	return m_stripSymbols;
}

/*****************************************************************************/
const std::string& CompileToolchainGNU::getRunPathFlag()
{
#if defined(CHALET_LINUX)
	if (m_runPathFlag.empty())
	{
		m_runPathFlag = "'-Wl,-R$$ORIGIN'";
	}
#endif
	return m_runPathFlag;
}

/*****************************************************************************/
const std::string& CompileToolchainGNU::getOptimizations()
{
	if (m_optimizations.empty())
	{
		auto& configuration = m_state.configuration;
		m_optimizations = configuration.optimizations();

		if (configuration.debugSymbols())
		{
			// force -O0 (set to anything else here would be in error)
			if (m_optimizations != "-Og" && m_optimizations != "-O0")
				m_optimizations = "-O0";
		}
	}

	return m_optimizations;
}

/*****************************************************************************/
const std::string& CompileToolchainGNU::getPchInclude()
{
	// TODO: Potential for more than one pch?
	if (m_pchInclude.empty() && m_project.usesPch())
	{
		const auto objDirPch = m_state.paths.getPrecompiledHeaderInclude(m_project);
		m_pchInclude = fmt::format("-include {objDirPch}", FMT_ARG(objDirPch));
	}

	return m_pchInclude;
}

/*****************************************************************************/
std::string CompileToolchainGNU::getAsmGenerateCommand(const std::string& inputFile, const std::string& outputFile)
{
	// TODO: Customizations for these commands
#if defined(CHALET_MACOS)
	return fmt::format("otool -tvV {inputFile} | c++filt > {outputFile}",
		FMT_ARG(inputFile),
		FMT_ARG(outputFile));
#else
	return fmt::format("objdump -d -C -Mintel {inputFile} > {outputFile}",
		FMT_ARG(inputFile),
		FMT_ARG(outputFile));
#endif
}

/*****************************************************************************/
std::string CompileToolchainGNU::getPchCompileCommand(const std::string& inputFile, const std::string& outputFile, const std::string& dependency)
{
	const auto& cc = m_config.compilerExecutable();
	const auto cFlags = getCompileFlags();
	const auto& defines = getDefines();
	const auto& includeDirs = getIncludes();

	return fmt::format("{cc} -MT {outputFile} -MMD -MP -MF {dependency} {cFlags} {defines} {includeDirs} -o {outputFile} -c {inputFile}",
		FMT_ARG(cc),
		FMT_ARG(inputFile),
		FMT_ARG(outputFile),
		FMT_ARG(dependency),
		FMT_ARG(cFlags),
		FMT_ARG(defines),
		FMT_ARG(includeDirs));
}

/*****************************************************************************/
std::string CompileToolchainGNU::getRcCompileCommand(const std::string& inputFile, const std::string& outputFile, const std::string& dependency)
{
	const auto& rc = m_state.compilers.rc();
	const auto& defines = getDefines();
	const auto& includeDirs = getIncludes();

	// Note: The dependency generation args have to be passed into the preprocessor
	//   The underlying preprocessor command is "gcc -E -xc-header -DRC_INVOKED"
	//   This runs in C mode, so we don't want any c++ flags passed in
	//   See: https://sourceware.org/binutils/docs/binutils/windres.html
	return fmt::format("{rc} -J rc -O coff --preprocessor-arg=-MT --preprocessor-arg={outputFile} --preprocessor-arg=-MMD --preprocessor-arg=-MP --preprocessor-arg=-MF --preprocessor-arg={dependency} {defines} {includeDirs} -i {inputFile} -o {outputFile}",
		FMT_ARG(rc),
		FMT_ARG(inputFile),
		FMT_ARG(outputFile),
		FMT_ARG(dependency),
		FMT_ARG(defines),
		FMT_ARG(includeDirs));
}

/*****************************************************************************/
std::string CompileToolchainGNU::getCppCompileCommand(const std::string& inputFile, const std::string& outputFile, const std::string& dependency)
{
	const auto& cc = m_config.compilerExecutable();
	const auto cFlags = getCompileFlags();
	const auto& defines = getDefines();
	const auto& includeDirs = getIncludes();

	const auto& pchInclude = getPchInclude();

	return fmt::format("{cc} -MT {outputFile} -MMD -MP -MF {dependency} {cFlags} {defines} {includeDirs} {pchInclude} -o {outputFile} -c {inputFile}",
		FMT_ARG(cc),
		FMT_ARG(inputFile),
		FMT_ARG(outputFile),
		FMT_ARG(dependency),
		FMT_ARG(cFlags),
		FMT_ARG(defines),
		FMT_ARG(pchInclude),
		FMT_ARG(includeDirs));
}

/*****************************************************************************/
std::string CompileToolchainGNU::getObjcppCompileCommand(const std::string& inputFile, const std::string& outputFile, const std::string& dependency, const bool treatAsC)
{
	const auto& cc = m_config.compilerExecutable();
	const auto cFlags = getCompileFlags(treatAsC);
	const auto& defines = getDefines();
	const auto& includeDirs = getIncludes();

	return fmt::format("{cc} -MT {outputFile} -MMD -MP -MF {dependency} {cFlags} {defines} {includeDirs} -o {outputFile} -c {inputFile}",
		FMT_ARG(cc),
		FMT_ARG(inputFile),
		FMT_ARG(outputFile),
		FMT_ARG(dependency),
		FMT_ARG(cFlags),
		FMT_ARG(defines),
		FMT_ARG(includeDirs));
}

/*****************************************************************************/
std::string CompileToolchainGNU::getLinkerTargetCommand(const std::string& outputFile, const std::string& sourceObjs, const std::string& outputFileBase)
{
	const ProjectKind kind = m_project.kind();
	const auto& libDirs = getLibDirs();
	const auto& linkerOptions = getLinkerOptions();
	const auto& links = getLinks();

	const auto& cc = m_config.compilerExecutable();

	switch (kind)
	{
		case ProjectKind::DynamicLibrary: {
			// TODO: any difference in MinGW Clang vs GCC
			if (m_config.isMingw())
			{
				std::string outputDef;
				if (m_project.windowsOutputDef())
					outputDef = " -Wl,--output-def=\"{outputFileBase}.def\"";

				return fmt::format("{cc} -shared {outputDef}-Wl,--out-implib=\"{outputFileBase}.a\" -Wl,--dll {linkerOptions} {libDirs} -o {outputFile} {sourceObjs} {links}",
					FMT_ARG(cc),
					FMT_ARG(outputDef),
					FMT_ARG(outputFileBase),
					FMT_ARG(linkerOptions),
					FMT_ARG(libDirs),
					FMT_ARG(outputFile),
					FMT_ARG(sourceObjs),
					FMT_ARG(links));
			}
			else if (m_config.isClang())
			{
				return fmt::format("{cc} -dynamiclib -fPIC -undefined suppress -flat_namespace {linkerOptions} {libDirs} -o {outputFile} {sourceObjs} {links}",
					FMT_ARG(cc),
					FMT_ARG(linkerOptions),
					FMT_ARG(libDirs),
					FMT_ARG(outputFile),
					FMT_ARG(sourceObjs),
					FMT_ARG(links));
			}
			else
			{
				return fmt::format("{cc} -shared -fPIC {linkerOptions} {libDirs} -o {outputFile} {sourceObjs} {links}",
					FMT_ARG(cc),
					FMT_ARG(linkerOptions),
					FMT_ARG(libDirs),
					FMT_ARG(outputFile),
					FMT_ARG(sourceObjs),
					FMT_ARG(links));
			}
		}

		case ProjectKind::StaticLibrary: {
			const auto& ar = m_state.tools.ar();

			return fmt::format("{ar} -c -r -s {outputFile} {sourceObjs}",
				FMT_ARG(ar),
				FMT_ARG(outputFile),
				FMT_ARG(sourceObjs));
		}

		case ProjectKind::DesktopApplication:
		case ProjectKind::ConsoleApplication:
		default:
			break;
	}

	const auto& rpath = getRunPathFlag();

	return fmt::format("{cc} {libDirs} -o {outputFile} {rpath} {sourceObjs} {links} {linkerOptions}",
		FMT_ARG(cc),
		FMT_ARG(libDirs),
		FMT_ARG(outputFile),
		FMT_ARG(rpath),
		FMT_ARG(sourceObjs),
		FMT_ARG(links),
		FMT_ARG(linkerOptions));
}
}
