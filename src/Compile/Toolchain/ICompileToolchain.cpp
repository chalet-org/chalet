/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Toolchain/ICompileToolchain.hpp"

#include "Compile/CompilerConfig.hpp"
#include "FileTemplates/PlatformFileTemplates.hpp"

#include "State/BuildState.hpp"
#include "State/Target/ProjectTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/String.hpp"

#include "Compile/Toolchain/CompileToolchainApple.hpp"
#include "Compile/Toolchain/CompileToolchainGNU.hpp"
#include "Compile/Toolchain/CompileToolchainLLVM.hpp"
#include "Compile/Toolchain/CompileToolchainMSVC.hpp"

namespace chalet
{
/*****************************************************************************/
ICompileToolchain::ICompileToolchain(const BuildState& inState, const ProjectTarget& inProject, const CompilerConfig& inConfig) :
	m_state(inState),
	m_project(inProject),
	m_config(inConfig)
{
	m_quotePaths = m_state.toolchain.strategy() != StrategyType::Native;

	m_isMakefile = m_state.toolchain.strategy() == StrategyType::Makefile;
	m_isNinja = m_state.toolchain.strategy() == StrategyType::Ninja;
	m_isNative = m_state.toolchain.strategy() == StrategyType::Native;
}

/*****************************************************************************/
[[nodiscard]] CompileToolchain ICompileToolchain::make(const ToolchainType inType, const BuildState& inState, const ProjectTarget& inProject, const CompilerConfig& inConfig)
{
	switch (inType)
	{
		case ToolchainType::MSVC:
			return std::make_unique<CompileToolchainMSVC>(inState, inProject, inConfig);
		case ToolchainType::Apple:
			return std::make_unique<CompileToolchainApple>(inState, inProject, inConfig);
		case ToolchainType::LLVM:
			return std::make_unique<CompileToolchainLLVM>(inState, inProject, inConfig);
		case ToolchainType::Unknown:
		case ToolchainType::GNU:
			return std::make_unique<CompileToolchainGNU>(inState, inProject, inConfig);
		default:
			break;
	}

	Diagnostic::errorAbort("Unimplemented ToolchainType requested: ", static_cast<int>(inType));
	return nullptr;
}

/*****************************************************************************/
[[nodiscard]] CompileToolchain ICompileToolchain::make(const CppCompilerType inCompilerType, const BuildState& inState, const ProjectTarget& inProject, const CompilerConfig& inConfig)
{
	switch (inCompilerType)
	{
		case CppCompilerType::AppleClang:
			return std::make_unique<CompileToolchainApple>(inState, inProject, inConfig);
		case CppCompilerType::Clang:
		case CppCompilerType::MingwClang:
		case CppCompilerType::EmScripten:
			return std::make_unique<CompileToolchainLLVM>(inState, inProject, inConfig);
		case CppCompilerType::Intel:
		case CppCompilerType::MingwGcc:
		case CppCompilerType::Gcc:
			return std::make_unique<CompileToolchainGNU>(inState, inProject, inConfig);
		case CppCompilerType::VisualStudio:
			return std::make_unique<CompileToolchainMSVC>(inState, inProject, inConfig);
		case CppCompilerType::Unknown:
		default:
			break;
	}

	Diagnostic::errorAbort("Unimplemented ToolchainType requested: ", static_cast<int>(inCompilerType));
	return nullptr;
}

/*****************************************************************************/
bool ICompileToolchain::initialize()
{
	return true;
}

/*****************************************************************************/
bool ICompileToolchain::createWindowsApplicationManifest()
{
	if (m_project.isStaticLibrary())
		return true;

	auto& sources = m_state.cache.file().sources();

	const auto windowsManifestFile = m_state.paths.getWindowsManifestFilename(m_project);
	const auto windowsManifestResourceFile = m_state.paths.getWindowsManifestResourceFilename(m_project);

	bool manifestChanged = sources.fileChangedOrDoesNotExist(windowsManifestFile);

	if (!windowsManifestFile.empty() && manifestChanged)
	{
		if (!m_isNative && Commands::pathExists(windowsManifestResourceFile))
			Commands::remove(windowsManifestResourceFile);

		if (!Commands::pathExists(windowsManifestFile))
		{
			std::string manifestContents;
			if (m_project.windowsApplicationManifest().empty())
				manifestContents = PlatformFileTemplates::minimumWindowsAppManifest();
			else
				manifestContents = PlatformFileTemplates::generalWindowsAppManifest(m_project.name(), m_state.environment.version(), m_state.info.targetArchitecture());

			String::replaceAll(manifestContents, "\t", " ");

			if (!Commands::createFileWithContents(windowsManifestFile, manifestContents))
			{
				Diagnostic::error("Error creating windows manifest file: {}", windowsManifestFile);
				return false;
			}
		}
	}

	if (!windowsManifestResourceFile.empty() && (sources.fileChangedOrDoesNotExist(windowsManifestResourceFile) || manifestChanged))
	{
		std::string rcContents = PlatformFileTemplates::windowsManifestResource(windowsManifestFile, m_project.isSharedLibrary());
		if (!Commands::createFileWithContents(windowsManifestResourceFile, rcContents))
		{
			Diagnostic::error("Error creating windows manifest resource file: {}", windowsManifestResourceFile);
			return false;
		}
	}

	return true;
}

/*****************************************************************************/
bool ICompileToolchain::createWindowsApplicationIcon()
{
	if (!m_project.isExecutable())
		return true;

	auto& sources = m_state.cache.file().sources();

	const auto& windowsIconFile = m_project.windowsApplicationIcon();
	const auto windowsIconResourceFile = m_state.paths.getWindowsIconResourceFilename(m_project);

	if (!windowsIconFile.empty() && sources.fileChangedOrDoesNotExist(windowsIconFile))
	{
		if (!m_isNative && Commands::pathExists(windowsIconResourceFile))
			Commands::remove(windowsIconResourceFile);

		if (!Commands::pathExists(windowsIconFile))
		{
			Diagnostic::error("Windows icon does not exist: {}", windowsIconFile);
			return false;
		}
	}

	if (!windowsIconResourceFile.empty() && sources.fileChangedOrDependantChanged(windowsIconResourceFile, windowsIconFile))
	{
		std::string rcContents = PlatformFileTemplates::windowsIconResource(windowsIconFile);
		if (!Commands::createFileWithContents(windowsIconResourceFile, rcContents))
		{
			Diagnostic::error("Error creating windows icon resource file: {}", windowsIconResourceFile);
			return false;
		}
	}

	return true;
}

/*****************************************************************************/
void ICompileToolchain::addExectuable(StringList& outArgList, const std::string& inExecutable) const
{
	if (m_isNative)
		outArgList.push_back(inExecutable);
	else
		outArgList.emplace_back(fmt::format("\"{}\"", inExecutable));
}

/*****************************************************************************/
// Compile
/*****************************************************************************/
/*****************************************************************************/
void ICompileToolchain::addIncludes(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ICompileToolchain::addWarnings(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ICompileToolchain::addDefines(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ICompileToolchain::addPchInclude(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ICompileToolchain::addOptimizationOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ICompileToolchain::addLanguageStandard(StringList& outArgList, const CxxSpecialization specialization) const
{
	UNUSED(outArgList, specialization);
}

/*****************************************************************************/
void ICompileToolchain::addDebuggingInformationOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ICompileToolchain::addProfileInformationCompileOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ICompileToolchain::addCompileOptions(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ICompileToolchain::addDiagnosticColorOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ICompileToolchain::addLibStdCppCompileOption(StringList& outArgList, const CxxSpecialization specialization) const
{
	UNUSED(outArgList, specialization);
}

/*****************************************************************************/
void ICompileToolchain::addPositionIndependentCodeOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ICompileToolchain::addNoRunTimeTypeInformationOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ICompileToolchain::addNoExceptionsOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ICompileToolchain::addThreadModelCompileOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}

bool ICompileToolchain::addArchitecture(StringList& outArgList) const
{
	UNUSED(outArgList);

	return true;
}

bool ICompileToolchain::addArchitectureOptions(StringList& outArgList) const
{
	UNUSED(outArgList);

	return true;
}

/*****************************************************************************/
// Linking
/*****************************************************************************/
/*****************************************************************************/
void ICompileToolchain::addLibDirs(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ICompileToolchain::addLinks(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ICompileToolchain::addRunPath(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ICompileToolchain::addStripSymbolsOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ICompileToolchain::addLinkerOptions(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ICompileToolchain::addProfileInformationLinkerOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ICompileToolchain::addLinkTimeOptimizationOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ICompileToolchain::addThreadModelLinkerOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ICompileToolchain::addLinkerScripts(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ICompileToolchain::addLibStdCppLinkerOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ICompileToolchain::addStaticCompilerLibraryOptions(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ICompileToolchain::addPlatformGuiApplicationFlag(StringList& outArgList) const
{
	UNUSED(outArgList);
}
}
