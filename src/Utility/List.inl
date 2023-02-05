/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Utility/List.hpp"

namespace chalet
{
/*****************************************************************************/
template <typename VectorType, class T>
void List::forEach(std::vector<VectorType>& inList, T* inst, void (T::*func)(VectorType&&))
{
	for (auto&& item : inList)
	{
		std::invoke(func, inst, std::move(item));
	}
}

/*****************************************************************************/
template <typename VectorType>
bool List::addIfDoesNotExist(std::vector<VectorType>& outList, VectorType&& inValue)
{
	if (std::find(outList.begin(), outList.end(), inValue) == outList.end())
	{
		outList.emplace_back(std::forward<VectorType>(inValue));
		return true;
	}

	return false;
}

/*****************************************************************************/
template <typename VectorType>
void List::removeIfExists(std::vector<VectorType>& outList, VectorType&& inValue)
{
	outList.erase(std::remove(outList.begin(), outList.end(), inValue), outList.end());
}

/*****************************************************************************/
template <typename VectorType>
bool List::contains(const std::vector<VectorType>& inList, const VectorType& inValue)
{
	return std::find(inList.begin(), inList.end(), inValue) != inList.end();
}

namespace priv
{
/*****************************************************************************/
template <typename T>
void addArg(StringList& outList, const T& inArg)
{
	using Type = std::decay_t<T>;
	if constexpr (std::is_same_v<StringList, Type>)
	{
		for (const auto& item : inArg)
		{
			if (item.empty())
				continue;

			if (std::find(outList.begin(), outList.end(), item) == outList.end())
				outList.emplace_back(item);
		}
	}
	else
	{
		static_assert((std::is_constructible_v<std::string, const T&>));
		if (inArg.empty())
			return;

		std::string value(inArg);
		if (std::find(outList.begin(), outList.end(), value) == outList.end())
			outList.emplace_back(std::move(value));
	}
}
}

template <typename... Args>
StringList List::combine(Args&&... args)
{
	StringList ret;

	((priv::addArg<Args>(ret, std::forward<Args>(args))), ...);

	return ret;
}
}
