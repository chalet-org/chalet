/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Toolchain/CompileToolchainVisualStudio.hpp"

#include "Terminal/Commands.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

#include "Compile/CompilerConfig.hpp"
#include "State/BuildState.hpp"
#include "State/Target/SourceTarget.hpp"

// https://docs.microsoft.com/en-us/cpp/build/reference/compiler-options-listed-alphabetically?view=msvc-160

namespace chalet
{
/*****************************************************************************/
CompileToolchainVisualStudio::CompileToolchainVisualStudio(const BuildState& inState, const SourceTarget& inProject, const CompilerConfig& inConfig) :
	ICompileToolchain(inState, inProject, inConfig),
	m_compilerType(m_config.compilerType())
{
	UNUSED(m_project);
}

/*****************************************************************************/
ToolchainType CompileToolchainVisualStudio::type() const noexcept
{
	return ToolchainType::VisualStudio;
}

/*****************************************************************************/
bool CompileToolchainVisualStudio::initialize()
{
	if (m_project.usesPch())
	{
		const auto& objDir = m_state.paths.objDir();
		const auto& pch = m_project.pch();
		m_pchSource = fmt::format("{}/{}.cpp", objDir, pch);

		m_pchMinusLocation = String::getPathFilename(pch);

		if (!Commands::pathExists(m_pchSource))
		{
			if (!Commands::createFileWithContents(m_pchSource, fmt::format("#include \"{}\"", m_pchMinusLocation)))
				return false;
		}
	}

	if (!createWindowsApplicationManifest())
		return false;

	if (!createWindowsApplicationIcon())
		return false;

	return true;
}

/*****************************************************************************/
StringList CompileToolchainVisualStudio::getPchCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency, const std::string& arch)
{
	UNUSED(generateDependency, dependency, arch);

	chalet_assert(!outputFile.empty(), "");

	StringList ret;

	if (m_config.compilerExecutable().empty())
		return ret;

	addExectuable(ret, m_config.compilerExecutable());
	ret.emplace_back("/nologo");
	addDiagnosticsOption(ret);

	if (generateDependency && m_isNinja)
	{
		ret.emplace_back("/showIncludes");
	}

	addThreadModelCompileOption(ret);
	addOptimizationOption(ret);

	const auto specialization = m_project.language() == CodeLanguage::CPlusPlus ? CxxSpecialization::CPlusPlus : CxxSpecialization::C;
	addLanguageStandard(ret, specialization);
	addNoExceptionsOption(ret);
	addWarnings(ret);

	ret.emplace_back("/utf-8");

	addCompileOptions(ret);
	addNoRunTimeTypeInformationOption(ret);
	// addWholeProgramOptimization(ret);

	addDefines(ret);
	addIncludes(ret);

	std::string pchObject = outputFile;
	String::replaceAll(pchObject, ".pch", ".obj");

	ret.emplace_back(getPathCommand("/Fp", outputFile));

	ret.emplace_back(getPathCommand("/Fo", pchObject));
	UNUSED(inputFile);

	ret.emplace_back("/c");
	ret.emplace_back(getPathCommand("/Yc", m_pchMinusLocation));
	ret.push_back(m_pchSource);

	return ret;
}

/*****************************************************************************/
StringList CompileToolchainVisualStudio::getRcCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency)
{
	UNUSED(generateDependency, dependency);

	StringList ret;

	if (m_state.toolchain.compilerWindowsResource().empty())
		return ret;

	addExectuable(ret, m_state.toolchain.compilerWindowsResource());
	ret.emplace_back("/nologo");

	addResourceDefines(ret);
	addIncludes(ret);

	ret.emplace_back(getPathCommand("/Fo", outputFile));

	ret.push_back(inputFile);

	return ret;
}

/*****************************************************************************/
StringList CompileToolchainVisualStudio::getCxxCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency, const CxxSpecialization specialization)
{
	UNUSED(generateDependency, dependency);

	chalet_assert(!outputFile.empty(), "");

	StringList ret;

	if (m_config.compilerExecutable().empty())
		return ret;

	addExectuable(ret, m_config.compilerExecutable());
	ret.emplace_back("/nologo");
	addDiagnosticsOption(ret);
	ret.emplace_back("/MP");

	if (generateDependency && m_isNinja)
	{
		ret.emplace_back("/showIncludes");
	}

	addThreadModelCompileOption(ret);
	addOptimizationOption(ret);
	addLanguageStandard(ret, specialization);
	addNoExceptionsOption(ret);
	addWarnings(ret);

	ret.emplace_back("/utf-8");

	addCompileOptions(ret);
	addNoRunTimeTypeInformationOption(ret);
	// addWholeProgramOptimization(ret);

	addDebuggingInformationOption(ret);

	addDefines(ret);
	addIncludes(ret);

	ret.emplace_back(getPathCommand("/Fo", outputFile));

	ret.emplace_back("/c");
	addPchInclude(ret);
	ret.push_back(inputFile);

	return ret;
}

/*****************************************************************************/
StringList CompileToolchainVisualStudio::getLinkerTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase)
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
StringList CompileToolchainVisualStudio::getSharedLibTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase)
{
	UNUSED(outputFile, sourceObjs);

	chalet_assert(!outputFile.empty() && sourceObjs.size() > 0, "");

	StringList ret;

	if (m_state.toolchain.linker().empty())
		return ret;

	addExectuable(ret, m_state.toolchain.linker());
	ret.emplace_back("/nologo");
	ret.emplace_back("/dll");

	addTargetPlatformArch(ret);
	addSubSystem(ret);
	addEntryPoint(ret);
	addCgThreads(ret);
	addLinkerOptions(ret);
	addLibDirs(ret);

	const bool debugSymbols = m_state.configuration.debugSymbols();
	if (m_state.configuration.linkTimeOptimization())
	{
		// combines w/ /GL - I think this is basically part of MS's link-time optimization
		// ret.emplace_back("/LTCG");

		// Note: These are also tied to /INCREMENTAL (implied with /debug)
		if (debugSymbols)
			ret.emplace_back("/opt:NOREF,NOICF,NOLBR");
		else
			ret.emplace_back("/opt:REF,ICF,LBR");

		// OPT:LBR - relates to arm binaries
	}

	if (debugSymbols)
	{
		ret.emplace_back("/debug");
		ret.emplace_back("/INCREMENTAL");

		ret.emplace_back(fmt::format("/pdb:{}.pdb", outputFileBase));
	}
	else
	{
		ret.emplace_back("/release");
		ret.emplace_back("/INCREMENTAL:NO");
	}

	// TODO /version
	ret.emplace_back("/version:0.0");

	ret.emplace_back(fmt::format("/out:{}", outputFile));

	addPrecompiledHeaderLink(ret);
	addSourceObjects(ret, sourceObjs);
	addLinks(ret);

	return ret;
}

/*****************************************************************************/
StringList CompileToolchainVisualStudio::getStaticLibTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase)
{
	UNUSED(outputFile, sourceObjs);

	chalet_assert(!outputFile.empty() && sourceObjs.size() > 0, "");

	StringList ret;

	if (m_state.toolchain.archiver().empty())
		return ret;

	addExectuable(ret, m_state.toolchain.archiver());
	ret.emplace_back("/nologo");

	addTargetPlatformArch(ret);

	/*if (m_state.configuration.linkTimeOptimization())
	{
		// combines w/ /GL - I think this is basically part of MS's link-time optimization
		ret.emplace_back("/LTCG");
	}*/

	if (m_project.warningsTreatedAsErrors())
		ret.emplace_back("/WX");

	// const auto& objDir = m_state.paths.objDir();
	// ret.emplace_back(fmt::format("/DEF:{}/{}.def", objDir, outputFileBase));
	UNUSED(outputFileBase);

	// TODO: /SUBSYSTEM

	ret.emplace_back(fmt::format("/out:{}", outputFile));

	addSourceObjects(ret, sourceObjs);

	return ret;
}

/*****************************************************************************/
StringList CompileToolchainVisualStudio::getExecutableTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase)
{
	UNUSED(outputFile, sourceObjs);

	chalet_assert(!outputFile.empty() && sourceObjs.size() > 0, "");

	StringList ret;

	if (m_state.toolchain.linker().empty())
		return ret;

	addExectuable(ret, m_state.toolchain.linker());
	ret.emplace_back("/nologo");

	addTargetPlatformArch(ret);
	addSubSystem(ret);
	addEntryPoint(ret);
	addCgThreads(ret);
	addLinkerOptions(ret);
	addLibDirs(ret);

	const bool debugSymbols = m_state.configuration.debugSymbols();
	if (m_state.configuration.linkTimeOptimization())
	{
		// combines w/ /GL - I think this is basically part of MS's link-time optimization
		// ret.emplace_back("/LTCG");

		// Note: These are also tied to /INCREMENTAL (implied with /debug)
		if (debugSymbols)
			ret.emplace_back("/opt:NOREF,NOICF,NOLBR");
		else
			ret.emplace_back("/opt:REF,ICF,LBR");

		// OPT:LBR - relates to arm binaries
	}

	if (debugSymbols)
	{
		ret.emplace_back("/debug");
		ret.emplace_back("/INCREMENTAL");

		ret.emplace_back(fmt::format("/pdb:{}.pdb", outputFileBase));
	}
	else
	{
		ret.emplace_back("/release");
		ret.emplace_back("/INCREMENTAL:NO");
	}

	// TODO /version
	ret.emplace_back("/version:0.0");

	ret.emplace_back(fmt::format("/out:{}", outputFile));

	addPrecompiledHeaderLink(ret);
	addSourceObjects(ret, sourceObjs);
	addLinks(ret);

	return ret;
}

/*****************************************************************************/
void CompileToolchainVisualStudio::addIncludes(StringList& outArgList) const
{
	// outArgList.emplace_back("/X"); // ignore "Path"

	const std::string option{ "/I" };

	for (const auto& dir : m_project.includeDirs())
	{
		std::string outDir = dir;
		if (String::endsWith('/', outDir))
			outDir.pop_back();

		outArgList.emplace_back(getPathCommand(option, outDir));
	}

	for (const auto& dir : m_project.locations())
	{
		std::string outDir = dir;
		if (String::endsWith('/', outDir))
			outDir.pop_back();

		outArgList.emplace_back(getPathCommand(option, outDir));
	}

	if (m_project.usesPch())
	{
		auto outDir = String::getPathFolder(m_project.pch());
		List::addIfDoesNotExist(outArgList, getPathCommand(option, outDir));
	}
}

/*****************************************************************************/
StringList CompileToolchainVisualStudio::getLinkExclusions() const
{
	return { "stdc++fs" };
}

/*****************************************************************************/
void CompileToolchainVisualStudio::addWarnings(StringList& outArgList) const
{
	// TODO: ProjectWarnings::Custom would need to convert GNU warnings to MSVC warning codes

	switch (m_project.warningsPreset())
	{
		case ProjectWarnings::Minimal:
			outArgList.emplace_back("/W1");
			break;

		case ProjectWarnings::Extra:
			outArgList.emplace_back("/W2");
			break;

		case ProjectWarnings::Pedantic: {
			outArgList.emplace_back("/W3");
			break;
		}
		case ProjectWarnings::Error: {
			outArgList.emplace_back("/W3");
			outArgList.emplace_back("/WX");
			break;
		}
		case ProjectWarnings::Strict:
		case ProjectWarnings::StrictPedantic: {
			outArgList.emplace_back("/W4");
			outArgList.emplace_back("/WX");
			break;
		}
		case ProjectWarnings::VeryStrict: {
			// outArgList.emplace_back("/Wall"); // Note: Lots of messy compiler level warnings that break your build!
			outArgList.emplace_back("/W4");
			outArgList.emplace_back("/WX");
			break;
		}

		case ProjectWarnings::Custom: {
			// TODO: Refactor this so the strict warnings are stored somewhere GNU can use
			auto& warnings = m_project.warnings();

			StringList veryStrict{
				"noexcept",
				"undef",
				"conversion",
				"cast-qual",
				"float-equal",
				"inline",
				"old-style-cast",
				"strict-null-sentinel",
				"overloaded-virtual",
				"sign-conversion",
				"sign-promo",
			};

			bool strictSet = false;
			for (auto& w : warnings)
			{
				if (!String::equals(veryStrict, w))
					continue;

				outArgList.emplace_back("/Wall");
				strictSet = true;
				break;
			}

			if (!strictSet)
			{
				StringList strictPedantic{
					"unused",
					"cast-align",
					"double-promotion",
					"format=2",
					"missing-declarations",
					"missing-include-dirs",
					"non-virtual-dtor",
					"redundant-decls",
					"unreachable-code",
					"shadow",
				};
				for (auto& w : warnings)
				{
					if (!String::equals(strictPedantic, w))
						continue;

					outArgList.emplace_back("/W4");
					strictSet = true;
					break;
				}
			}

			if (!strictSet)
			{
				if (List::contains<std::string>(warnings, "pedantic"))
				{
					outArgList.emplace_back("/W3");
				}
				else if (List::contains<std::string>(warnings, "extra"))
				{
					outArgList.emplace_back("/W2");
				}
				else if (List::contains<std::string>(warnings, "all"))
				{
					outArgList.emplace_back("/W1");
				}
			}

			if (List::contains<std::string>(warnings, "pedantic"))
			{
				outArgList.emplace_back("/WX");
			}

			break;
		}

		case ProjectWarnings::None:
		default:
			break;
	}
}

/*****************************************************************************/
void CompileToolchainVisualStudio::addDefines(StringList& outArgList) const
{
	const std::string prefix = "/D";
	for (auto& define : m_project.defines())
	{
		outArgList.emplace_back(prefix + define);
	}
}

/*****************************************************************************/
void CompileToolchainVisualStudio::addResourceDefines(StringList& outArgList) const
{
	const std::string prefix = "/d";
	for (auto& define : m_project.defines())
	{
		outArgList.emplace_back(prefix + define);
	}
}

/*****************************************************************************/
void CompileToolchainVisualStudio::addPchInclude(StringList& outArgList) const
{
	// TODO: Potential for more than one pch?
	if (m_project.usesPch())
	{
		const auto& compilerConfig = m_state.compilers.get(m_project.language());
		const auto objDirPch = m_state.paths.getPrecompiledHeaderTarget(m_project, compilerConfig);

		outArgList.emplace_back(getPathCommand("/Yu", m_pchMinusLocation));

		// /Fp specifies the location of the PCH object file
		outArgList.emplace_back(getPathCommand("/Fp", objDirPch));

		// /FI force-includes the PCH source file so one doesn't need to use the #include directive in every file
		outArgList.emplace_back(getPathCommand("/FI", m_pchMinusLocation));
	}
}

/*****************************************************************************/
void CompileToolchainVisualStudio::addOptimizationOption(StringList& outArgList) const
{
	std::string opt;
	auto& configuration = m_state.configuration;

	OptimizationLevel level = configuration.optimizationLevel();

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

	outArgList.emplace_back(std::move(opt));

	if (configuration.debugSymbols())
	{
		auto buildDir = m_state.paths.buildOutputDir() + '/';
		outArgList.emplace_back("/Zi"); // separate pdb
		outArgList.emplace_back("/FS"); // Force Synchronous PDB Writes
		outArgList.emplace_back(getPathCommand("/Fd", buildDir));
		outArgList.emplace_back(std::move(opt));
		outArgList.emplace_back("/Ob0");  // disable inline expansion
		outArgList.emplace_back("/RTC1"); // Enables stack frame run-time error checking, uninitialized variables
	}
	else
	{
		outArgList.emplace_back(std::move(opt));
	}
}

/*****************************************************************************/
void CompileToolchainVisualStudio::addLanguageStandard(StringList& outArgList, const CxxSpecialization specialization) const
{
	// TODO: Reverse years so c11 / c++14 is checked explicitly & newest year isn't
	if (specialization == CxxSpecialization::C)
	{
		outArgList.emplace_back("/TC"); // Treat code as C

		std::string langStandard = String::toLowerCase(m_project.cStandard());
		if (String::equals({ "gnu2x", "gnu18", "gnu17", "c2x", "c18", "c17", "iso9899:2018", "iso9899:2017" }, langStandard))
		{
			outArgList.emplace_back("/std:c17");
		}
		else
		{
			outArgList.emplace_back("/std:c11");
		}
	}
	else if (specialization == CxxSpecialization::CPlusPlus)
	{
		outArgList.emplace_back("/TP"); // Treat code as C++

		std::string langStandard = String::toLowerCase(m_project.cppStandard());
		if (String::equals({ "c++23", "c++2b", "gnu++23", "gnu++2b" }, langStandard))
		{
			outArgList.emplace_back("/std:c++latest");
		}
		else if (String::equals({ "c++20", "c++2a", "gnu++20", "gnu++2a" }, langStandard))
		{
			// TODO: c++20 in 2019 16.11 & 17.0 (Preview 3+ presumably)
			//   https://devblogs.microsoft.com/cppblog/msvc-cpp20-and-the-std-cpp20-switch/
			outArgList.emplace_back("/std:c++20");
		}
		else if (String::equals({ "c++17", "c++1z", "gnu++17", "gnu++1z" }, langStandard))
		{
			outArgList.emplace_back("/std:c++17");
		}
		else
		{
			outArgList.emplace_back("/std:c++14");
		}
	}
}

/*****************************************************************************/
void CompileToolchainVisualStudio::addDebuggingInformationOption(StringList& outArgList) const
{
	// TODO! - pdb files etc
	// /Zi /ZI /debug
	UNUSED(outArgList);
}

/*****************************************************************************/
void CompileToolchainVisualStudio::addDiagnosticsOption(StringList& outArgList) const
{
	outArgList.emplace_back("/diagnostics:caret");
}

/*****************************************************************************/
void CompileToolchainVisualStudio::addCompileOptions(StringList& outArgList) const
{
	for (auto& option : m_project.compileOptions())
	{
		List::addIfDoesNotExist(outArgList, option);
	}
}

/*****************************************************************************/
void CompileToolchainVisualStudio::addNoRunTimeTypeInformationOption(StringList& outArgList) const
{
	if (!m_project.rtti())
	{
		List::addIfDoesNotExist(outArgList, "/GR-");
	}
}

/*****************************************************************************/
void CompileToolchainVisualStudio::addNoExceptionsOption(StringList& outArgList) const
{
	// /EH - Exception handling model
	// s - standard C++ stack unwinding
	// c - functions declared as extern "C" never throw

	if (!m_project.exceptions())
	{
		List::addIfDoesNotExist(outArgList, "/GR-"); // must also disable rtti
		List::addIfDoesNotExist(outArgList, "/D_HAS_EXCEPTIONS=0");
	}
	else
	{
		List::addIfDoesNotExist(outArgList, "/EHsc");
	}
}

/*****************************************************************************/
void CompileToolchainVisualStudio::addThreadModelCompileOption(StringList& outArgList) const
{
	// /MD - multithreaded dll
	// /MDd - debug multithreaded dll
	// /MT - multithreaded executable
	// /MTd - debug multithreaded executable

	// TODO: at the moment, assumes threaded

	if (m_project.isSharedLibrary())
	{
		if (m_state.configuration.debugSymbols())
			outArgList.emplace_back("/MDd");
		else
			outArgList.emplace_back("/MD");
	}
	else
	{
		if (m_state.configuration.debugSymbols())
			outArgList.emplace_back("/MTd");
		else
			outArgList.emplace_back("/MT");
	}
}

/*****************************************************************************/
void CompileToolchainVisualStudio::addWholeProgramOptimization(StringList& outArgList) const
{
	if (m_state.configuration.linkTimeOptimization())
	{
		outArgList.emplace_back("/GL");
	}
}

/*****************************************************************************/
void CompileToolchainVisualStudio::addLibDirs(StringList& outArgList) const
{
	std::string option{ "/LIBPATH:" };
	for (const auto& dir : m_project.libDirs())
	{
		outArgList.emplace_back(getPathCommand(option, dir));
	}

	outArgList.emplace_back(getPathCommand(option, m_state.paths.buildOutputDir()));
}

/*****************************************************************************/
void CompileToolchainVisualStudio::addLinks(StringList& outArgList) const
{
	const bool hasStaticLinks = m_project.staticLinks().size() > 0;
	const bool hasDynamicLinks = m_project.links().size() > 0;

	if (hasStaticLinks)
	{
		for (auto& target : m_state.targets)
		{
			if (target->isProject())
			{
				auto& project = static_cast<const SourceTarget&>(*target);
				if (List::contains(m_project.projectStaticLinks(), project.name()))
				{
					outArgList.push_back(project.outputFile());
				}
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

			bool found = false;
			for (auto& target : m_state.targets)
			{
				if (target->isProject())
				{
					auto& project = static_cast<const SourceTarget&>(*target);
					if (project.name() == link && project.isSharedLibrary())
					{
						auto outputFile = project.outputFile();
						if (String::endsWith(".dll", outputFile))
						{
							String::replaceAll(outputFile, ".dll", ".lib");
							outArgList.emplace_back(std::move(outputFile));
							found = true;
							break;
						}
					}
				}
			}

			if (!found)
			{
				outArgList.emplace_back(fmt::format("{}.lib", link));
			}
		}
	}

	// TODO: Dynamic way of determining this list
	//   would they differ between console app & windows app?
	//   or target architecture?
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
void CompileToolchainVisualStudio::addLinkerOptions(StringList& outArgList) const
{
	for (auto& option : m_project.linkerOptions())
	{
		// if (isFlagSupported(option))
		outArgList.push_back(option);
	}
}

/*****************************************************************************/
void CompileToolchainVisualStudio::addCgThreads(StringList& outArgList) const
{
	uint maxJobs = m_state.info.maxJobs();
	if (maxJobs > 4)
	{
		maxJobs = std::min<uint>(maxJobs, 8);
		outArgList.emplace_back(fmt::format("/cgthreads:{}", maxJobs));
	}
}

/*****************************************************************************/
void CompileToolchainVisualStudio::addSubSystem(StringList& outArgList) const
{
	const ProjectKind kind = m_project.kind();

	// TODO: Support for /driver:WDM (NativeWDM or something)
	// https://docs.microsoft.com/en-us/cpp/build/reference/subsystem-specify-subsystem?view=msvc-160

	if (kind == ProjectKind::Executable)
	{
		const auto subSystem = getMsvcCompatibleSubSystem();
		List::addIfDoesNotExist(outArgList, fmt::format("/subsystem:{}", subSystem));
	}
}

/*****************************************************************************/
void CompileToolchainVisualStudio::addEntryPoint(StringList& outArgList) const
{
	const auto entryPoint = getMsvcCompatibleEntryPoint();
	if (!entryPoint.empty())
	{
		List::addIfDoesNotExist(outArgList, fmt::format("/entry:{}", entryPoint));
	}
}

/*****************************************************************************/
void CompileToolchainVisualStudio::addTargetPlatformArch(StringList& outArgList) const
{
	// TODO: /MACHINE - target platform arch
	const auto arch = m_state.info.targetArchitecture();
	switch (arch)
	{
		case Arch::Cpu::X64:
			outArgList.emplace_back("/machine:x64");
			break;

		case Arch::Cpu::X86:
			outArgList.emplace_back("/machine:x86");
			break;

		case Arch::Cpu::ARM:
			outArgList.emplace_back("/machine:arm");
			break;

		case Arch::Cpu::ARM64:
		default:
			// ??
			break;
	}
}

/*****************************************************************************/
void CompileToolchainVisualStudio::addSourceObjects(StringList& outArgList, const StringList& sourceObjs) const
{
	for (auto& source : sourceObjs)
	{
		outArgList.push_back(source);
	}
}

/*****************************************************************************/
void CompileToolchainVisualStudio::addPrecompiledHeaderLink(StringList outArgList) const
{
	if (m_project.usesPch())
	{
		const auto& objDir = m_state.paths.objDir();
		const auto& pch = m_project.pch();
		std::string pchObject = fmt::format("{}/{}.obj", objDir, pch);
		std::string pchInclude = fmt::format("{}/{}.pch", objDir, pch);

		outArgList.emplace_back(std::move(pchObject));
		outArgList.emplace_back(std::move(pchInclude));
	}
}

}
