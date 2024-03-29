/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
enum class ModuleFileType : u16
{
	ModuleDependency,
	HeaderUnitDependency,
	HeaderUnitObject,
	SystemHeaderUnitObject,
	ModuleObject,			  // Module compilations
	ModuleImplementationUnit, // Aka, the file with "main"
};
}
