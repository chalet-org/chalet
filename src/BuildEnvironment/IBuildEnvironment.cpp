/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "BuildEnvironment/IBuildEnvironment.hpp"

#include "Cache/WorkspaceCache.hpp"
#include "Compile/ToolchainTypes.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Process/Environment.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "System/Files.hpp"
#include "Utility/Hash.hpp"
#include "Utility/Path.hpp"
#include "Utility/String.hpp"

#include "BuildEnvironment/BuildEnvironmentAppleLLVM.hpp"
#include "BuildEnvironment/BuildEnvironmentEmscripten.hpp"
#include "BuildEnvironment/BuildEnvironmentGNU.hpp"
#include "BuildEnvironment/BuildEnvironmentIntel.hpp"
#include "BuildEnvironment/BuildEnvironmentLLVM.hpp"
#include "BuildEnvironment/BuildEnvironmentVisualStudio.hpp"
#include "BuildEnvironment/BuildEnvironmentVisualStudioLLVM.hpp"

namespace chalet
{
const std::string& IBuildEnvironment::identifier() const noexcept
{
	return m_identifier;
}
/*****************************************************************************/
ToolchainType IBuildEnvironment::type() const noexcept
{
	return m_type;
}

/*****************************************************************************/
bool IBuildEnvironment::isWindowsTarget() const noexcept
{
	// Windows Clang should always target windows because it uses the MSVC abi
	// Mingw checks should also set m_isWindowsTarget, but just in case something went wrong
	//
	return m_isWindowsTarget || isWindowsClang() || isMingwGcc() || isMingwClang();
}

/*****************************************************************************/
bool IBuildEnvironment::isEmbeddedTarget() const noexcept
{
	return m_isEmbeddedTarget;
}

/*****************************************************************************/
bool IBuildEnvironment::isWindowsClang() const noexcept
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
bool IBuildEnvironment::isMsvcClang() const noexcept
{
	return m_type == ToolchainType::VisualStudioLLVM;
}

/*****************************************************************************/
bool IBuildEnvironment::isClang() const noexcept
{
	return m_type == ToolchainType::LLVM
		|| m_type == ToolchainType::AppleLLVM
		|| m_type == ToolchainType::VisualStudioLLVM
		|| m_type == ToolchainType::IntelLLVM
		|| m_type == ToolchainType::MingwLLVM
		|| m_type == ToolchainType::Emscripten;
}

/*****************************************************************************/
bool IBuildEnvironment::isAppleClang() const noexcept
{
	return m_type == ToolchainType::AppleLLVM;
}

/*****************************************************************************/
bool IBuildEnvironment::isGcc() const noexcept
{
	return m_type == ToolchainType::GNU
#if defined(CHALET_MACOS) || defined(CHALET_LINUX)
		|| m_type == ToolchainType::IntelClassic
#endif
		|| m_type == ToolchainType::MingwGNU;
}

/*****************************************************************************/
bool IBuildEnvironment::isIntelClassic() const noexcept
{
	return m_type == ToolchainType::IntelClassic;
}

/*****************************************************************************/
bool IBuildEnvironment::isMingw() const noexcept
{
	return isMingwGcc() || isMingwClang();
}

/*****************************************************************************/
bool IBuildEnvironment::isMingwGcc() const noexcept
{
	return m_type == ToolchainType::MingwGNU;
}

/*****************************************************************************/
bool IBuildEnvironment::isMingwClang() const noexcept
{
	return m_type == ToolchainType::MingwLLVM;
}

/*****************************************************************************/
bool IBuildEnvironment::isMsvc() const noexcept
{
	return m_type == ToolchainType::VisualStudio;
}

/*****************************************************************************/
bool IBuildEnvironment::isEmscripten() const noexcept
{
	return m_type == ToolchainType::Emscripten;
}

/*****************************************************************************/
const std::string& IBuildEnvironment::detectedVersion() const
{
	return m_detectedVersion;
}

/*****************************************************************************/
std::string IBuildEnvironment::getMajorVersion() const
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
bool IBuildEnvironment::isCompilerFlagSupported(const std::string& inFlag) const
{
	return m_supportedFlags.empty() || m_supportedFlags.find(inFlag) != m_supportedFlags.end();
}

/*****************************************************************************/
// Protected/Private Town
//
IBuildEnvironment::IBuildEnvironment(const ToolchainType inType, BuildState& inState) :
	m_state(inState),
	m_type(inType)
{
	UNUSED(m_state.inputs, m_state);
}

/*****************************************************************************/
[[nodiscard]] Unique<IBuildEnvironment> IBuildEnvironment::make(ToolchainType type, BuildState& inState)
{
	if (type == ToolchainType::Unknown)
	{
		auto& compiler = inState.toolchain.compilerCxxAny().path;
		type = IBuildEnvironment::detectToolchainTypeFromPath(compiler, inState);
		if (type == ToolchainType::Unknown)
		{
			// Diagnostic::error("Toolchain was not recognized from compiler path: '{}'", compiler);
			return nullptr;
		}
	}

	switch (type)
	{
		case ToolchainType::VisualStudio:
			return std::make_unique<BuildEnvironmentVisualStudio>(type, inState);
		case ToolchainType::AppleLLVM:
			return std::make_unique<BuildEnvironmentAppleLLVM>(type, inState);
		case ToolchainType::LLVM:
		case ToolchainType::MingwLLVM:
			return std::make_unique<BuildEnvironmentLLVM>(type, inState);
		case ToolchainType::GNU:
		case ToolchainType::MingwGNU:
			return std::make_unique<BuildEnvironmentGNU>(type, inState);
		case ToolchainType::VisualStudioLLVM:
			return std::make_unique<BuildEnvironmentVisualStudioLLVM>(type, inState);
		case ToolchainType::Emscripten:
			return std::make_unique<BuildEnvironmentEmscripten>(type, inState);
		case ToolchainType::IntelClassic:
		case ToolchainType::IntelLLVM:
#if CHALET_ENABLE_INTEL_ICC || CHALET_ENABLE_INTEL_ICX
			return std::make_unique<BuildEnvironmentIntel>(type, inState);
#endif
		case ToolchainType::Unknown:
		default:
			break;
	}

	Diagnostic::error("Unimplemented ToolchainType requested: {}", static_cast<i32>(type));
	return nullptr;
}

/*****************************************************************************/
bool IBuildEnvironment::create(const std::string& inVersion, const bool inRefreshCache)
{
	if (m_initialized)
	{
		Diagnostic::error("Compiler environment was already initialized.");
		return false;
	}

	m_initialized = true;
	m_identifier = ToolchainTypes::getTypeName(this->type());
	m_refreshCache = inRefreshCache;

	if (m_refreshCache)
	{
		auto& cache = m_state.cache.file();
		cache.resetDataCache();
	}

	if (!validateArchitectureFromInput())
		return false;

	if (!createFromVersion(inVersion))
		return false;

	return true;
}

/*****************************************************************************/
bool IBuildEnvironment::getCompilerInfoFromExecutable(CompilerInfo& outInfo)
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
bool IBuildEnvironment::makeSupportedCompilerFlags(const std::string& inExecutable)
{
	if (!supportsFlagFile())
		return true;

	std::string flagsFile = m_state.cache.getHashPath(fmt::format("flags_{}.env", inExecutable));
	if (!Files::pathExists(flagsFile))
	{
		if (populateSupportedFlags(inExecutable))
		{
			std::string outContents;
			for (auto& [flag, _] : m_supportedFlags)
			{
				outContents += flag + "\n";
			}

			Files::ofstream(flagsFile) << outContents;

			m_state.cache.file().addExtraHash(String::getPathFilename(flagsFile));
		}
	}
	else
	{
		auto input = Files::ifstream(flagsFile);
		std::string line;
		auto lineEnd = input.widen('\n');
		while (std::getline(input, line, lineEnd))
		{
			m_supportedFlags[std::move(line)] = true;
		}

		m_state.cache.file().addExtraHash(String::getPathFilename(flagsFile));
	}

	return true;
}

/*****************************************************************************/
void IBuildEnvironment::getDataWithCache(std::string& outData, const std::string& inId, const std::string& inItem, const std::function<std::string()>& onGet)
{
	auto& cache = m_state.cache.file();
	auto hash = Hash::string(fmt::format("{}_{}.txt", inItem, inId));

	outData = cache.getDataValue(hash, onGet);
}

/*****************************************************************************/
bool IBuildEnvironment::populateSupportedFlags(const std::string& inExecutable)
{
	UNUSED(inExecutable);
	return true;
}

/*****************************************************************************/
const std::string& IBuildEnvironment::commandInvoker() const
{
	return m_commandInvoker;
}

/*****************************************************************************/
void IBuildEnvironment::generateTargetSystemPaths()
{
}

/*****************************************************************************/
std::string IBuildEnvironment::getLibraryPrefix(const bool inMingwUnix) const
{
	bool mingw = isMingw();
	bool mingwWithPrefix = inMingwUnix && mingw;
	bool nonWindows = !mingw && !isMsvc() && !isWindowsClang();

	if (mingwWithPrefix || nonWindows)
		return "lib";
	else
		return std::string();
}

/*****************************************************************************/
std::string IBuildEnvironment::getExecutableExtension() const
{
	if (isWindowsTarget())
		return ".exe";
	else
		return std::string();
}

/*****************************************************************************/
std::string IBuildEnvironment::getSharedLibraryExtension() const
{
	if (isWindowsTarget())
		return ".dll";

	return Files::getPlatformSharedLibraryExtension();
}

/*****************************************************************************/
bool IBuildEnvironment::supportsCppModules() const
{
	auto& inputFile = m_state.inputs.inputFile();
	Diagnostic::error("{}: C++ modules are not supported by this toolchain.", inputFile);
	return false;
}

/*****************************************************************************/
std::string IBuildEnvironment::getObjectFile(const std::string& inSource) const
{
	return fmt::format("{}/{}.o", m_state.paths.objDir(), m_state.paths.getNormalizedOutputPath(inSource));
}

/*****************************************************************************/
std::string IBuildEnvironment::getAssemblyFile(const std::string& inSource) const
{
	return fmt::format("{}/{}.o.asm", m_state.paths.asmDir(), m_state.paths.getNormalizedOutputPath(inSource));
}

/*****************************************************************************/
std::string IBuildEnvironment::getPrecompiledHeaderIntermediateFile(const SourceTarget& inProject) const
{
	auto pchName = String::getPathFilename(inProject.precompiledHeader());
	return fmt::format("{}/{}", m_state.paths.intermediateDir(inProject), pchName);
}

/*****************************************************************************/
std::string IBuildEnvironment::getPrecompiledHeaderSourceFile(const SourceTarget& inProject) const
{
	// Most compilers don't use this, so return an empty string

	UNUSED(inProject);
	return std::string();
}

/*****************************************************************************/
std::string IBuildEnvironment::getWindowsResourceObjectFile(const std::string& inSource) const
{
	return fmt::format("{}/{}.res", m_state.paths.objDir(), m_state.paths.getNormalizedOutputPath(inSource));
}

/*****************************************************************************/
std::string IBuildEnvironment::getDependencyFile(const std::string& inSource) const
{
	return fmt::format("{}/{}.d", m_state.paths.depDir(), m_state.paths.getNormalizedOutputPath(inSource));
}

/*****************************************************************************/
StringList IBuildEnvironment::getSystemIncludeDirectories(const std::string& inExecutable)
{
	UNUSED(inExecutable);
	chalet_assert(false, "getSystemIncludeDirectories not implmeneted for Build environment");
	return StringList{};
}

/*****************************************************************************/
const std::string& IBuildEnvironment::sysroot() const noexcept
{
	return m_sysroot;
}

const std::string& IBuildEnvironment::targetSystemVersion() const noexcept
{
	return m_targetSystemVersion;
}

/*****************************************************************************/
const StringList& IBuildEnvironment::targetSystemPaths() const noexcept
{
	return m_targetSystemPaths;
}

/*****************************************************************************/
bool IBuildEnvironment::getCompilerPaths(CompilerInfo& outInfo) const
{
	auto path = String::getPathFolder(outInfo.path);
	const auto lowercasePath = String::toLowerCase(path);

	auto compilerStructures = getValidCompilerPaths();

	for (const auto& [binDir, libDir, includeDir] : compilerStructures)
	{
		if (!String::endsWith(binDir, lowercasePath))
			continue;

		auto tmp = path.substr(0, path.size() - binDir.size());

		{
			auto tmpLib = fmt::format("{}{}", tmp, libDir);
			auto tmpInclude = fmt::format("{}{}", tmp, includeDir);

			if (!Files::pathExists(tmpLib) || !Files::pathExists(tmpInclude))
				continue;
		}

		path = std::move(tmp);

#if defined(CHALET_WIN32)
		if (this->isMsvc())
		{
			if (String::contains("hostx64", binDir))
				m_state.info.setHostArchitecture("x86_64");
			else if (String::contains("hostx86", binDir))
				m_state.info.setHostArchitecture("i686");
			else if (String::contains("hostarm64", binDir))
				m_state.info.setHostArchitecture("arm64");
		}
#elif defined(CHALET_MACOS)
		Path::stripXcodeToolchain(path);
#endif
		outInfo.binDir = path + binDir;
		outInfo.libDir = path + libDir;
		outInfo.includeDir = path + includeDir;

		return true;
	}

	return false;
}

/*****************************************************************************/
bool IBuildEnvironment::createFromVersion(const std::string& inVersion)
{
	UNUSED(inVersion);
	return true;
}

/*****************************************************************************/
bool IBuildEnvironment::validateArchitectureFromInput()
{
	return true;
}

/*****************************************************************************/
bool IBuildEnvironment::readArchitectureTripleFromCompiler()
{
	return true;
}

/*****************************************************************************/
bool IBuildEnvironment::compilerVersionIsToolchainVersion() const
{
	return true;
}

/*****************************************************************************/
std::string IBuildEnvironment::getVarsPath(const std::string& inUniqueId) const
{
	const auto id = identifier();
	const auto hostArch = static_cast<std::underlying_type_t<Arch::Cpu>>(m_state.info.hostArchitecture());
	const auto& archString = m_state.info.targetArchitectureTriple();
	// auto archString = m_state.inputs.getArchWithOptionsAsString(m_state.info.targetArchitectureTriple());
	const auto& uniqueId = inUniqueId.empty() ? m_state.inputs.toolchainPreferenceName() : inUniqueId;

	auto path = m_state.cache.getHashPath(fmt::format("{}_{}_{}_{}.env", id, hostArch, archString, uniqueId));
	if (m_refreshCache)
	{
		Files::removeIfExists(path);
	}
	return path;
}

/*****************************************************************************/
std::string IBuildEnvironment::getCachePath(const std::string& inId) const
{
	auto path = m_state.cache.getHashPath(fmt::format("{}_{}", this->identifier(), inId));
	if (m_refreshCache)
	{
		Files::removeIfExists(path);
	}
	return path;
}

/*****************************************************************************/
ToolchainType IBuildEnvironment::detectToolchainTypeFromPath(const std::string& inExecutable, BuildState& inState)
{
	if (inExecutable.empty())
		return ToolchainType::Unknown;

	auto executable = String::toLowerCase(inExecutable);

#if defined(CHALET_WIN32)
	if (String::endsWith("/cl.exe", executable))
		return ToolchainType::VisualStudio;
#endif

#if CHALET_ENABLE_INTEL_ICC
	#if defined(CHALET_WIN32)
	if (String::endsWith("/icl.exe", inExecutable))
	#else
	if (String::endsWith(StringList{ "/icpc", "/icc" }, inExecutable))
	#endif
		return ToolchainType::IntelClassic;
#endif

#if CHALET_ENABLE_INTEL_ICX
	if (
	#if defined(CHALET_WIN32)
		String::endsWith("/icx.exe", executable)
	#else
		String::endsWith("/icx", executable)
	#endif
		|| String::contains(StringList{ "onepi", "intel" }, executable))
		return ToolchainType::IntelLLVM;
#endif

	if (String::contains({ "emcc", "em++", "wasm32-clang" }, executable))
	{
		return ToolchainType::Emscripten;
	}

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

	// We may have a toolchain based on LLVM or GCC
	{
		auto defines = BuildEnvironmentGNU::getCompilerMacros(executable, inState);
		if (!defines.empty())
		{
			bool mingw = String::contains({ "__MINGW32__", "__MINGW64__" }, defines);
			if (String::contains("__clang__", defines))
			{
				inState.toolchain.setTreatAs(CustomToolchainTreatAs::LLVM);
				if (mingw)
					return ToolchainType::MingwLLVM;
				else
					return ToolchainType::LLVM;
			}
			else if (mingw)
			{
				inState.toolchain.setTreatAs(CustomToolchainTreatAs::GCC);
				return ToolchainType::MingwGNU;
			}
			else if (String::contains("__GNUC__", defines))
			{
				inState.toolchain.setTreatAs(CustomToolchainTreatAs::GCC);
				return ToolchainType::GNU;
			}
		}
	}

	return ToolchainType::Unknown;
}
}
