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
	/**
		@brief Get the nil UUID string (all zeroes)
		@return Uuid
	 */
	static Uuid getNil();

	// static Uuid v1Experimental();

	/**
		@brief Create a version 4 UUID (random), using operating system apis
		@return Uuid
	 */
	static Uuid v4();

	/**
		@brief Create a version 5 UUID (namespace w/ SHA-1)
		@param inStr
		@param inNameSpace
		@return Uuid
	 */
	static Uuid v5(std::string_view inStr, std::string_view inNameSpace);

	Uuid();

	bool operator==(const Uuid& rhs) const;
	bool operator!=(const Uuid& rhs) const;

	const std::string& str() const noexcept;
	std::string toUpperCase() const;

	std::string toAppleHash() const;

private:
	Uuid(std::string&& inStr);

	std::string m_str;
};
}

#endif // CHALET_GUID_HPP
