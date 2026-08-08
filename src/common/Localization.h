#pragma once
#include "common/String.h"
#include <array>
#include <map>
#include <string>
 
// 简单的全局本地化管理器：
// - 根据当前语言索引加载内嵌的 JSON 语言资源
// - 提供 Tr(key[, fallback]) 获取翻译
 class Localization
 {
 public:
 	static Localization &Ref();
 
 	// 设置当前语言索引（与 GlobalPrefs::Ref().Get("Language") 对应）
 	void SetLanguageIndex(int index);
 
	// 获取翻译：先查当前语言,再查 en-US；若仍无且提供 fallback 则返回 fallback
	String Tr(const char *key, const char *fallback = nullptr) const;

	// 查询指定语言而不切换当前界面语言；所有语言表只在构造时解析一次
	String TrForLanguage(int index, const char *key, const char *fallback = nullptr) const;

 private:
	using TranslationTable = std::map<std::string, String>;
	static constexpr int LanguageCount = 12;

	Localization();
	void LoadLanguage(int index);
	void LoadFallbackEnglish();
	void Clear();

	int currentIndex{-1};
	std::array<TranslationTable, LanguageCount> languageCache;
	std::array<bool, LanguageCount> languageAvailable{};
	TranslationTable entries;
	TranslationTable fallbackEn;  // en-US,key 缺省时使用
 };

