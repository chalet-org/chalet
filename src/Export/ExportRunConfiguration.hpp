/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
struct ExportRunConfiguration
{
	std::string name;
	std::string config;
	std::string arch;
	std::string outputFile;
	StringList args;
	std::map<std::string, std::string> env;
};

using ExportRunConfigurationList = std::vector<ExportRunConfiguration>;
}
