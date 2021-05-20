/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Toolchain/ICompileToolchain.hpp"

namespace chalet
{
/*****************************************************************************/
bool ICompileToolchain::preBuild()
{
	return true;
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
void ICompileToolchain::addThreadModelCompileOption(StringList& outArgList) const
{
	UNUSED(outArgList);
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
