/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/CompilerInfo.hpp"
#include "Compile/CompilerPathStructure.hpp"
#include "Compile/ToolchainType.hpp"

namespace chalet
{
class BuildState;

struct IBuildEnvironment;
using BuildEnvironment = Unique<IBuildEnvironment>;

struct IBuildEnvironment
{
	virtual ~IBuildEnvironment() = default;

	const std::string& identifier() const noexcept;
	ToolchainType type() const noexcept;

	bool isWindowsTarget() const noexcept;
	bool isEmbeddedTarget() const noexcept;

	bool isWindowsClang() const noexcept;
	bool isMsvcClang() const noexcept;
	bool isClang() const noexcept;
	bool isAppleClang() const noexcept;
	bool isGcc() const noexcept;
	bool isIntelClassic() const noexcept;
	bool isMingw() const noexcept;
	bool isMingwGcc() const noexcept;
	bool isMingwClang() const noexcept;
	bool isMsvc() const noexcept;
	bool isEmscripten() const noexcept;

	const std::string& detectedVersion() const;
	std::string getMajorVersion() const;
	bool isCompilerFlagSupported(const std::string& inFlag) const;

	virtual std::string getExecutableExtension() const;
	virtual std::string getLibraryPrefix(const bool inMingwUnix) const final;
	virtual std::string getSharedLibraryExtension() const;
	virtual std::string getArchiveExtension() const = 0;
	virtual std::string getPrecompiledHeaderExtension() const = 0;

	virtual std::string getCompilerAliasForVisualStudio() const = 0;

	virtual std::string getObjectFile(const std::string& inSource) const;
	virtual std::string getAssemblyFile(const std::string& inSource) const;
	virtual std::string getWindowsResourceObjectFile(const std::string& inSource) const;
	virtual std::string getDependencyFile(const std::string& inSource) const;
	virtual std::string getModuleDirectivesDependencyFile(const std::string& inSource) const;
	virtual std::string getModuleBinaryInterfaceFile(const std::string& inSource) const;
	virtual std::string getModuleBinaryInterfaceDependencyFile(const std::string& inSource) const;

	const std::string& sysroot() const noexcept;
	const std::string& targetSystemVersion() const noexcept;
	const StringList& targetSystemPaths() const noexcept;

	const std::string& commandInvoker() const;

protected:
	friend class BuildState;
	friend struct CompilerTools;
	friend struct BuildPaths;

	explicit IBuildEnvironment(const ToolchainType inType, BuildState& inState);

	[[nodiscard]] static Unique<IBuildEnvironment> make(ToolchainType type, BuildState& inState);
	static ToolchainType detectToolchainTypeFromPath(const std::string& inExecutable, BuildState& inState);

	virtual StringList getVersionCommand(const std::string& inExecutable) const = 0;
	virtual std::string getFullCxxCompilerString(const std::string& inPath, const std::string& inVersion) const = 0;
	virtual bool verifyToolchain() = 0;
	virtual bool supportsFlagFile() = 0;
	virtual bool getCompilerVersionAndDescription(CompilerInfo& outInfo) = 0;
	virtual std::vector<CompilerPathStructure> getValidCompilerPaths() const = 0;

	virtual void generateTargetSystemPaths();
	virtual bool readArchitectureTripleFromCompiler();
	virtual bool compilerVersionIsToolchainVersion() const;
	virtual bool createFromVersion(const std::string& inVersion);
	virtual bool validateArchitectureFromInput();
	virtual bool populateSupportedFlags(const std::string& inExecutable);

	bool create(const std::string& inVersion = std::string());
	bool getCompilerPaths(CompilerInfo& outInfo) const;
	bool getCompilerInfoFromExecutable(CompilerInfo& outInfo);
	bool makeSupportedCompilerFlags(const std::string& inExecutable);

	void getDataWithCache(std::string& outArch, const std::string& inId, const std::string& inCompilerPath, const std::function<std::string()>& onGet);

	std::string getVarsPath(const std::string& inUniqueId = std::string()) const;
	std::string getCachePath(const std::string& inId) const;

	BuildState& m_state;

	std::string m_sysroot;
	std::string m_targetSystemVersion;
	StringList m_targetSystemPaths;

	Dictionary<bool> m_supportedFlags;

	std::string m_detectedVersion;
	std::string m_commandInvoker;

	mutable ToolchainType m_type;

	bool m_isWindowsTarget = false;
	bool m_isEmbeddedTarget = false;

private:
	std::string m_identifier;

	bool m_initialized = false;
};
}
