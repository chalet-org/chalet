/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Arguments/MappedArgument.hpp"

namespace chalet
{
/*****************************************************************************/
MappedArgument::MappedArgument(ArgumentIdentifier inId, Variant inValue) :
	id(inId),
	value(std::move(inValue))
{
}

}
