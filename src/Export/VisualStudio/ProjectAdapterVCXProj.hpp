/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_PROJECT_ADAPTER_VCXPROJ_HPP
#define CHALET_PROJECT_ADAPTER_VCXPROJ_HPP

#include "Compile/CommandAdapter/CommandAdapterMSVC.hpp"

namespace chalet
{
class BuildState;
struct SourceTarget;

struct ProjectAdapterVCXProj
{
	ProjectAdapterVCXProj(const BuildState& inState, const SourceTarget& inProject);

	bool createPrecompiledHeaderSource();
	bool createWindowsResources();

	bool usesPrecompiledHeader() const;
	bool usesLibrarian() const;
	bool usesModules() const;

	std::string getBoolean(const bool inValue) const;
	std::string getBooleanIfTrue(const bool inValue) const;
	std::string getBooleanIfFalse(const bool inValue) const;

	std::string getBuildDir() const;
	std::string getObjectDir() const;
	std::string getIntermediateDir() const;
	std::string getEmbedManifest() const;
	const std::string& getTargetName() const noexcept;
	const std::string& workingDirectory() const noexcept;

	// General
	std::string getConfigurationType() const;
	std::string getUseDebugLibraries() const;
	std::string getPlatformToolset() const;
	std::string getWholeProgramOptimization() const;
	std::string getCharacterSet() const;

	std::string getFunctionLevelLinking() const;
	std::string getIntrinsicFunctions() const;
	std::string getSDLCheck() const;
	std::string getConformanceMode() const;
	std::string getWarningLevel() const;
	std::string getExternalWarningLevel() const;
	std::string getPreprocessorDefinitions() const;
	std::string getLanguageStandardCpp() const;
	std::string getLanguageStandardC() const;
	std::string getTreatWarningsAsError() const;
	std::string getDiagnosticsFormat() const;
	std::string getDebugInformationFormat() const;
	std::string getSupportJustMyCode() const;
	std::string getEnableAddressSanitizer() const;
	std::string getOptimization() const;
	std::string getInlineFunctionExpansion() const;
	std::string getFavorSizeOrSpeed() const;
	std::string getWholeProgramOptimizationCompileFlag() const;
	std::string getBufferSecurityCheck() const;
	std::string getFloatingPointModel() const;
	std::string getBasicRuntimeChecks() const;
	std::string getRuntimeLibrary() const;
	std::string getExceptionHandling() const;
	std::string getRunTimeTypeInfo() const;
	std::string getTreatWChartAsBuiltInType() const;
	std::string getForceConformanceInForLoopScope() const;
	std::string getRemoveUnreferencedCodeData() const;
	std::string getCallingConvention() const;
	std::string getPrecompiledHeaderFile() const;
	const std::string& getPrecompiledHeaderMinusLocation() const noexcept;
	const std::string& getPrecompiledHeaderSourceFile() const noexcept;
	std::string getPrecompiledHeaderOutputFile() const;
	std::string getPrecompiledHeaderObjectFile() const;
	std::string getProgramDataBaseFileName() const;
	std::string getAssemblerOutput() const;
	std::string getAssemblerListingLocation() const;
	std::string getAdditionalIncludeDirectories(const bool inAddCwd = false) const;
	std::string getAdditionalCompilerOptions() const;

	std::string getGenerateDebugInformation() const;
	std::string getIncrementalLinkDatabaseFile() const;
	std::string getFixedBaseAddress() const;
	std::string getAdditionalLibraryDirectories() const;
	std::string getAdditionalDependencies() const;
	std::string getTreatLinkerWarningAsErrors() const;
	std::string getLinkIncremental() const;
	std::string getSubSystem() const;
	std::string getEntryPoint() const;
	std::string getLinkTimeCodeGeneration() const;
	std::string getTargetMachine() const;
	std::string getEnableCOMDATFolding() const;
	std::string getOptimizeReferences() const;
	std::string getLinkerLinkTimeCodeGeneration() const;
	std::string getLinkTimeCodeGenerationObjectFile() const;
	std::string getImportLibrary() const;
	std::string getProgramDatabaseFile() const;
	std::string getStripPrivateSymbols() const;
	std::string getEntryPointSymbol() const;
	std::string getRandomizedBaseAddress() const;
	std::string getDataExecutionPrevention() const;
	std::string getProfile() const;
	std::string getAdditionalLinkerOptions() const;

	std::string getLocalDebuggerEnvironment() const;

private:
	const BuildState& m_state;
	const SourceTarget& m_project;

	CommandAdapterMSVC m_msvcAdapter;

	uint m_versionMajorMinor = 0;
	uint m_versionPatch = 0;
};
}

#endif // CHALET_PROJECT_ADAPTER_VCXPROJ_HPP
