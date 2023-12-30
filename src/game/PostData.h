#pragma once
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace http
{
	struct Header
	{
		std::string name;
		std::string value;
	};
	struct FormItem
	{
		std::string name;
		std::string value;
		std::optional<std::string> filename;
	};
	using StringData = std::string;
	using FormData = std::vector<FormItem>;
	using PostData = std::variant<StringData, FormData>;
}
