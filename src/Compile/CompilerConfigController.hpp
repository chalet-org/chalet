/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILER_CONFIG_CONTROLLER_HPP
#define CHALET_COMPILER_CONFIG_CONTROLLER_HPP

#include "Compile/CodeLanguage.hpp"

namespace chalet
{
struct CompilerConfig;
class BuildState;

struct CompilerConfigController
{
	CompilerConfig& get(const CodeLanguage inLanguage);
	const CompilerConfig& get(const CodeLanguage inLanguage) const;

private:
	friend class BuildState;

	void makeConfigForLanguage(const CodeLanguage inLanguage, const BuildState& inState);
	bool initialize();

	std::unordered_map<CodeLanguage, Unique<CompilerConfig>> m_configs;
};
}

#endif // CHALET_COMPILER_CONFIG_CONTROLLER_HPP
