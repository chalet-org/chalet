/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#define CHALET_DISALLOW_COPY_MOVE(ClassName)         \
	ClassName(const ClassName&) = delete;            \
	ClassName(ClassName&&) noexcept = delete;        \
	ClassName& operator=(const ClassName&) = delete; \
	ClassName& operator=(ClassName&&) noexcept = delete

#define CHALET_DISALLOW_COPY(ClassName)   \
	ClassName(const ClassName&) = delete; \
	ClassName& operator=(const ClassName&) = delete

#define CHALET_DISALLOW_MOVE(ClassName)       \
	ClassName(ClassName&&) noexcept = delete; \
	ClassName& operator=(ClassName&&) noexcept = delete

#define CHALET_DEFAULT_COPY_MOVE(ClassName)           \
	ClassName(const ClassName&) = default;            \
	ClassName(ClassName&&) noexcept = default;        \
	ClassName& operator=(const ClassName&) = default; \
	ClassName& operator=(ClassName&&) noexcept = default

#define CHALET_DEFAULT_COPY(ClassName)     \
	ClassName(const ClassName&) = default; \
	ClassName& operator=(const ClassName&) = default

#define CHALET_DEFAULT_MOVE(ClassName)         \
	ClassName(ClassName&&) noexcept = default; \
	ClassName& operator=(ClassName&&) noexcept = default

#define CHALET_CONSTANT(x) static constexpr const char x[]
