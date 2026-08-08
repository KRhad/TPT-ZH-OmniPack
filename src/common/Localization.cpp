#include "Localization.h"
#include <cstring>
#include <exception>
#include <iostream>
#include <json/json.h>
#include <memory>
#include <span>
#include <utility>
// 顺序与 LoadLanguage(index) 一致：en, zh-CN, zh-TW, de, es, fr, it, ja, ko, pt, ru, lzh
#include "lang_en_US_json.h"
#include "lang_zh_CN_json.h"
#include "lang_zh_TW_json.h"
#include "lang_de_DE_json.h"
#include "lang_es_ES_json.h"
#include "lang_fr_FR_json.h"
#include "lang_it_IT_json.h"
#include "lang_ja_JP_json.h"
#include "lang_ko_KR_json.h"
#include "lang_pt_BR_json.h"
#include "lang_ru_RU_json.h"
#include "lang_lzh_json.h"

static bool ParseLanguageJson(
	std::span<const char> text,
	std::map<std::string, String> &out,
	const char *resourceName
);

struct LanguageResource
{
	std::span<const char> data;
	const char *name;
};

static LanguageResource GetLanguageResource(int index)
{
	switch (index)
	{
	case 0:  return { lang_en_US_json.AsCharSpan(), "en-US" };
	case 1:  return { lang_zh_CN_json.AsCharSpan(), "zh-CN" };
	case 2:  return { lang_zh_TW_json.AsCharSpan(), "zh-TW" };
	case 3:  return { lang_de_DE_json.AsCharSpan(), "de-DE" };
	case 4:  return { lang_es_ES_json.AsCharSpan(), "es-ES" };
	case 5:  return { lang_fr_FR_json.AsCharSpan(), "fr-FR" };
	case 6:  return { lang_it_IT_json.AsCharSpan(), "it-IT" };
	case 7:  return { lang_ja_JP_json.AsCharSpan(), "ja-JP" };
	case 8:  return { lang_ko_KR_json.AsCharSpan(), "ko-KR" };
	case 9:  return { lang_pt_BR_json.AsCharSpan(), "pt-BR" };
	case 10: return { lang_ru_RU_json.AsCharSpan(), "ru-RU" };
	case 11: return { lang_lzh_json.AsCharSpan(), "lzh" };
	default: return { lang_en_US_json.AsCharSpan(), "en-US" };
	}
}
 
 Localization &Localization::Ref()
 {
 	static Localization inst;
 	return inst;
 }

 Localization::Localization()
 {
	for (int index = 0; index < LanguageCount; ++index)
	{
		auto resource = GetLanguageResource(index);
		languageAvailable[index] = ParseLanguageJson(resource.data, languageCache[index], resource.name);
	}
 	LoadFallbackEnglish();
 }
 
 void Localization::Clear()
 {
 	entries.clear();
 }

void Localization::LoadFallbackEnglish()
{
	if (!fallbackEn.empty())
		return;
	if (languageAvailable[0])
		fallbackEn = languageCache[0];
}

// 将语言文件中为了保持 JSON 合法而写成字面量的控制序列转换为渲染器使用的字节。
// \b+字母 对应 C++ 字符串中的 "\bg" 等；\x0E 用于结束富文本链接。
static void ConvertLegacyFormatCodes(std::string &text)
{
	const char *formatLetters = "wgorlbtuU";
	for (size_t i = 0; i < text.size(); )
	{
		if (text[i] == '\\' && i + 2 < text.size() && text[i + 1] == 'b')
		{
			char code = text[i + 2];
			if (std::strchr(formatLetters, code))
			{
				text[i] = '\b';
				text[i + 1] = code;
				text.erase(i + 2, 1);
				i += 2;
				continue;
			}
		}
		if (
			text[i] == '\\' &&
			i + 3 < text.size() &&
			text[i + 1] == 'x' &&
			text[i + 2] == '0' &&
			text[i + 3] == 'E'
		)
		{
			text[i] = '\x0E';
			text.erase(i + 1, 3);
			++i;
			continue;
		}
		++i;
	}
}

static std::string OneLineJsonError(std::string error)
{
	for (char &ch : error)
	{
		if (ch == '\r' || ch == '\n' || ch == '\t')
			ch = ' ';
	}
	while (!error.empty() && error.back() == ' ')
		error.pop_back();
	return error;
}

static void ReportLanguageError(const char *resourceName, const std::string &detail)
{
	std::cerr << "Localization: " << resourceName << ": " << detail << std::endl;
}

// JsonCpp 1.9.5 may still skip comments at a few object/array boundaries even
// when allowComments is false. Reject comment openers outside strings before
// invoking the strict reader so every embedded resource obeys JSON syntax.
static bool ContainsJsonComment(std::span<const char> text)
{
	bool inString = false;
	bool escaped = false;
	for (size_t index = 0; index < text.size(); ++index)
	{
		char current = text[index];
		if (inString)
		{
			if (escaped)
				escaped = false;
			else if (current == '\\')
				escaped = true;
			else if (current == '"')
				inString = false;
		}
		else if (current == '"')
		{
			inString = true;
		}
		else if (
			current == '/' &&
			index + 1 < text.size() &&
			(text[index + 1] == '/' || text[index + 1] == '*')
		)
		{
			return true;
		}
	}
	return false;
}

static bool ParseLanguageJson(
	std::span<const char> text,
	std::map<std::string, String> &out,
	const char *resourceName
)
{
	if (text.empty())
	{
		ReportLanguageError(resourceName, "empty JSON resource");
		return false;
	}
	if (ContainsJsonComment(text))
	{
		ReportLanguageError(resourceName, "JSON comments are not allowed");
		return false;
	}

	try
	{
		Json::CharReaderBuilder builder;
		Json::CharReaderBuilder::strictMode(&builder.settings_);
		builder["collectComments"] = false;
		builder["allowComments"] = false;
		builder["allowTrailingCommas"] = false;
		builder["strictRoot"] = true;
		builder["allowDroppedNullPlaceholders"] = false;
		builder["allowNumericKeys"] = false;
		builder["allowSingleQuotes"] = false;
		builder["failIfExtra"] = true;
		builder["rejectDupKeys"] = true;
		builder["allowSpecialFloats"] = false;
		builder["skipBom"] = false;

		std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
		if (!reader)
		{
			ReportLanguageError(resourceName, "could not create JSON reader");
			return false;
		}

		Json::Value root;
		std::string errors;
		if (!reader->parse(text.data(), text.data() + text.size(), &root, &errors))
		{
			ReportLanguageError(resourceName, "JSON parse failed: " + OneLineJsonError(std::move(errors)));
			return false;
		}
		if (!root.isObject())
		{
			ReportLanguageError(resourceName, "JSON root must be an object");
			return false;
		}

		std::map<std::string, String> parsed;
		for (const auto &key : root.getMemberNames())
		{
			const Json::Value &value = root[key];
			if (!value.isString())
			{
				ReportLanguageError(resourceName, "value for key \"" + key + "\" must be a string");
				return false;
			}

			std::string translated = value.asString();
			ConvertLegacyFormatCodes(translated);
			parsed.emplace(key, ByteString(translated).FromUtf8());
		}

		out = std::move(parsed);
		return true;
	}
	catch (const std::exception &error)
	{
		ReportLanguageError(resourceName, "JSON processing failed: " + OneLineJsonError(error.what()));
		return false;
	}
	catch (...)
	{
		ReportLanguageError(resourceName, "JSON processing failed: unknown exception");
		return false;
	}
}

void Localization::LoadLanguage(int index)
{
	Clear();

	int cacheIndex = index >= 0 && index < LanguageCount ? index : 0;
	if (languageAvailable[cacheIndex])
		entries = languageCache[cacheIndex];
	else
		entries = fallbackEn;
 }
 
 void Localization::SetLanguageIndex(int index)
 {
 	if (index == currentIndex)
 		return;
 	currentIndex = index;
 	LoadLanguage(index);
 }
 
 String Localization::Tr(const char *key, const char *fallback) const
 {
 	auto it = entries.find(key);
 	if (it != entries.end())
 		return it->second;
 	auto itEn = fallbackEn.find(key);
 	if (itEn != fallbackEn.end())
 		return itEn->second;
	if (fallback)
		return ByteString(fallback).FromUtf8();
	return String();
 }

String Localization::TrForLanguage(int index, const char *key, const char *fallback) const
{
	int cacheIndex = index >= 0 && index < LanguageCount ? index : 0;
	if (languageAvailable[cacheIndex])
	{
		auto it = languageCache[cacheIndex].find(key);
		if (it != languageCache[cacheIndex].end())
			return it->second;
	}
	auto itEn = fallbackEn.find(key);
	if (itEn != fallbackEn.end())
		return itEn->second;
	if (fallback)
		return ByteString(fallback).FromUtf8();
	return String();
}

