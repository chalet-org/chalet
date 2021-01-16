/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "CacheJson/CacheCompilers.hpp"

namespace chalet
{
/*****************************************************************************/
const std::string& CacheCompilers::cpp() const noexcept
{
	return m_cpp;
}
void CacheCompilers::setCpp(const std::string& inValue) noexcept
{
	m_cpp = inValue;
}

/*****************************************************************************/
const std::string& CacheCompilers::cc() const noexcept
{
	return m_cc;
}
void CacheCompilers::setCc(const std::string& inValue) noexcept
{
	m_cc = inValue;
}

/*****************************************************************************/
const std::string& CacheCompilers::rc() const noexcept
{
	return m_rc;
}
void CacheCompilers::setRc(const std::string& inValue) noexcept
{
	m_rc = inValue;
}

}
