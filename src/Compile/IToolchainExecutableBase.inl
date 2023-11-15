/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/IToolchainExecutableBase.hpp"

// #include "Utility/Reflect.hpp"

namespace chalet
{
/*****************************************************************************/
template <typename T>
[[nodiscard]] Unique<T> IToolchainExecutableBase::makeTool(i32 result, const BuildState& inState, const SourceTarget& inProject)
{
	chalet_assert(result == 1 || result == 0, "makeTool expected result with 1 or 0");
	// LOG(Reflect::className<T>());

	if (result == 1)
		return std::make_unique<T>(inState, inProject);
	else // return == 0
		return nullptr;
}
}