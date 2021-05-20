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
	m_state(inState),
	m_project(inProject),
	m_config(inConfig),
	m_compilerType(m_config.compilerType())
{
	m_quotePaths = m_state.environment.strategy() != StrategyType::Native; // might not be needed
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
		for (auto loc : m_project.locations())
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

	const auto& cc = m_config.compilerExecutable();
	ret.push_back(fmt::format("\"{}\"", cc));
	ret.push_back("/nologo");

	addThreadModelCompileOption(ret);
	addOptimizationOption(ret);

	const auto specialization = m_project.language() == CodeLanguage::CPlusPlus ? CxxSpecialization::Cpp : CxxSpecialization::C;
	addLanguageStandard(ret, specialization);
	addExceptionHandlingModel(ret);
	addWarnings(ret);

	addCompileOptions(ret);
	addNoRunTimeTypeInformationOption(ret);
	addWholeProgramOptimization(ret);

	addDefines(ret);
	addIncludes(ret);

	auto pchObject = outputFile;
	String::replaceAll(pchObject, ".pch", ".obj");

	ret.push_back(getPathCommand("/Fo", pchObject));
	ret.push_back(getPathCommand("/Fp", outputFile));

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

	const auto& rc = m_state.compilerTools.rc();
	ret.push_back(fmt::format("\"{}\"", rc));

	addResourceDefines(ret);
	addIncludes(ret);

	ret.push_back(getPathCommand("/fo", outputFile));

	ret.push_back(inputFile);

	return ret;
}

/*****************************************************************************/
StringList CompileToolchainMSVC::getCxxCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency, const CxxSpecialization specialization)
{
	UNUSED(generateDependency, dependency);

	chalet_assert(!outputFile.empty(), "");

	StringList ret;

	const auto& cc = m_config.compilerExecutable();
	ret.push_back(fmt::format("\"{}\"", cc));
	ret.push_back("/nologo");
	ret.push_back("/MP");

	addThreadModelCompileOption(ret);
	addOptimizationOption(ret);
	addLanguageStandard(ret, specialization);
	addExceptionHandlingModel(ret);
	addWarnings(ret);

	addCompileOptions(ret);
	addNoRunTimeTypeInformationOption(ret);
	addWholeProgramOptimization(ret);

	addDebuggingInformationOption(ret);

	addDefines(ret);
	addIncludes(ret);

	addPchInclude(ret);

	ret.push_back(getPathCommand("/Fo", outputFile));

	ret.push_back("/c");
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
		return getSharedLibTargetCommand(outputFile, sourceObjs);
	}
	else if (kind == ProjectKind::StaticLibrary)
	{
		return getStaticLibTargetCommand(outputFile, sourceObjs, outputFileBase);
	}
	else
	{
		return getExecutableTargetCommand(outputFile, sourceObjs);
	}
}

/*****************************************************************************/
StringList CompileToolchainMSVC::getSharedLibTargetCommand(const std::string& outputFile, const StringList& sourceObjs)
{
	UNUSED(outputFile, sourceObjs);
	return {
		"echo",
		outputFile
	};
}

/*****************************************************************************/
StringList CompileToolchainMSVC::getStaticLibTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase)
{
	UNUSED(outputFile, sourceObjs);

	chalet_assert(!outputFile.empty(), "");

	StringList ret;

	auto& lib = m_state.compilerTools.archiver();
	ret.push_back(fmt::format("\"{}\"", lib));
	ret.push_back("/NOLOGO");

	if (m_state.configuration.linkTimeOptimization())
	{
		// combines w/ /GL - I think this is basically part of MS's link-time optimization
		ret.push_back("/LTCG");
	}

	if (m_project.warningsTreatedAsErrors())
		ret.push_back("/WX");

	// const auto& objDir = m_state.paths.objDir();
	// ret.push_back(fmt::format("/DEF:{}/{}.def", objDir, outputFileBase));
	UNUSED(outputFileBase);

	// TODO: /SUBSYSTEM
	// TODO: /MACHINE - target platform arch
	ret.push_back("/MACHINE:x64");

	ret.push_back(fmt::format("/OUT:{}", outputFile));

	addSourceObjects(ret, sourceObjs);

	return ret;
}

/*****************************************************************************/
StringList CompileToolchainMSVC::getExecutableTargetCommand(const std::string& outputFile, const StringList& sourceObjs)
{
	UNUSED(outputFile, sourceObjs);
	return {
		"echo",
		outputFile
	};
}

/*****************************************************************************/
void CompileToolchainMSVC::addIncludes(StringList& inArgList) const
{
	// inArgList.push_back("/X"); // ignore "Path"

	const std::string prefix = "/I";

	for (const auto& dir : m_project.includeDirs())
	{
		std::string outDir = dir;
		if (String::endsWith('/', outDir))
			outDir.pop_back();

		inArgList.push_back(fmt::format("{}\"{}\"", prefix, outDir));
	}

	for (const auto& dir : m_project.locations())
	{
		std::string outDir = dir;
		if (String::endsWith('/', outDir))
			outDir.pop_back();

		inArgList.push_back(fmt::format("{}\"{}\"", prefix, outDir));
	}
}

/*****************************************************************************/
void CompileToolchainMSVC::addWarnings(StringList& inArgList) const
{
	// TODO: ProjectWarnings::Custom would need to convert GNU warnings to MSVC warning codes

	switch (m_project.warningsPreset())
	{
		case ProjectWarnings::Minimal:
			inArgList.push_back("/W1");
			break;

		case ProjectWarnings::Extra:
			inArgList.push_back("/W2");
			break;

		case ProjectWarnings::Error: {
			inArgList.push_back("/W2");
			inArgList.push_back("/WX");
			break;
		}
		case ProjectWarnings::Pedantic: {
			inArgList.push_back("/W3");
			inArgList.push_back("/WX");
			break;
		}
		case ProjectWarnings::Strict:
		case ProjectWarnings::StrictPedantic: {
			inArgList.push_back("/W4");
			inArgList.push_back("/WX");
			break;
		}
		case ProjectWarnings::VeryStrict: {
			inArgList.push_back("/Wall");
			inArgList.push_back("/WX");
			break;
		}

		case ProjectWarnings::Custom:
		case ProjectWarnings::None:
		default:
			inArgList.push_back("/W3");
			break;
	}
}

/*****************************************************************************/
void CompileToolchainMSVC::addDefines(StringList& inArgList) const
{
	const std::string prefix = "/D";
	for (auto& define : m_project.defines())
	{
		inArgList.push_back(prefix + define);
	}
}

/*****************************************************************************/
void CompileToolchainMSVC::addResourceDefines(StringList& inArgList) const
{
	const std::string prefix = "/d";
	for (auto& define : m_project.defines())
	{
		inArgList.push_back(prefix + define);
	}
}

/*****************************************************************************/
void CompileToolchainMSVC::addExceptionHandlingModel(StringList& inArgList) const
{
	// /EH - Exception handling model
	// s - standard C++ stack unwinding
	// c - functions declared as extern "C" never throw

	inArgList.push_back("/EHsc");
}

/*****************************************************************************/
void CompileToolchainMSVC::addPchInclude(StringList& inArgList) const
{
	// TODO: Potential for more than one pch?
	if (m_project.usesPch())
	{
		const auto objDirPch = m_state.paths.getPrecompiledHeaderTarget(m_project);

		inArgList.push_back(getPathCommand("/Yu", m_pchMinusLocation));

		// /Fp specifies the location of the PCH object file
		inArgList.push_back(getPathCommand("/Fp", objDirPch));

		// /FI force-includes the PCH source file so one doesn't need to use the #include directive in every file
		inArgList.push_back(getPathCommand("/FI", m_pchMinusLocation));
	}
}

/*****************************************************************************/
void CompileToolchainMSVC::addOptimizationOption(StringList& inArgList) const
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

	inArgList.push_back(std::move(opt));
}

/*****************************************************************************/
void CompileToolchainMSVC::addLanguageStandard(StringList& inArgList, const CxxSpecialization specialization) const
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
			inArgList.push_back("/std:c17");
		}
		else
		{
			inArgList.push_back("/std:c11");
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
			inArgList.push_back("/std:c++latest");
		}
		else if (String::equals(langStandard, "c++17")
			|| String::equals(langStandard, "c++1z")
			|| String::equals(langStandard, "gnu++17")
			|| String::equals(langStandard, "gnu++1z"))
		{
			inArgList.push_back("/std:c++17");
		}
		else
		{
			inArgList.push_back("/std:c++14");
		}
	}
}

/*****************************************************************************/
void CompileToolchainMSVC::addDebuggingInformationOption(StringList& inArgList) const
{
	// TODO! - pdb files etc
	// /Zi /ZI /debug
	UNUSED(inArgList);
}

/*****************************************************************************/
void CompileToolchainMSVC::addCompileOptions(StringList& inArgList) const
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void CompileToolchainMSVC::addNoRunTimeTypeInformationOption(StringList& inArgList) const
{
	if (!m_project.rtti())
	{
		List::addIfDoesNotExist(inArgList, "/GR-");
	}
}

/*****************************************************************************/
void CompileToolchainMSVC::addThreadModelCompileOption(StringList& inArgList) const
{
	// /MD - multithreaded dll
	// /MDd - debug multithreaded dll
	// /MT - multithreaded executable
	// /MTd - debug multithreaded executable

	// TODO: at the moment, assumes threaded

	if (m_project.isExecutable())
	{
		if (m_state.configuration.debugSymbols())
			inArgList.push_back("/MTd");
		else
			inArgList.push_back("/MT");
	}
	else
	{
		if (m_state.configuration.debugSymbols())
			inArgList.push_back("/MDd");
		else
			inArgList.push_back("/MD");
	}
}

/*****************************************************************************/
void CompileToolchainMSVC::addWholeProgramOptimization(StringList& inArgList) const
{
	if (m_state.configuration.linkTimeOptimization())
	{
		inArgList.push_back("/GL");
	}
}

/*****************************************************************************/
// void addLibDirs(StringList& inArgList);

std::string CompileToolchainMSVC::getPathCommand(std::string_view inCmd, const std::string& inPath) const
{
	if (m_quotePaths)
		return fmt::format("{}\"{}\"", inCmd, inPath);
	else
		return fmt::format("{}{}", inCmd, inPath);
}

/*****************************************************************************/
void CompileToolchainMSVC::addSourceObjects(StringList& inArgList, const StringList& sourceObjs) const
{
	for (auto& source : sourceObjs)
	{
		inArgList.push_back(source);
	}
}

}
