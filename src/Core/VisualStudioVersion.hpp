/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_VISUAL_STUDIO_VERSION_HPP
#define CHALET_VISUAL_STUDIO_VERSION_HPP

namespace chalet
{
enum class VisualStudioVersion : ushort
{
	None,
	Stable = 1,
	Preview = 2,
	VisualStudio2010 = 10,
	VisualStudio2012 = 11,
	VisualStudio2013 = 12,
	VisualStudio2015 = 14,
	VisualStudio2017 = 15,
	VisualStudio2019 = 16,
	VisualStudio2022 = 17,
};
}

#endif // CHALET_VISUAL_STUDIO_VERSION_HPP
