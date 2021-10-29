/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CompilerCxx/CompilerCxxVisualStudioCL.hpp"

#include "State/BuildConfiguration.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CompilerCxxVisualStudioCL::CompilerCxxVisualStudioCL(const BuildState& inState, const SourceTarget& inProject) :
	ICompilerCxx(inState, inProject)
{
}

/*****************************************************************************/
bool CompilerCxxVisualStudioCL::initialize()
{
	if (!createPrecompiledHeaderSource())
		return false;

	return true;
}

/*****************************************************************************/
bool CompilerCxxVisualStudioCL::createPrecompiledHeaderSource()
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

	return true;
}

/*****************************************************************************/
StringList CompilerCxxVisualStudioCL::getPrecompiledHeaderCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency, const std::string& arch)
{
	UNUSED(generateDependency, dependency, arch);

	chalet_assert(!outputFile.empty(), "");

	StringList ret;

	auto& executable = m_state.toolchain.compilerCxx(m_project.language()).path;

	if (executable.empty())
		return ret;

	ret.emplace_back(getQuotedExecutablePath(executable));
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
StringList CompilerCxxVisualStudioCL::getCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency, const CxxSpecialization specialization)
{
	UNUSED(generateDependency, dependency);

	chalet_assert(!outputFile.empty(), "");

	StringList ret;

	auto& executable = m_state.toolchain.compilerCxx(m_project.language()).path;

	if (executable.empty())
		return ret;

	ret.emplace_back(getQuotedExecutablePath(executable));
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
void CompilerCxxVisualStudioCL::addIncludes(StringList& outArgList) const
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
void CompilerCxxVisualStudioCL::addWarnings(StringList& outArgList) const
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
void CompilerCxxVisualStudioCL::addDefines(StringList& outArgList) const
{
	const std::string prefix = "/D";
	for (auto& define : m_project.defines())
	{
		outArgList.emplace_back(prefix + define);
	}
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addPchInclude(StringList& outArgList) const
{
	// TODO: Potential for more than one pch?
	if (m_project.usesPch())
	{
		const auto objDirPch = m_state.paths.getPrecompiledHeaderTarget(m_project);

		outArgList.emplace_back(getPathCommand("/Yu", m_pchMinusLocation));

		// /Fp specifies the location of the PCH object file
		outArgList.emplace_back(getPathCommand("/Fp", objDirPch));

		// /FI force-includes the PCH source file so one doesn't need to use the #include directive in every file
		outArgList.emplace_back(getPathCommand("/FI", m_pchMinusLocation));
	}
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addOptimizationOption(StringList& outArgList) const
{
	std::string opt;

	OptimizationLevel level = m_state.configuration.optimizationLevel();

	if (m_state.configuration.debugSymbols()
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

	if (m_state.configuration.debugSymbols())
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
void CompilerCxxVisualStudioCL::addLanguageStandard(StringList& outArgList, const CxxSpecialization specialization) const
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
void CompilerCxxVisualStudioCL::addDebuggingInformationOption(StringList& outArgList) const
{
	// TODO! - pdb files etc
	// /Zi /ZI /debug
	UNUSED(outArgList);
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addCompileOptions(StringList& outArgList) const
{
	for (auto& option : m_project.compileOptions())
	{
		List::addIfDoesNotExist(outArgList, option);
	}
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addNoRunTimeTypeInformationOption(StringList& outArgList) const
{
	if (!m_project.rtti())
	{
		List::addIfDoesNotExist(outArgList, "/GR-");
	}
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addNoExceptionsOption(StringList& outArgList) const
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
void CompilerCxxVisualStudioCL::addThreadModelCompileOption(StringList& outArgList) const
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
void CompilerCxxVisualStudioCL::addResourceDefines(StringList& outArgList) const
{
	const std::string prefix = "/d";
	for (auto& define : m_project.defines())
	{
		outArgList.emplace_back(prefix + define);
	}
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addDiagnosticsOption(StringList& outArgList) const
{
	outArgList.emplace_back("/diagnostics:caret");
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addWholeProgramOptimization(StringList& outArgList) const
{
	if (m_state.configuration.linkTimeOptimization())
	{
		outArgList.emplace_back("/GL");
	}
}

}
