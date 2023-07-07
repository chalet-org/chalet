/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_JSON_VALUES_HPP
#define CHALET_JSON_VALUES_HPP

#define CH_STR(x) static constexpr const char x[]

namespace chalet
{
namespace Values
{
CH_STR(Auto) = "auto";
CH_STR(All) = "all";
}
}

#undef CH_STR

#endif // CHALET_JSON_VALUES_HPP
