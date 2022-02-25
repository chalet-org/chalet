/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_MODULE_FILE_TYPE_HPP
#define CHALET_MODULE_FILE_TYPE_HPP

namespace chalet
{
enum class ModuleFileType : ushort
{
	ModuleDependency,
	HeaderUnitDependency,
	HeaderUnitObject,
	ModuleObject,
	ModuleImplementationUnit,
};
}

#endif // CHALET_MODULE_FILE_TYPE_HPP
