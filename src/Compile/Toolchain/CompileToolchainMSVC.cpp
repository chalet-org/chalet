/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Toolchain/CompileToolchainMSVC.hpp"

#include "Libraries/Format.hpp"
#include "Libraries/Regex.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

// https://docs.microsoft.com/en-us/cpp/build/reference/compiler-options-listed-alphabetically?view=msvc-160

namespace chalet
{
/*****************************************************************************/
CompileToolchainMSVC::CompileToolchainMSVC(const BuildState& inState, const ProjectConfiguration& inProject, const CompilerConfig& inConfig) :
	ICompileToolchain(inState),
	m_project(inProject),
	m_config(inConfig),
	m_compilerType(m_config.compilerType())
{
	UNUSED(m_project);
}

/*****************************************************************************/
ToolchainType CompileToolchainMSVC::type() const
{
	return ToolchainType::MSVC;
}

/*****************************************************************************/
bool CompileToolchainMSVC::preBuild()
{
	if (m_project.usesPch())
	{
		const auto& objDir = m_state.paths.objDir();
		const auto& pch = m_project.pch();
		m_pchSource = fmt::format("{}/{}.cpp", objDir, pch);

		m_pchMinusLocation = pch;
		for (std::string loc : m_project.locations())
		{
			if (loc.back() != '/')
				loc.push_back('/');

			String::replaceAll(m_pchMinusLocation, loc, "");
		}

		if (!Commands::pathExists(m_pchSource))
		{
			return Commands::createFileWithContents(m_pchSource, fmt::format("#include \"{}\"", m_pchMinusLocation));
		}
	}

	return true;
}

/*****************************************************************************/
StringList CompileToolchainMSVC::getPchCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency)
{
	UNUSED(generateDependency, dependency);

	chalet_assert(!outputFile.empty(), "");

	StringList ret;

	addExectuable(ret, m_config.compilerExecutable());
	ret.push_back("/nologo");
	ret.push_back("/diagnostics:caret");

	if (generateDependency && m_isNinja)
	{
		ret.push_back("/showIncludes");
	}

	addThreadModelCompileOption(ret);
	addOptimizationOption(ret);

	const auto specialization = m_project.language() == CodeLanguage::CPlusPlus ? CxxSpecialization::Cpp : CxxSpecialization::C;
	addLanguageStandard(ret, specialization);
	addExceptionHandlingModel(ret);
	addWarnings(ret);

	ret.push_back("/utf-8");

	addCompileOptions(ret);
	addNoRunTimeTypeInformationOption(ret);
	// addWholeProgramOptimization(ret);

	addDefines(ret);
	addIncludes(ret);

	std::string pchObject = outputFile;
	String::replaceAll(pchObject, ".pch", ".obj");

	ret.push_back(getPathCommand("/Fp", outputFile));

	ret.push_back(getPathCommand("/Fo", pchObject));
	UNUSED(inputFile);

	ret.push_back("/c");
	ret.push_back(getPathCommand("/Yc", m_pchMinusLocation));
	ret.push_back(m_pchSource);

	return ret;
}

/*****************************************************************************/
StringList CompileToolchainMSVC::getRcCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency)
{
	UNUSED(generateDependency, dependency);

	StringList ret;

	addExectuable(ret, m_state.compilerTools.rc());
	ret.push_back("/nologo");

	addResourceDefines(ret);
	addIncludes(ret);

	ret.push_back(getPathCommand("/Fo", outputFile));

	ret.push_back(inputFile);

	return ret;
}

/*****************************************************************************/
StringList CompileToolchainMSVC::getCxxCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency, const CxxSpecialization specialization)
{
	UNUSED(generateDependency, dependency);

	chalet_assert(!outputFile.empty(), "");

	StringList ret;

	addExectuable(ret, m_config.compilerExecutable());
	ret.push_back("/nologo");
	ret.push_back("/diagnostics:caret");
	ret.push_back("/MP");

	if (generateDependency && m_isNinja)
	{
		ret.push_back("/showIncludes");
	}

	addThreadModelCompileOption(ret);
	addOptimizationOption(ret);
	addLanguageStandard(ret, specialization);
	addExceptionHandlingModel(ret);
	addWarnings(ret);

	ret.push_back("/utf-8");

	addCompileOptions(ret);
	addNoRunTimeTypeInformationOption(ret);
	// addWholeProgramOptimization(ret);

	addDebuggingInformationOption(ret);

	addDefines(ret);
	addIncludes(ret);

	ret.push_back(getPathCommand("/Fo", outputFile));

	ret.push_back("/c");
	addPchInclude(ret);
	ret.push_back(inputFile);

	return ret;
}

/*****************************************************************************/
StringList CompileToolchainMSVC::getLinkerTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase)
{
	UNUSED(outputFileBase);

	ProjectKind kind = m_project.kind();
	if (kind == ProjectKind::SharedLibrary)
	{
		return getSharedLibTargetCommand(outputFile, sourceObjs, outputFileBase);
	}
	else if (kind == ProjectKind::StaticLibrary)
	{
		return getStaticLibTargetCommand(outputFile, sourceObjs, outputFileBase);
	}
	else
	{
		return getExecutableTargetCommand(outputFile, sourceObjs, outputFileBase);
	}
}

/*****************************************************************************/
StringList CompileToolchainMSVC::getSharedLibTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase)
{
	UNUSED(outputFile, sourceObjs);

	chalet_assert(!outputFile.empty() && sourceObjs.size() > 0, "");

	StringList ret;

	addExectuable(ret, m_state.compilerTools.linker());
	ret.push_back("/nologo");
	ret.push_back("/dll");

	addTargetPlatformArch(ret);
	addSubSystem(ret);
	addCgThreads(ret);
	addLibDirs(ret);

	const bool debugSymbols = m_state.configuration.debugSymbols();
	if (m_state.configuration.linkTimeOptimization())
	{
		// combines w/ /GL - I think this is basically part of MS's link-time optimization
		// ret.push_back("/LTCG");

		// Note: These are also tied to /INCREMENTAL (implied with /debug)
		if (debugSymbols)
			ret.push_back("/opt:NOREF,NOICF,NOLBR");
		else
			ret.push_back("/opt:REF,ICF,LBR");

		// OPT:LBR - relates to arm binaries
	}

	if (debugSymbols)
	{
		ret.push_back("/debug");
		ret.push_back("/INCREMENTAL");

		ret.push_back(fmt::format("/pdb:{}.pdb", outputFileBase));
	}
	else
	{
		ret.push_back("/INCREMENTAL:NO");
	}

	// TODO /version
	ret.push_back("/version:0.0");

	ret.push_back(fmt::format("/out:{}", outputFile));

	addPrecompiledHeaderLink(ret);
	addSourceObjects(ret, sourceObjs);
	addLinks(ret);

	return ret;
}

/*****************************************************************************/
StringList CompileToolchainMSVC::getStaticLibTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase)
{
	UNUSED(outputFile, sourceObjs);

	chalet_assert(!outputFile.empty() && sourceObjs.size() > 0, "");

	StringList ret;

	addExectuable(ret, m_state.compilerTools.archiver());
	ret.push_back("/nologo");

	addTargetPlatformArch(ret);

	/*if (m_state.configuration.linkTimeOptimization())
	{
		// combines w/ /GL - I think this is basically part of MS's link-time optimization
		ret.push_back("/LTCG");
	}*/

	if (m_project.warningsTreatedAsErrors())
		ret.push_back("/WX");

	// const auto& objDir = m_state.paths.objDir();
	// ret.push_back(fmt::format("/DEF:{}/{}.def", objDir, outputFileBase));
	UNUSED(outputFileBase);

	// TODO: /SUBSYSTEM

	ret.push_back(fmt::format("/out:{}", outputFile));

	addSourceObjects(ret, sourceObjs);

	return ret;
}

/*****************************************************************************/
StringList CompileToolchainMSVC::getExecutableTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase)
{
	UNUSED(outputFile, sourceObjs);

	chalet_assert(!outputFile.empty() && sourceObjs.size() > 0, "");

	StringList ret;

	addExectuable(ret, m_state.compilerTools.linker());
	ret.push_back("/nologo");

	addTargetPlatformArch(ret);
	addSubSystem(ret);
	addCgThreads(ret);
	addLibDirs(ret);

	const bool debugSymbols = m_state.configuration.debugSymbols();
	if (m_state.configuration.linkTimeOptimization())
	{
		// combines w/ /GL - I think this is basically part of MS's link-time optimization
		// ret.push_back("/LTCG");

		// Note: These are also tied to /INCREMENTAL (implied with /debug)
		if (debugSymbols)
			ret.push_back("/opt:NOREF,NOICF,NOLBR");
		else
			ret.push_back("/opt:REF,ICF,LBR");

		// OPT:LBR - relates to arm binaries
	}

	if (debugSymbols)
	{
		ret.push_back("/debug");
		ret.push_back("/INCREMENTAL");

		ret.push_back(fmt::format("/pdb:{}.pdb", outputFileBase));
	}
	else
	{
		ret.push_back("/INCREMENTAL:NO");
	}

	// TODO /version
	ret.push_back("/version:0.0");

	ret.push_back(fmt::format("/out:{}", outputFile));

	addPrecompiledHeaderLink(ret);
	addSourceObjects(ret, sourceObjs);
	addLinks(ret);

	return ret;
}

/*****************************************************************************/
void CompileToolchainMSVC::addIncludes(StringList& outArgList) const
{
	// outArgList.push_back("/X"); // ignore "Path"

	const std::string option{ "/I" };

	for (const auto& dir : m_project.includeDirs())
	{
		std::string outDir = dir;
		if (String::endsWith('/', outDir))
			outDir.pop_back();

		outArgList.push_back(getPathCommand(option, outDir));
	}

	for (const auto& dir : m_project.locations())
	{
		std::string outDir = dir;
		if (String::endsWith('/', outDir))
			outDir.pop_back();

		outArgList.push_back(getPathCommand(option, outDir));
	}

	for (const auto& dir : m_state.msvcEnvironment.include())
	{
		outArgList.push_back(getPathCommand(option, dir));
	}
}

/*****************************************************************************/
StringList CompileToolchainMSVC::getLinkExclusions() const
{
	return { "stdc++fs" };
}

/*****************************************************************************/
void CompileToolchainMSVC::addWarnings(StringList& outArgList) const
{
	// TODO: ProjectWarnings::Custom would need to convert GNU warnings to MSVC warning codes

	switch (m_project.warningsPreset())
	{
		case ProjectWarnings::Minimal:
			outArgList.push_back("/W1");
			break;

		case ProjectWarnings::Extra:
			outArgList.push_back("/W2");
			break;

		case ProjectWarnings::Error: {
			outArgList.push_back("/W2");
			outArgList.push_back("/WX");
			break;
		}
		case ProjectWarnings::Pedantic: {
			outArgList.push_back("/W3");
			outArgList.push_back("/WX");
			break;
		}
		case ProjectWarnings::Strict:
		case ProjectWarnings::StrictPedantic: {
			outArgList.push_back("/W4");
			outArgList.push_back("/WX");
			break;
		}
		case ProjectWarnings::VeryStrict: {
			outArgList.push_back("/Wall");
			outArgList.push_back("/WX");
			break;
		}

		case ProjectWarnings::Custom:
		case ProjectWarnings::None:
		default:
			outArgList.push_back("/W3");
			break;
	}
}

/*****************************************************************************/
void CompileToolchainMSVC::addDefines(StringList& outArgList) const
{
	const std::string prefix = "/D";
	for (auto& define : m_project.defines())
	{
		outArgList.push_back(prefix + define);
	}
}

/*****************************************************************************/
void CompileToolchainMSVC::addResourceDefines(StringList& outArgList) const
{
	const std::string prefix = "/d";
	for (auto& define : m_project.defines())
	{
		outArgList.push_back(prefix + define);
	}
}

/*****************************************************************************/
void CompileToolchainMSVC::addExceptionHandlingModel(StringList& outArgList) const
{
	// /EH - Exception handling model
	// s - standard C++ stack unwinding
	// c - functions declared as extern "C" never throw

	outArgList.push_back("/EHsc");
}

/*****************************************************************************/
void CompileToolchainMSVC::addPchInclude(StringList& outArgList) const
{
	// TODO: Potential for more than one pch?
	if (m_project.usesPch())
	{
		const auto objDirPch = m_state.paths.getPrecompiledHeaderTarget(m_project);

		outArgList.push_back(getPathCommand("/Yu", m_pchMinusLocation));

		// /Fp specifies the location of the PCH object file
		outArgList.push_back(getPathCommand("/Fp", objDirPch));

		// /FI force-includes the PCH source file so one doesn't need to use the #include directive in every file
		outArgList.push_back(getPathCommand("/FI", m_pchMinusLocation));
	}
}

/*****************************************************************************/
void CompileToolchainMSVC::addOptimizationOption(StringList& outArgList) const
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
		opt = "/Od";
	}
	else
	{
		switch (level)
		{
			case OptimizationLevel::L1:
				opt = "/O1";
				break;

			case OptimizationLevel::L2:
				opt = "/O2";
				break;

			case OptimizationLevel::L3:
				opt = "/Ox";
				break;

			case OptimizationLevel::Size:
				opt = "/Os";
				break;

			case OptimizationLevel::Fast:
				opt = "/Ot";
				break;

			case OptimizationLevel::Debug:
			case OptimizationLevel::None:
				opt = "/Od";
				break;

			case OptimizationLevel::CompilerDefault:
			default:
				break;
		}
	}

	// Note: in MSVC, one can combine these (annoyingly)
	//	Might be desireable to add:
	//    /Oy (suppresses the creation of frame pointers on the call stack for quicker function calls.)
	//    /Oi (generates intrinsic functions for appropriate function calls.)

	if (opt.empty())
		return;

	outArgList.push_back(std::move(opt));

	if (configuration.debugSymbols())
	{
		auto buildDir = m_state.paths.buildOutputDir() + '/';
		outArgList.push_back("/Zi"); // separate pdb
		outArgList.push_back("/FS"); // Force Synchronous PDB Writes
		outArgList.push_back(getPathCommand("/Fd", buildDir));
		outArgList.push_back(std::move(opt));
		outArgList.push_back("/Ob0");  // disable inline expansion
		outArgList.push_back("/RTC1"); // Enables stack frame run-time error checking, uninitialized variables
	}
	else
	{
		outArgList.push_back(std::move(opt));
	}
}

/*****************************************************************************/
void CompileToolchainMSVC::addLanguageStandard(StringList& outArgList, const CxxSpecialization specialization) const
{
	if (specialization == CxxSpecialization::C)
	{
		std::string langStandard = String::toLowerCase(m_project.cStandard());
		if (String::equals("gnu2x", langStandard)
			|| String::equals("gnu18", langStandard)
			|| String::equals("gnu17", langStandard)
			|| String::equals("c2x", langStandard)
			|| String::equals("c18", langStandard)
			|| String::equals("c17", langStandard)
			|| String::equals(langStandard, "iso9899:2018")
			|| String::equals(langStandard, "iso9899:2017"))
		{
			outArgList.push_back("/std:c17");
		}
		else
		{
			outArgList.push_back("/std:c11");
		}
	}
	else if (specialization == CxxSpecialization::Cpp)
	{
		std::string langStandard = String::toLowerCase(m_project.cppStandard());
		if (String::equals(langStandard, "c++20")
			|| String::equals(langStandard, "c++2a")
			|| String::equals(langStandard, "gnu++20")
			|| String::equals(langStandard, "gnu++2a"))
		{
			outArgList.push_back("/std:c++latest");
		}
		else if (String::equals(langStandard, "c++17")
			|| String::equals(langStandard, "c++1z")
			|| String::equals(langStandard, "gnu++17")
			|| String::equals(langStandard, "gnu++1z"))
		{
			outArgList.push_back("/std:c++17");
		}
		else
		{
			outArgList.push_back("/std:c++14");
		}
	}
}

/*****************************************************************************/
void CompileToolchainMSVC::addDebuggingInformationOption(StringList& outArgList) const
{
	// TODO! - pdb files etc
	// /Zi /ZI /debug
	UNUSED(outArgList);
}

/*****************************************************************************/
void CompileToolchainMSVC::addCompileOptions(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void CompileToolchainMSVC::addNoRunTimeTypeInformationOption(StringList& outArgList) const
{
	if (!m_project.rtti())
	{
		List::addIfDoesNotExist(outArgList, "/GR-");
	}
}

/*****************************************************************************/
void CompileToolchainMSVC::addThreadModelCompileOption(StringList& outArgList) const
{
	// /MD - multithreaded dll
	// /MDd - debug multithreaded dll
	// /MT - multithreaded executable
	// /MTd - debug multithreaded executable

	// TODO: at the moment, assumes threaded

	if (m_project.isSharedLibrary())
	{
		if (m_state.configuration.debugSymbols())
			outArgList.push_back("/MDd");
		else
			outArgList.push_back("/MD");
	}
	else
	{
		if (m_state.configuration.debugSymbols())
			outArgList.push_back("/MTd");
		else
			outArgList.push_back("/MT");
	}
}

/*****************************************************************************/
void CompileToolchainMSVC::addWholeProgramOptimization(StringList& outArgList) const
{
	if (m_state.configuration.linkTimeOptimization())
	{
		outArgList.push_back("/GL");
	}
}

/*****************************************************************************/
void CompileToolchainMSVC::addLibDirs(StringList& outArgList) const
{
	std::string option{ "/LIBPATH:" };
	for (const auto& dir : m_project.libDirs())
	{
		outArgList.push_back(getPathCommand(option, dir));
	}

	outArgList.push_back(getPathCommand(option, m_state.paths.buildOutputDir()));

	for (const auto& dir : m_state.msvcEnvironment.lib())
	{
		outArgList.push_back(getPathCommand(option, dir));
	}
}

/*****************************************************************************/
void CompileToolchainMSVC::addLinks(StringList& outArgList) const
{
	const bool hasStaticLinks = m_project.staticLinks().size() > 0;
	const bool hasDynamicLinks = m_project.links().size() > 0;

	UNUSED(hasDynamicLinks);

	if (hasStaticLinks)
	{
		for (auto& project : m_state.projects)
		{
			auto& link = project->name();
			if (List::contains(m_project.projectStaticLinks(), link))
			{
				outArgList.push_back(project->outputFile());
				break;
			}
		}
	}

	if (hasDynamicLinks)
	{
		auto excludes = getLinkExclusions();

		for (auto& link : m_project.links())
		{
			if (List::contains(excludes, link))
				continue;

			for (auto& project : m_state.projects)
			{
				if (project->name() == link)
				{
					auto outputFile = project->outputFile();
					if (String::endsWith(".dll", outputFile))
					{
						String::replaceAll(outputFile, ".dll", ".lib");
						outArgList.push_back(std::move(outputFile));
						break;
					}
				}
			}
		}
	}

	// TODO: Dynamic way of determining this list
	// would they differ between console app & windows app?
	for (const char* link : {
			 "DbgHelp",
			 "kernel32",
			 "user32",
			 "gdi32",
			 "winspool",
			 "shell32",
			 "ole32",
			 "oleaut32",
			 "uuid",
			 "comdlg32",
			 "advapi32",
		 })
	{
		List::addIfDoesNotExist(outArgList, fmt::format("{}.lib", link));
	}
}

/*****************************************************************************/
void CompileToolchainMSVC::addCgThreads(StringList& outArgList) const
{
	uint maxJobs = m_state.environment.maxJobs();
	if (maxJobs > 4)
	{
		maxJobs = std::min<uint>(maxJobs, 8);
		outArgList.push_back(fmt::format("/cgthreads:{}", maxJobs));
	}
}

/*****************************************************************************/
void CompileToolchainMSVC::addSubSystem(StringList& outArgList) const
{
	std::string subsystem;
	switch (m_project.kind())
	{
		case ProjectKind::ConsoleApplication:
			subsystem = "console";
			break;

		case ProjectKind::DesktopApplication:
			subsystem = "windows";
			break;

		default: break;
	}

	// TODO:
	// https://docs.microsoft.com/en-us/cpp/build/reference/subsystem-specify-subsystem?view=msvc-160

	if (subsystem.empty())
		return;

	outArgList.push_back(fmt::format("/subsystem:{}", subsystem));
}

/*****************************************************************************/
void CompileToolchainMSVC::addTargetPlatformArch(StringList& outArgList) const
{
	// TODO: /MACHINE - target platform arch
	outArgList.push_back("/machine:x64");
}

/*****************************************************************************/
std::string CompileToolchainMSVC::getPathCommand(std::string_view inCmd, const std::string& inPath) const
{
	if (m_quotePaths)
		return fmt::format("{}\"{}\"", inCmd, inPath);
	else
		return fmt::format("{}{}", inCmd, inPath);
}

/*****************************************************************************/
void CompileToolchainMSVC::addSourceObjects(StringList& outArgList, const StringList& sourceObjs) const
{
	for (auto& source : sourceObjs)
	{
		outArgList.push_back(source);
	}
}

/*****************************************************************************/
void CompileToolchainMSVC::addPrecompiledHeaderLink(StringList outArgList) const
{
	if (m_project.usesPch())
	{
		const auto& objDir = m_state.paths.objDir();
		const auto& pch = m_project.pch();
		std::string pchObject = fmt::format("{}/{}.obj", objDir, pch);
		std::string pchInclude = fmt::format("{}/{}.pch", objDir, pch);

		outArgList.push_back(std::move(pchObject));
		outArgList.push_back(std::move(pchInclude));
	}
}

}
