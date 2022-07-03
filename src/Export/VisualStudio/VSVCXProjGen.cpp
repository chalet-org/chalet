/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
 */

#include "Export/VisualStudio/VSVCXProjGen.hpp"

#include "Compile/Environment/ICompileEnvironment.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildState.hpp"
#include "State/TargetMetadata.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/String.hpp"
#include "Xml/XmlFile.hpp"

namespace chalet
{
/*****************************************************************************/
VSVCXProjGen::VSVCXProjGen(const std::vector<Unique<BuildState>>& inStates, const std::string& inCwd) :
	m_states(inStates),
	m_cwd(inCwd)
{
	UNUSED(m_cwd);
}

/*****************************************************************************/
bool VSVCXProjGen::saveToFile(const std::string& inTargetName)
{
	if (m_states.empty())
		return false;

	if (!saveFiltersFile(fmt::format("{name}/{name}.vcxproj.filters", fmt::arg("name", inTargetName))))
		return false;

	if (!saveUserFile(fmt::format("{name}/{name}.vcxproj.user", fmt::arg("name", inTargetName))))
		return false;

	return true;
}

/*****************************************************************************/
bool VSVCXProjGen::saveFiltersFile(const std::string& inFilename)
{
	XmlFile xmlFile(inFilename);

	auto& xml = xmlFile.xml;
	auto& xmlRoot = xml.root();
	xmlRoot.setName("Project");
	xmlRoot.addAttribute("ToolsVersion", "4.0");
	xmlRoot.addAttribute("xmlns", "http://schemas.microsoft.com/developer/msbuild/2003");
	xmlRoot.addChildNode("ItemGroup", [this](XmlNode& node) {
		node.addChildNode("Filter", [this](XmlNode& node2) {
			node2.addAttribute("Include", "Source Files");
			node2.addChildNode("UniqueIdentifier", "{4FC737F1-C7A5-4376-A066-2A32D752A2FF}");
			node2.addChildNode("Extensions", "cpp;c;cc;cxx;c++;cppm;ixx;def;odl;idl;hpj;bat;asm;asmx");
		});
		node.addChildNode("Filter", [this](XmlNode& node2) {
			node2.addAttribute("Include", "Header Files");
			node2.addChildNode("UniqueIdentifier", "{93995380-89BD-4b04-88EB-625FBE52EBFB}");
			node2.addChildNode("Extensions", "h;hh;hpp;hxx;h++;hm;inl;inc;ipp;xsd");
		});
		node.addChildNode("Filter", [this](XmlNode& node2) {
			node2.addAttribute("Include", "Resource Files");
			node2.addChildNode("UniqueIdentifier", "{67DA6AB6-F800-4c08-8B7A-83BB121AAD01}");
			node2.addChildNode("Extensions", "rc;ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe;resx;tiff;tif;png;wav;mfcribbon-ms");
		});
	});

	xmlFile.save();

	return true;
}

/*****************************************************************************/
bool VSVCXProjGen::saveUserFile(const std::string& inFilename)
{
	XmlFile xmlFile(inFilename);

	auto& xml = xmlFile.xml;
	auto& xmlRoot = xml.root();
	xmlRoot.setName("Project");
	xmlRoot.addAttribute("ToolsVersion", "Current");
	xmlRoot.addAttribute("xmlns", "http://schemas.microsoft.com/developer/msbuild/2003");
	xmlRoot.addChildNode("PropertyGroup", [](XmlNode& node) {
		UNUSED(node);
	});

	xmlFile.save();

	return true;
}
}
