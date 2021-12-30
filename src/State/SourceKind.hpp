/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SOURCE_KIND_HPP
#define CHALET_SOURCE_KIND_HPP

namespace chalet
{
enum class SourceKind : ushort
{
	None,
	StaticLibrary,
	SharedLibrary,
	Executable
};
}

#endif // CHALET_SOURCE_KIND_HPP
