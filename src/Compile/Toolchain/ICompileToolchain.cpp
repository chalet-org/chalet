/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Toolchain/ICompileToolchain.hpp"

namespace chalet
{
/*****************************************************************************/
void ICompileToolchain::addIncludes(StringList& inArgList)
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addWarnings(StringList& inArgList)
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addDefines(StringList& inArgList)
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addPchInclude(StringList& inArgList)
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addOptimizationOption(StringList& inArgList)
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addLanguageStandard(StringList& inArgList, const CxxSpecialization specialization)
{
	UNUSED(inArgList, specialization);
}

/*****************************************************************************/
void ICompileToolchain::addDebuggingInformationOption(StringList& inArgList)
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addProfileInformationCompileOption(StringList& inArgList)
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addCompileOptions(StringList& inArgList)
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addDiagnosticColorOption(StringList& inArgList)
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addLibStdCppCompileOption(StringList& inArgList, const CxxSpecialization specialization)
{
	UNUSED(inArgList, specialization);
}

/*****************************************************************************/
void ICompileToolchain::addPositionIndependentCodeOption(StringList& inArgList)
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addNoRunTimeTypeInformationOption(StringList& inArgList)
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addThreadModelCompileOption(StringList& inArgList)
{
	UNUSED(inArgList);
}

/*****************************************************************************/

// Linking
void ICompileToolchain::addLibDirs(StringList& inArgList)
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addLinks(StringList& inArgList)
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addRunPath(StringList& inArgList)
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addStripSymbolsOption(StringList& inArgList)
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addLinkerOptions(StringList& inArgList)
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addProfileInformationLinkerOption(StringList& inArgList)
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addLinkTimeOptimizationOption(StringList& inArgList)
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addThreadModelLinkerOption(StringList& inArgList)
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addLinkerScripts(StringList& inArgList)
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addLibStdCppLinkerOption(StringList& inArgList)
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addStaticCompilerLibraryOptions(StringList& inArgList)
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void ICompileToolchain::addPlatformGuiApplicationFlag(StringList& inArgList)
{
	UNUSED(inArgList);
}
}
