/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "ChaletJson/ChaletJsonParser.hpp"
#include "Libraries/Json.hpp"
#include "State/BuildState.hpp"

namespace chalet
{
struct BuildFileChecker
{
	BuildFileChecker(BuildState& inState);

	bool run();

private:
	Json getExpandedBuildFile();

	bool checkNode(const Json& inNode, Json& outJson, const std::string& inLastKey = std::string());

	bool checkNodeWithExternalDependency(Json& node, const IExternalDependency* inTarget);

	template <typename T>
	bool checkNodeWithTargetPtr(Json& node, const T* inTarget);

	BuildState& m_state;

	ChaletJsonParser m_parser;
};
}

#include "Check/BuildFileChecker.inl"
