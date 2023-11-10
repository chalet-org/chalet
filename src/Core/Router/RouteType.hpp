/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
enum class RouteType : u16
{
	Unknown,
	Help,
	BuildRun,
	Build,
	Rebuild,
	Run,
	Bundle,
	Clean,
	Configure,
	Init,
	Export,
	SettingsGet,
	SettingsGetKeys,
	SettingsSet,
	SettingsUnset,
	Validate,
	Query,
	TerminalTest,
#if defined(CHALET_DEBUG)
	Debug,
#endif
	//
	Count,
};
}
