/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Utility/Uuid.hpp"

#include <sstream>

#include "Libraries/LibUuid.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{
/*****************************************************************************/
Uuid Uuid::getNil()
{
	return Uuid();
}

/*****************************************************************************/
// Note: uuid_time_generator is considered experimental as of writing this
//
/*Uuid Uuid::v1Experimental()
{
	auto id = uuids::uuid_time_generator{}();
	return Uuid(uuids::to_string(id));
}*/

/*****************************************************************************/
Uuid Uuid::v4()
{
	auto id = uuids::uuid_system_generator{}();
	return Uuid(uuids::to_string(id));
}

/*****************************************************************************/
Uuid Uuid::v5(std::string_view inStr, std::string_view inNameSpace)
{
	uuids::uuid_name_generator gen(uuids::uuid::from_string(inNameSpace).value());
	auto id = gen(inStr);

	return Uuid(uuids::to_string(id));
}

/*****************************************************************************/
Uuid::Uuid() :
	m_str(uuids::to_string(uuids::uuid{}))
{
}

/*****************************************************************************/
Uuid::Uuid(std::string&& inStr) :
	m_str(std::move(inStr))
{
}

/*****************************************************************************/
bool Uuid::operator==(const Uuid& rhs) const
{
	return m_str == rhs.m_str;
}

/*****************************************************************************/
bool Uuid::operator!=(const Uuid& rhs) const
{
	return m_str != rhs.m_str;
}

/*****************************************************************************/
const std::string& Uuid::str() const noexcept
{
	return m_str;
}

/*****************************************************************************/
std::string Uuid::toUpperCase() const
{
	std::string ret(m_str);
	std::transform(ret.begin(), ret.end(), ret.begin(), [](uchar c) {
		return static_cast<uchar>(::toupper(static_cast<uchar>(c)));
	});
	return ret;
}
}
