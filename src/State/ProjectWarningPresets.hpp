/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_PROJECT_WARNING_PRESETS_HPP
#define CHALET_PROJECT_WARNING_PRESETS_HPP

namespace chalet
{
enum class ProjectWarningPresets : ushort
{
	None,
	Custom,
	Minimal,
	Extra,
	Pedantic,
	Strict,
	StrictPedantic,
	VeryStrict
};
}

#endif // CHALET_PROJECT_WARNING_PRESETS_HPP
