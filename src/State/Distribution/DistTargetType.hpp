/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_DIST_TARGET_TYPE_HPP
#define CHALET_DIST_TARGET_TYPE_HPP

namespace chalet
{
enum class DistTargetType
{
	Unknown,
	DistributionBundle,
	BundleArchive,
	MacosDiskImage,
	Script,
	Process,
	Validation,
};
}

#endif // CHALET_DIST_TARGET_TYPE_HPP
