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
	BuildRun,
	Build,
	Rebuild,
	Run,
	Bundle,
	Clean,
	// Profile,
	Install,
	Configure, // maybe redundant - Install could serve the same purpose
	Init,
#if defined(CHALET_DEBUG)
	Debug,
#endif
};
}

#endif // CHALET_ROUTE_HPP
