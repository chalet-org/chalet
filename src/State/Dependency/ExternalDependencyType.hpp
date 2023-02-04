/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_EXTERNAL_DEPENDENCY_TYPE_HPP
#define CHALET_EXTERNAL_DEPENDENCY_TYPE_HPP

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

#endif // CHALET_EXTERNAL_DEPENDENCY_TYPE_HPP
