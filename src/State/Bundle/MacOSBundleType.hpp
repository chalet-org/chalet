/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_MACOS_BUNDLE_TYPE_HPP
#define CHALET_MACOS_BUNDLE_TYPE_HPP

namespace chalet
{
enum class MacOSBundleType : ushort
{
	None,
	Application,
	Framework,
	Plugin,
	KernelExtension,
};
}

#endif // CHALET_MACOS_BUNDLE_TYPE_HPP
