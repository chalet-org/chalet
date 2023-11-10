/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
// Source control + packages
enum class ExternalDependencyType : ushort
{
	Git,
	Local,
	// SVN,
	Script,
};
}
