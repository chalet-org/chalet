#include "State/SourceFileGroup.hpp"

namespace chalet
{
/*****************************************************************************/
CxxSpecialization SourceFileGroup::getSpecialization() const
{
	switch (this->type)
	{
		case SourceType::C:
			return CxxSpecialization::C;
		case SourceType::ObjectiveC:
			return CxxSpecialization::ObjectiveC;
		case SourceType::ObjectiveCPlusPlus:
			return CxxSpecialization::ObjectiveCPlusPlus;
		default:
			return CxxSpecialization::CPlusPlus;
	}
}
}
