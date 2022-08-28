/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SCRIPT_TYPE_HPP
#define CHALET_SCRIPT_TYPE_HPP

namespace chalet
{
enum class ScriptType
{
	None,
	UnixShell,
	Python,
	Ruby,
	Perl,
	Lua,
	Tcl,
	Awk,
	Powershell,
	WindowsCommand,
};
}

#endif // CHALET_SCRIPT_TYPE_HPP
