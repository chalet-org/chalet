/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_PROJECT_WARNINGS_HPP
#define CHALET_PROJECT_WARNINGS_HPP

namespace chalet
{
enum class ProjectWarnings : ushort
{
	None,
	Minimal,
	Extra,
	Error,
	Pedantic,
	Strict,
	StrictPedantic,
	VeryStrict,
	//
	Custom
};
}

#endif // CHALET_PROJECT_WARNINGS_HPP
