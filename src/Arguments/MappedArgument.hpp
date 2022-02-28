/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_MAPPED_ARGUMENT_HPP
#define CHALET_MAPPED_ARGUMENT_HPP

#include "Arguments/ArgumentIdentifier.hpp"
#include "Utility/Variant.hpp"

namespace chalet
{
struct MappedArgument
{
	ArgumentIdentifier id;
	Variant value;

	MappedArgument(ArgumentIdentifier inId, Variant inValue);
};
}

#endif // CHALET_MAPPED_ARGUMENT_HPP
