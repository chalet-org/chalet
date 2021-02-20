/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_PROJECT_KIND_HPP
#define CHALET_PROJECT_KIND_HPP

namespace chalet
{
enum class ProjectKind : ushort
{
	None,
	StaticLibrary,
	SharedLibrary,
	ConsoleApplication,
	DesktopApplication
};
}

#endif // CHALET_PROJECT_KIND_HPP
