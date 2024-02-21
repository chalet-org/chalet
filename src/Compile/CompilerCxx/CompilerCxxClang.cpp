/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CompilerCxx/CompilerCxxClang.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildState.hpp"
#include "State/Target/SourceTarget.hpp"
#include "System/Files.hpp"
#include "Utility/List.hpp"
#include "Utility/RegexPatterns.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CompilerCxxClang::CompilerCxxClang(const BuildState& inState, const SourceTarget& inProject) :
	CompilerCxxGCC(inState, inProject),
	m_clangAdapter(inState, inProject)
{
}

/*****************************************************************************/
std::string CompilerCxxClang::getPragmaId() const
{
	return std::string("clang");
}

/*****************************************************************************/
StringList CompilerCxxClang::getWarningExclusions() const
{
	return {
		"noexcept",
		"strict-null-sentinel"
	};
}

/*****************************************************************************/
StringList CompilerCxxClang::getModuleCommand(const std::string& inputFile, const std::string& outputFile, const std::string& dependency, const std::string& interfaceFile, const StringList& inModuleReferences, const StringList& inHeaderUnits, const ModuleFileType inType)
{
	UNUSED(inHeaderUnits);

	StringList ret;

	if (!addExecutable(ret))
		return ret;

	if (generateDependencies())
	{
		ret.emplace_back("-MT");
		ret.emplace_back(getQuotedPath(interfaceFile));
		ret.emplace_back("-MMD");
		ret.emplace_back("-MP");
		ret.emplace_back("-MF");
		ret.emplace_back(getQuotedPath(dependency));
	}

	constexpr auto derivative = SourceType::CPlusPlus;

	addSourceFileInterpretation(ret, inType);

	const bool moduleDependency = inType == ModuleFileType::ModuleDependency;
	const bool isSystemHeaderUnit = inType == ModuleFileType::SystemHeaderUnitObject;
	const bool isHeaderUnit = isSystemHeaderUnit || inType == ModuleFileType::HeaderUnitObject;
	const bool isModuleObject = inType == ModuleFileType::ModuleObject;
	const bool moduleImplementationUnit = inType == ModuleFileType::ModuleImplementationUnit;
	if (moduleDependency || isHeaderUnit)
		ret.emplace_back("--precompile");

	if (isSystemHeaderUnit)
		ret.emplace_back("-fmodule-header=system");

	if (moduleDependency || isModuleObject || moduleImplementationUnit)
	{
		if (moduleDependency)
			ret.emplace_back("-fmodule-output");

		for (const auto& item : inModuleReferences)
		{
			// Note: can't quote these paths
			ret.emplace_back(fmt::format("-fmodule-file={}", item));
		}

		for (const auto& item : inHeaderUnits)
		{
			ret.emplace_back(fmt::format("-fmodule-file={}", getQuotedPath(item)));
		}

		if (isModuleObject)
			ret.emplace_back(fmt::format("-fmodule-output={}", getQuotedPath(interfaceFile)));
	}

	addOptimizations(ret);
	addLanguageStandard(ret, derivative);
	addCppCoroutines(ret);
	addCppConcepts(ret);
	addWarnings(ret);

	if (isSystemHeaderUnit)
	{
		// TODO: Found this in emscripten. maybe remove this in the future
		List::addIfDoesNotExist(ret, "-Wno-pragma-system-header-outside-header");
	}

	addCharsets(ret);
	addLibStdCppCompileOption(ret, derivative);
	addPositionIndependentCodeOption(ret);
	addCompileOptions(ret);
	addObjectiveCxxRuntimeOption(ret, derivative);
	addDiagnosticColorOption(ret);
	addFastMathOption(ret);
	addNoRunTimeTypeInformationOption(ret);
	addNoExceptionsOption(ret);
	addThreadModelCompileOption(ret);
	addArchitecture(ret, std::string());
	addSystemRootOption(ret);
	addLinkTimeOptimizations(ret);

	addDebuggingInformationOption(ret);
	addProfileInformation(ret);
	addSanitizerOptions(ret);

	addDefines(ret);

	// Before other includes
	addPchInclude(ret, derivative);

	addIncludes(ret);
	addSystemIncludes(ret);

	ret.emplace_back("-o");

	if (moduleDependency || isHeaderUnit)
		ret.emplace_back(getQuotedPath(interfaceFile));
	else
		ret.emplace_back(getQuotedPath(outputFile));

	ret.emplace_back("-c");
	// if (inType == ModuleFileType::SystemHeaderUnitObject)
	// 	ret.emplace_back(getQuotedPath(String::getPathFilename(inputFile)));
	// else
	ret.emplace_back(getQuotedPath(inputFile));

	return ret;
}

/*****************************************************************************/
void CompilerCxxClang::addSourceFileInterpretation(StringList& outArgList, const ModuleFileType moduleType) const
{
	outArgList.emplace_back("-x");

	if (moduleType == ModuleFileType::SystemHeaderUnitObject)
		outArgList.emplace_back("c++-system-header");
	else if (moduleType == ModuleFileType::HeaderUnitObject)
		outArgList.emplace_back("c++-header");
	else if (moduleType == ModuleFileType::ModuleImplementationUnit)
		outArgList.emplace_back("c++");
	else
		outArgList.emplace_back("c++-module");
}

/*****************************************************************************/
void CompilerCxxClang::addWarnings(StringList& outArgList) const
{
	CompilerCxxGCC::addWarnings(outArgList);
}

/*****************************************************************************/
void CompilerCxxClang::addProfileInformation(StringList& outArgList) const
{
	CompilerCxxGCC::addProfileInformation(outArgList);
}

/*****************************************************************************/
void CompilerCxxClang::addSanitizerOptions(StringList& outArgList, const BuildState& inState)
{
	SourceTarget dummyTarget(inState);
	CommandAdapterClang clangAdapter(inState, dummyTarget);
	StringList sanitizers = clangAdapter.getSanitizersList();
	if (!sanitizers.empty())
	{
		auto list = String::join(sanitizers, ',');
		outArgList.emplace_back(fmt::format("-fsanitize={}", list));
	}
}

/*****************************************************************************/
void CompilerCxxClang::addSanitizerOptions(StringList& outArgList) const
{
	if (m_state.configuration.enableSanitizers())
	{
		CompilerCxxClang::addSanitizerOptions(outArgList, m_state);
	}
}

/*****************************************************************************/
void CompilerCxxClang::addLanguageStandard(StringList& outArgList, const SourceType derivative) const
{
	const CodeLanguage language = m_project.language();
	bool validPchType = derivative == SourceType::CxxPrecompiledHeader && (language == CodeLanguage::C || language == CodeLanguage::ObjectiveC);
	bool useC = validPchType || derivative == SourceType::C || derivative == SourceType::ObjectiveC;

	auto standard = useC ? m_clangAdapter.getLanguageStandardC() : m_clangAdapter.getLanguageStandardCpp();
	if (!standard.empty())
	{
		outArgList.emplace_back(fmt::format("-std={}", standard));
	}
}

/*****************************************************************************/
void CompilerCxxClang::addDiagnosticColorOption(StringList& outArgList) const
{
	std::string diagnosticColor{ "-fcolor-diagnostics" };
	if (isFlagSupported(diagnosticColor))
		List::addIfDoesNotExist(outArgList, std::move(diagnosticColor));
}

/*****************************************************************************/
void CompilerCxxClang::addLibStdCppCompileOption(StringList& outArgList, const SourceType derivative) const
{
	UNUSED(outArgList, derivative);
}

/*****************************************************************************/
void CompilerCxxClang::addPositionIndependentCodeOption(StringList& outArgList) const
{
	CompilerCxxGCC::addPositionIndependentCodeOption(outArgList);
}

/*****************************************************************************/
void CompilerCxxClang::addThreadModelCompileOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
bool CompilerCxxClang::addArchitecture(StringList& outArgList, const std::string& inArch) const
{
	return CompilerCxxClang::addArchitectureToCommand(outArgList, inArch, m_state);
}

/*****************************************************************************/
bool CompilerCxxClang::addArchitectureToCommand(StringList& outArgList, const std::string& inArch, const BuildState& inState)
{
	// https://clang.llvm.org/docs/CrossCompilation.html

	// clang -print-supported-cpus

	UNUSED(inArch);

	const auto& targetArchString = inState.info.targetArchitectureTriple();

	outArgList.emplace_back("-target");
	outArgList.push_back(targetArchString);

	const auto& archOptions = inState.inputs.archOptions();
	if (archOptions.size() == 3) // <cpu-name>,<fpu-name>,<fabi>
	{
		outArgList.emplace_back(fmt::format("-mcpu={}", archOptions[0]));
		outArgList.emplace_back(fmt::format("-mfpu={}", archOptions[1]));
		outArgList.emplace_back(fmt::format("-mfloat-abi={}", archOptions[2]));
	}

	return true;
}

/*****************************************************************************/
void CompilerCxxClang::addLinkTimeOptimizations(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void CompilerCxxClang::addCppCoroutines(StringList& outArgList) const
{
	if (m_clangAdapter.supportsCppCoroutines())
	{
		std::string option{ "-fcoroutines-ts" };
		// if (isFlagSupported(option))
		List::addIfDoesNotExist(outArgList, std::move(option));
	}
}

/*****************************************************************************/
void CompilerCxxClang::addCppConcepts(StringList& outArgList) const
{
	if (m_clangAdapter.supportsCppConcepts())
	{
		std::string option{ "-fconcepts-ts" };
		// if (isFlagSupported(option))
		List::addIfDoesNotExist(outArgList, std::move(option));
	}
}

}
