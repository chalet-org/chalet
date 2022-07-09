/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_GUID_HPP
#define CHALET_GUID_HPP

namespace chalet
{
struct Uuid
{
	static Uuid getNil();
	static Uuid v4();
	static Uuid v5(std::string_view inStr, std::string_view inNameSpace);

	bool operator==(const Uuid& rhs) const;
	bool operator!=(const Uuid& rhs) const;

	const std::string& str() const noexcept;
	std::string toUpperCase() const;

private:
	Uuid();
	Uuid(std::string&& inStr);

	std::string m_str;
};
}

#endif // CHALET_GUID_HPP
