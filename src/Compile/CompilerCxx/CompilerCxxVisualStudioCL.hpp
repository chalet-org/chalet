/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILER_EXECUTABLE_CXX_VISUAL_STUDIO_CL_HPP
#define CHALET_COMPILER_EXECUTABLE_CXX_VISUAL_STUDIO_CL_HPP

#include "Compile/CompilerCxx/ICompilerCxx.hpp"

namespace chalet
{
struct CompilerCxxVisualStudioCL : public ICompilerCxx
{
	explicit CompilerCxxVisualStudioCL(const BuildState& inState, const SourceTarget& inProject);

	virtual bool initialize() override;

	virtual StringList getPrecompiledHeaderCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency, const std::string& arch) override;
	virtual StringList getCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency, const CxxSpecialization specialization) override;

protected:
	virtual void addIncludes(StringList& outArgList) const override;
	virtual void addWarnings(StringList& outArgList) const override;
	virtual void addDefines(StringList& outArgList) const override;
	virtual void addPchInclude(StringList& outArgList) const override;
	virtual void addOptimizationOption(StringList& outArgList) const override;
	virtual void addLanguageStandard(StringList& outArgList, const CxxSpecialization specialization) const override;
	virtual void addDebuggingInformationOption(StringList& outArgList) const override;
	virtual void addCompileOptions(StringList& outArgList) const override;
	virtual void addNoRunTimeTypeInformationOption(StringList& outArgList) const override;
	virtual void addNoExceptionsOption(StringList& outArgList) const override;
	virtual void addThreadModelCompileOption(StringList& outArgList) const override;

	//
	virtual void addDiagnosticsOption(StringList& outArgList) const;
	virtual void addWholeProgramOptimization(StringList& outArgList) const;

	bool createPrecompiledHeaderSource();

private:
	virtual void addUnsortedOptions(StringList& outArgList) const final;

	std::string m_pchSource;
	std::string m_pchMinusLocation;
};
}

#endif // CHALET_COMPILER_EXECUTABLE_CXX_VISUAL_STUDIO_CL_HPP
