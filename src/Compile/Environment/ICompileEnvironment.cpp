/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Environment/ICompileEnvironment.hpp"

#include "Cache/WorkspaceCache.hpp"
#include "Compile/ToolchainTypes.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Utility/DefinesExperimental.hpp"
#include "Utility/String.hpp"

#include "Compile/Environment/CompileEnvironmentAppleLLVM.hpp"
#include "Compile/Environment/CompileEnvironmentGNU.hpp"
#include "Compile/Environment/CompileEnvironmentIntel.hpp"
#include "Compile/Environment/CompileEnvironmentLLVM.hpp"
#include "Compile/Environment/CompileEnvironmentVisualStudio.hpp"
#include "Compile/Environment/CompileEnvironmentVisualStudioLLVM.hpp"

namespace chalet
{
const std::string& ICompileEnvironment::identifier() const noexcept
{
	return m_identifier;
}
/*****************************************************************************/
ToolchainType ICompileEnvironment::type() const noexcept
{
	return m_type;
}

/*****************************************************************************/
bool ICompileEnvironment::isWindowsTarget() const noexcept
{
	return m_isWindowsTarget;
}

/*****************************************************************************/
bool ICompileEnvironment::isEmbeddedTarget() const noexcept
{
	return m_isEmbeddedTarget;
}

/*****************************************************************************/
bool ICompileEnvironment::isWindowsClang() const noexcept
{
#if defined(CHALET_WIN32)
	return m_type == ToolchainType::LLVM
		|| m_type == ToolchainType::VisualStudioLLVM
		|| m_type == ToolchainType::IntelLLVM;
#else
	return false;
#endif
}

/*****************************************************************************/
bool ICompileEnvironment::isMsvcClang() const noexcept
{
	return m_type == ToolchainType::VisualStudioLLVM;
}

/*****************************************************************************/
bool ICompileEnvironment::isClang() const noexcept
{
	return m_type == ToolchainType::LLVM
		|| m_type == ToolchainType::AppleLLVM
		|| m_type == ToolchainType::VisualStudioLLVM
		|| m_type == ToolchainType::IntelLLVM
		|| m_type == ToolchainType::MingwLLVM
		|| m_type == ToolchainType::EmScripten;
}

/*****************************************************************************/
bool ICompileEnvironment::isAppleClang() const noexcept
{
	return m_type == ToolchainType::AppleLLVM;
}

/*****************************************************************************/
bool ICompileEnvironment::isGcc() const noexcept
{
	return m_type == ToolchainType::GNU
#if defined(CHALET_MACOS) || defined(CHALET_LINUX)
		|| m_type == ToolchainType::IntelClassic
#endif
		|| m_type == ToolchainType::MingwGNU;
}

/*****************************************************************************/
bool ICompileEnvironment::isIntelClassic() const noexcept
{
	return m_type == ToolchainType::IntelClassic;
}

/*****************************************************************************/
bool ICompileEnvironment::isMingw() const noexcept
{
	return isMingwGcc() || isMingwClang();
}

/*****************************************************************************/
bool ICompileEnvironment::isMingwGcc() const noexcept
{
	return m_type == ToolchainType::MingwGNU;
}

/*****************************************************************************/
bool ICompileEnvironment::isMingwClang() const noexcept
{
	return m_type == ToolchainType::MingwLLVM;
}

/*****************************************************************************/
bool ICompileEnvironment::isMsvc() const noexcept
{
	return m_type == ToolchainType::VisualStudio;
}

/*****************************************************************************/
bool ICompileEnvironment::isClangOrMsvc() const noexcept
{
	return isClang() || isMsvc();
}

/*****************************************************************************/
const std::string& ICompileEnvironment::detectedVersion() const
{
	return m_detectedVersion;
}

/*****************************************************************************/
std::string ICompileEnvironment::getMajorVersion() const
{
	std::string ret = m_detectedVersion;

	auto firstDecimal = ret.find('.');
	if (firstDecimal != std::string::npos)
	{
		ret = ret.substr(0, firstDecimal);
	}

	return ret;
}

/*****************************************************************************/
bool ICompileEnvironment::isCompilerFlagSupported(const std::string& inFlag) const
{
	return m_supportedFlags.find(inFlag) != m_supportedFlags.end();
}

/*****************************************************************************/
// Protected/Private Town
//
ICompileEnvironment::ICompileEnvironment(const ToolchainType inType, BuildState& inState) :
	m_state(inState),
	m_type(inType)
{
	UNUSED(m_state.inputs, m_state);
}

/*****************************************************************************/
[[nodiscard]] Unique<ICompileEnvironment> ICompileEnvironment::make(ToolchainType type, BuildState& inState)
{
	CustomToolchainTreatAs treatAs = inState.toolchain.treatAs();
	if (treatAs != CustomToolchainTreatAs::None)
	{
		switch (treatAs)
		{
			case CustomToolchainTreatAs::LLVM:
				type = ToolchainType::LLVM;
				break;
			case CustomToolchainTreatAs::GCC:
				type = ToolchainType::GNU;
				break;
			default:
				return nullptr;
		}
	}
	else if (type == ToolchainType::Unknown)
	{
		auto& compiler = inState.toolchain.compilerCxxAny().path;
		type = ICompileEnvironment::detectToolchainTypeFromPath(compiler);
		if (type == ToolchainType::Unknown)
		{
			// Diagnostic::error("Toolchain was not recognized from compiler path: '{}'", compiler);
			return nullptr;
		}
	}

	switch (type)
	{
		case ToolchainType::VisualStudio:
			return std::make_unique<CompileEnvironmentVisualStudio>(type, inState);
		case ToolchainType::AppleLLVM:
			return std::make_unique<CompileEnvironmentAppleLLVM>(type, inState);
		case ToolchainType::LLVM:
		case ToolchainType::MingwLLVM:
			return std::make_unique<CompileEnvironmentLLVM>(type, inState);
		case ToolchainType::GNU:
		case ToolchainType::MingwGNU:
			return std::make_unique<CompileEnvironmentGNU>(type, inState);
		case ToolchainType::VisualStudioLLVM:
			return std::make_unique<CompileEnvironmentVisualStudioLLVM>(type, inState);
		case ToolchainType::IntelClassic:
		case ToolchainType::IntelLLVM:
#if CHALET_EXPERIMENTAL_ENABLE_INTEL_ICC || CHALET_EXPERIMENTAL_ENABLE_INTEL_ICX
			return std::make_unique<CompileEnvironmentIntel>(type, inState);
#endif
		case ToolchainType::Unknown:
		default:
			break;
	}

	Diagnostic::error("Unimplemented ToolchainType requested: ", static_cast<int>(type));
	return nullptr;
}

/*****************************************************************************/
bool ICompileEnvironment::create(const std::string& inVersion)
{
	if (m_initialized)
	{
		Diagnostic::error("Compiler environment was already initialized.");
		return false;
	}

	m_initialized = true;
	m_identifier = ToolchainTypes::getTypeName(this->type());

	if (!validateArchitectureFromInput())
		return false;

	if (!createFromVersion(inVersion))
		return false;

	return true;
}

/*****************************************************************************/
bool ICompileEnvironment::getCompilerInfoFromExecutable(CompilerInfo& outInfo)
{
	if (outInfo.path.empty())
	{
		Diagnostic::error("Compiler executable was unexpectedly blank: '{}'", outInfo.path);
		return false;
	}

	if (!getCompilerPaths(outInfo))
	{
		Diagnostic::error("Unexpected compiler toolchain structure found from executable: '{}'", outInfo.path);
		return false;
	}

	if (!getCompilerVersionAndDescription(outInfo))
	{
		Diagnostic::error("Error getting the version and description for: '{}'", outInfo.path);
		return false;
	}

	if (!makeSupportedCompilerFlags(outInfo.path))
	{
		Diagnostic::error("Error collecting supported compiler flags.");
		return false;
	}

	return true;
}

/*****************************************************************************/
bool ICompileEnvironment::makeSupportedCompilerFlags(const std::string& inExecutable)
{
	if (!supportsFlagFile())
		return true;

	std::string flagsFile = m_state.cache.getHashPath(fmt::format("flags_{}.env", inExecutable), CacheType::Local);
	if (!Commands::pathExists(flagsFile))
	{
		if (populateSupportedFlags(inExecutable))
		{
			std::string outContents;
			for (auto& [flag, _] : m_supportedFlags)
			{
				outContents += flag + "\n";
			}

			std::ofstream(flagsFile) << outContents;

			m_state.cache.file().addExtraHash(String::getPathFilename(flagsFile));
		}
	}
	else
	{
		std::ifstream input(flagsFile);
		for (std::string line; std::getline(input, line);)
		{
			m_supportedFlags[std::move(line)] = true;
		}

		m_state.cache.file().addExtraHash(String::getPathFilename(flagsFile));
	}

	return true;
}

/*****************************************************************************/
bool ICompileEnvironment::populateSupportedFlags(const std::string& inExecutable)
{
	UNUSED(inExecutable);
	return true;
}

/*****************************************************************************/
void ICompileEnvironment::generateTargetSystemPaths()
{
}

/*****************************************************************************/
std::string ICompileEnvironment::getObjectFile(const std::string& inSource) const
{
	return fmt::format("{}/{}.o", m_state.paths.objDir(), m_state.paths.getNormalizedOutputPath(inSource));
}

/*****************************************************************************/
std::string ICompileEnvironment::getAssemblyFile(const std::string& inSource) const
{
	return fmt::format("{}/{}.o.asm", m_state.paths.asmDir(), m_state.paths.getNormalizedOutputPath(inSource));
}

/*****************************************************************************/
std::string ICompileEnvironment::getWindowsResourceObjectFile(const std::string& inSource) const
{
	return fmt::format("{}/{}.res", m_state.paths.objDir(), m_state.paths.getNormalizedOutputPath(inSource));
}

/*****************************************************************************/
std::string ICompileEnvironment::getDependencyFile(const std::string& inSource) const
{
	return fmt::format("{}/{}.d", m_state.paths.depDir(), m_state.paths.getNormalizedOutputPath(inSource));
}

/*****************************************************************************/
std::string ICompileEnvironment::getModuleDirectivesDependencyFile(const std::string& inSource) const
{
	// Note: This isn't an actual convention, just a placeholder until GCC/Clang have one
	return fmt::format("{}/{}.d.module", m_state.paths.depDir(), m_state.paths.getNormalizedOutputPath(inSource));
}

/*****************************************************************************/
std::string ICompileEnvironment::getModuleBinaryInterfaceFile(const std::string& inSource) const
{
	// Note: This isn't an actual convention, just a placeholder until GCC/Clang have one
	return fmt::format("{}/{}.bmi", m_state.paths.objDir(), m_state.paths.getNormalizedOutputPath(inSource));
}

/*****************************************************************************/
std::string ICompileEnvironment::getModuleBinaryInterfaceDependencyFile(const std::string& inSource) const
{
	// Note: This isn't an actual convention, just a placeholder until GCC/Clang have one
	return fmt::format("{}/{}.bmi.d", m_state.paths.depDir(), m_state.paths.getNormalizedOutputPath(inSource));
}

/*****************************************************************************/
const std::string& ICompileEnvironment::sysroot() const noexcept
{
	return m_sysroot;
}

const std::string& ICompileEnvironment::targetSystemVersion() const noexcept
{
	return m_targetSystemVersion;
}

/*****************************************************************************/
const StringList& ICompileEnvironment::targetSystemPaths() const noexcept
{
	return m_targetSystemPaths;
}

/*****************************************************************************/
bool ICompileEnvironment::getCompilerPaths(CompilerInfo& outInfo) const
{
	std::string path = String::getPathFolder(outInfo.path);
	const std::string lowercasePath = String::toLowerCase(path);

	std::vector<CompilerPathStructure> compilerStructures = getValidCompilerPaths();

	for (const auto& [binDir, libDir, includeDir] : compilerStructures)
	{
		if (!String::endsWith(binDir, lowercasePath))
			continue;

		auto tmp = path.substr(0, path.size() - binDir.size());

		{
			std::string tmpLib = fmt::format("{}{}", tmp, libDir);
			std::string tmpInclude = fmt::format("{}{}", tmp, includeDir);
			if (!Commands::pathExists(tmpLib) || !Commands::pathExists(tmpInclude))
				continue;
		}

		path = std::move(tmp);

#if defined(CHALET_MACOS)
		const auto& xcodePath = Commands::getXcodePath();
		String::replaceAll(path, xcodePath, "");
		String::replaceAll(path, "/Toolchains/XcodeDefault.xctoolchain", "");
#endif
		outInfo.binDir = path + binDir;
		outInfo.libDir = path + libDir;
		outInfo.includeDir = path + includeDir;

		return true;
	}

	return false;
}

/*****************************************************************************/
bool ICompileEnvironment::createFromVersion(const std::string& inVersion)
{
	UNUSED(inVersion);
	return true;
}

/*****************************************************************************/
bool ICompileEnvironment::validateArchitectureFromInput()
{
	return true;
}

/*****************************************************************************/
bool ICompileEnvironment::readArchitectureTripleFromCompiler()
{
	return true;
}

/*****************************************************************************/
bool ICompileEnvironment::compilerVersionIsToolchainVersion() const
{
	return true;
}

/*****************************************************************************/
std::string ICompileEnvironment::getVarsPath(const std::string& inUniqueId) const
{
	const auto id = identifier();
	const auto hostArch = static_cast<std::underlying_type_t<Arch::Cpu>>(m_state.info.hostArchitecture());
	const auto& archString = m_state.info.targetArchitectureTriple();
	// auto archString = m_state.inputs.getArchWithOptionsAsString(m_state.info.targetArchitectureTriple());
	const auto& uniqueId = String::equals('0', inUniqueId) ? m_state.inputs.toolchainPreferenceName() : inUniqueId;

	return m_state.cache.getHashPath(fmt::format("{}_{}_{}_{}.env", id, hostArch, archString, uniqueId), CacheType::Local);
}

/*****************************************************************************/
ToolchainType ICompileEnvironment::detectToolchainTypeFromPath(const std::string& inExecutable)
{
	if (inExecutable.empty())
		return ToolchainType::Unknown;

	auto executable = String::toLowerCase(inExecutable);

#if defined(CHALET_WIN32)
	if (String::endsWith("/cl.exe", executable))
		return ToolchainType::VisualStudio;
#endif

#if CHALET_EXPERIMENTAL_ENABLE_INTEL_ICC
	#if defined(CHALET_WIN32)
	if (String::endsWith("/icl.exe", inExecutable))
	#else
	if (String::endsWith(StringList{ "/icpc", "/icc" }, inExecutable))
	#endif
		return ToolchainType::IntelClassic;
#endif

#if CHALET_EXPERIMENTAL_ENABLE_INTEL_ICX
	if (
	#if defined(CHALET_WIN32)
		String::endsWith("/icx.exe", executable)
	#else
		String::endsWith("/icx", executable)
	#endif
		|| String::contains(StringList{ "onepi", "intel" }, executable))
		return ToolchainType::IntelLLVM;
#endif

	if (String::contains("clang", executable))
	{
#if defined(CHALET_WIN32)
		StringList vsLLVM{
			"/vc/tools/llvm/x64/bin/clang.exe",
			"/vc/tools/llvm/x64/bin/clang++.exe",
			"/vc/tools/llvm/bin/clang.exe",
			"/vc/tools/llvm/bin/clang++.exe",
		};
		if (String::endsWith(vsLLVM, executable))
			return ToolchainType::VisualStudioLLVM;
#elif defined(CHALET_MACOS)
		if (String::contains(StringList{ "contents/developer", "code" }, executable))
			return ToolchainType::AppleLLVM;
		else if (String::contains("developer/commandlinetools", executable))
			return ToolchainType::AppleLLVM;
#endif

		return ToolchainType::LLVM;
	}

	if (String::contains(StringList{ "gcc", "g++" }, executable))
	{
#if defined(CHALET_MACOS)
		if (String::contains(StringList{ "contents/developer", "xcode" }, executable))
			return ToolchainType::AppleLLVM;
		else if (String::contains("developer/commandlinetools", executable))
			return ToolchainType::AppleLLVM;
#endif

#if defined(CHALET_WIN32)
		return ToolchainType::MingwGNU;
#else
		if (String::contains("mingw", executable))
			return ToolchainType::MingwGNU;

		return ToolchainType::GNU;
#endif
	}

	return ToolchainType::Unknown;
}

}
