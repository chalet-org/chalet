/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_CACHE_COMPILERS_HPP
#define CHALET_CACHE_COMPILERS_HPP

namespace chalet
{
struct CacheCompilers
{
	CacheCompilers() = default;

	const std::string& cpp() const noexcept;
	void setCpp(const std::string& inValue) noexcept;

	const std::string& cc() const noexcept;
	void setCc(const std::string& inValue) noexcept;

	const std::string& rc() const noexcept;
	void setRc(const std::string& inValue) noexcept;

private:
	std::string m_cpp;
	std::string m_cc;
	std::string m_rc;
};
}

#endif // CHALET_CACHE_COMPILERS_HPP
