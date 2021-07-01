/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_ROUTE_HPP
#define CHALET_ROUTE_HPP

namespace chalet
{
enum class Route : ushort
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
	SettingsGet,
	SettingsSet,
	SettingsUnset,
#if defined(CHALET_DEBUG)
	Debug,
#endif
	//
	Count,
};
}

#endif // CHALET_ROUTE_HPP
