/*
 *  VS_LocString.h
 *  VectorStorm
 *
 *  Created by Trevor Powell on 09/03/2020
 *  Copyright 2020 Trevor Powell.  All rights reserved.
 *
 */

#ifndef VS_LOCSTRING_H
#define VS_LOCSTRING_H

#include "VS_LocalisationTable.h"
#include "VS/Utils/fmt/format.h"
// #include "VS/Utils/VS_String.h"
// #include "VS/Utils/VS_Array.h"

class vsLocString
{
public:
	vsString m_key;
	// vsArray<struct Arg*> m_args;

	vsLocString( const vsString& key);

	// operator vsString() const;
};

// vsLocString vsLocFormat(fmt::CStringRef format_str, fmt::ArgList args);
// FMT_VARIADIC(vsLocString, vsLocFormat, fmt::CStringRef);

// void format_arg(fmt::BasicFormatter<char> &f, const char *&format_str, const vsLocString &s);

template <>
struct fmt::formatter<vsLocString> {
	// constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
	// 	auto it = ctx.begin();
	// 	return it;
	// }

	template <typename FormatContext>
		auto format(const vsLocString& p, FormatContext& ctx) -> decltype(ctx.out()) {
			return format_to(
					ctx.out(),
					"{}",
					p.m_key
					);
		}
};

#endif // VS_LOCSTRING_H

