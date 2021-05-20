/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Toolchain/CompileToolchainApple.hpp"

// TODO: Find a nice way to separate out the clang/appleclang stuff from CompileToolchainGNU

namespace chalet
{
/*****************************************************************************/
CompileToolchainApple::CompileToolchainApple(const BuildState& inState, const ProjectConfiguration& inProject, const CompilerConfig& inConfig) :
	CompileToolchainLLVM(inState, inProject, inConfig)
{
}

/*****************************************************************************/
ToolchainType CompileToolchainApple::type() const
{
	return ToolchainType::Apple;
}

// ="-Wl,-flat_namespace,-undefined,suppress

/*****************************************************************************/
// Note: Noops mean a flag/feature isn't supported

/*****************************************************************************/
// Linking
/*****************************************************************************/
/*****************************************************************************/
void CompileToolchainApple::addProfileInformationLinkerOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void CompileToolchainApple::addLibStdCppLinkerOption(StringList& outArgList) const
{
	CompileToolchainLLVM::addLibStdCppLinkerOption(outArgList);

	// TODO: Apple has a "-stdlib=libstdc++" flag that is pre-C++11 for compatibility
}

/*****************************************************************************/
// Objective-C / Objective-C++
/*****************************************************************************/
/*****************************************************************************/
void CompileToolchainApple::addObjectiveCxxLink(StringList& outArgList) const
{
	// Unused in AppleClang
	UNUSED(outArgList);
}

/*****************************************************************************/
void CompileToolchainApple::addObjectiveCxxCompileOption(StringList& outArgList, const CxxSpecialization specialization) const
{
	const bool isObjCpp = specialization == CxxSpecialization::ObjectiveCpp;
	const bool isObjC = specialization == CxxSpecialization::ObjectiveC;
	const bool isObjCxx = specialization == CxxSpecialization::ObjectiveCpp || specialization == CxxSpecialization::ObjectiveC;
	if (m_project.objectiveCxx() && isObjCxx)
	{
		outArgList.push_back("-x");
		if (isObjCpp)
			outArgList.push_back("objective-c++");
		else if (isObjC)
			outArgList.push_back("objective-c");
	}
}

/*****************************************************************************/
void CompileToolchainApple::addObjectiveCxxRuntimeOption(StringList& outArgList, const CxxSpecialization specialization) const
{
	// Unused in AppleClang
	UNUSED(outArgList, specialization);
}

}
