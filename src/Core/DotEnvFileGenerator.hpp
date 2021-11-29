/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_DOT_ENV_FILE_GENERATOR_HPP
#define CHALET_DOT_ENV_FILE_GENERATOR_HPP

namespace chalet
{
struct DotEnvFileGenerator
{
	explicit DotEnvFileGenerator(const std::string& inFilename);

	const std::string& filename() const noexcept;

	void set(const std::string& inKey, const std::string& inValue);
	bool save();

private:
	std::string m_filename;

	Dictionary<std::string> m_variables;
};
}

#endif // CHALET_DOT_ENV_FILE_GENERATOR_HPP
