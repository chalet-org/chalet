/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_ROUTE_TYPE_HPP
#define CHALET_ROUTE_TYPE_HPP

namespace chalet
{
enum class RouteType : ushort
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

#endif // CHALET_ROUTE_TYPE_HPP