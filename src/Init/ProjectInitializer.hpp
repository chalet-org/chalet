/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_PROJECT_INITIALIZER_HPP
#define CHALET_PROJECT_INITIALIZER_HPP

#include "State/CommandLineInputs.hpp"

namespace chalet
{
class ProjectInitializer
{
public:
	explicit ProjectInitializer(const CommandLineInputs& inInputs);

	bool run();

private:
	bool makeBuildJson();
	bool makeMainCpp();
	bool makePch();
	bool makeGitIgnore();
	bool makeReadme();

	const CommandLineInputs& m_inputs;

	std::string m_rootPath;
};
}

#endif // CHALET_PROJECT_INITIALIZER_HPP
