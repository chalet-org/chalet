/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SUBPROCESS_TYPES_HPP
#define CHALET_SUBPROCESS_TYPES_HPP

namespace chalet
{
// maps to subprocess::PipeOption (without exposing the api)
enum class PipeOption : int
{
	Inherit, ///< Inherits current process handle
	StdOut,	 ///< Redirects to stdout
	StdErr,	 ///< redirects to stderr
	/** Redirects to provided pipe. You can open /dev/null. Pipe handle
            that you specify will be made inheritable and closed automatically.
        */
	Specific,
	Pipe, ///< Redirects to a new handle created for you.
	Close ///< Troll the child by providing a closed pipe.
};
}

#endif // CHALET_SUBPROCESS_TYPES_HPP
