#include "common/String.h"
#include "common/platform/Platform.h"
#include "graphics/FontReader.h"
#include "graphics/Graphics.h"
#include "graphics/VideoBuffer.h"

#include <json/json.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <string_view>

namespace
{
String Text(std::initializer_list<char32_t> codepoints)
{
	return String(codepoints.begin(), codepoints.end());
}

struct ProbeText
{
	std::string_view name;
	String text;
};

bool WriteReport(std::filesystem::path const &path, std::array<ProbeText, 12> const &texts)
{
	std::ofstream report(path, std::ios::binary);
	if (!report)
		return false;
	report << "name\tcodepoint\twidth\tresource_offset\treplacement\tnonempty\n";
	for (auto const &entry : texts)
	{
		for (auto codepoint : entry.text)
		{
			FontReader reader(codepoint);
			bool nonempty = false;
			for (int pixel = 0; pixel < reader.GetWidth() * FONT_H; pixel++)
				nonempty = nonempty || reader.NextPixel() != 0;
			FontReader replacement(0xFFFD);
			report << entry.name << '\t' << "U+" << std::hex << std::uppercase << static_cast<unsigned int>(codepoint)
				<< std::dec << '\t' << reader.GetWidth() << '\t' << reader.GetResourceOffset()
				<< '\t' << (reader.GetResourceOffset() == replacement.GetResourceOffset() ? "true" : "false")
				<< '\t' << (nonempty ? "true" : "false") << '\n';
			if (!nonempty && codepoint != U' ')
				return false;
		}
	}
	return true;
}

bool ValidateCatalog(std::filesystem::path const &path, std::filesystem::path const &reportPath)
{
	std::ifstream input(path, std::ios::binary);
	if (!input)
		return false;
	std::string json((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
	Json::CharReaderBuilder builder;
	Json::Value root;
	std::string errors;
	std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	if (!reader || !reader->parse(json.data(), json.data() + json.size(), &root, &errors) || !root.isObject())
		return false;

	std::ofstream report(reportPath, std::ios::app | std::ios::binary);
	if (!report)
		return false;
	int measured = 0;
	int rendered = 0;
	int replacement = 0;
	Graphics graphics;
	for (auto const &key : root.getMemberNames())
	{
		if (!root[key].isString())
			return false;
		String text;
		try
		{
			text = ByteString(root[key].asString()).FromUtf8(false);
		}
		catch (std::exception const &)
		{
			return false;
		}
		if (text.empty())
			return false;
		(void)Graphics::TextSize(text);
		measured++;
		for (auto codepoint : text)
		{
			if (codepoint < 0x20)
				continue;
			FontReader glyph(codepoint);
			FontReader fallback(0xFFFD);
			if (glyph.GetResourceOffset() == fallback.GetResourceOffset() && codepoint != 0xFFFD)
				replacement++;
		}
		graphics.Clear();
		graphics.BlendText({ 8, 8 }, text, (0xFFFFFF_rgb).WithAlpha(255));
		rendered++;
	}
	report << "catalog_entries\t" << measured << '\n';
	report << "catalog_measured\t" << measured << '\n';
	report << "catalog_rendered\t" << rendered << '\n';
	report << "catalog_replacement_glyphs\t" << replacement << '\n';
	return replacement == 0;
}
}

int main(int argc, char *argv[])
{
	if (argc != 2 && argc != 3)
	{
		std::cerr << "usage: font_render_probe <output-directory> [zh-CN.json]\n";
		return 2;
	}
	auto output = std::filesystem::path(argv[1]);
	std::error_code error;
	std::filesystem::create_directories(output, error);
	if (error)
	{
		std::cerr << "could not create output directory: " << error.message() << '\n';
		return 2;
	}

	std::array<ProbeText, 12> texts = {{
		{ "simplified-chinese", Text({ 0x7B80, 0x4F53, 0x4E2D, 0x6587 }) },
		{ "metallurgy", Text({ 0x5DE5, 0x4E1A, 0x51B6, 0x91D1 }) },
		{ "ecology", Text({ 0x5C40, 0x90E8, 0x751F, 0x6001 }) },
		{ "chemistry", Text({ 0x9AD8, 0x7EA7, 0x5316, 0x5B66 }) },
		{ "nuclear", Text({ 0x53D7, 0x63A7, 0x6838, 0x5DE5, 0x4E1A }) },
		{ "common-ui", Text({ 0x8BBE, 0x7F6E, 0x4FDD, 0x5B58, 0x52A0, 0x8F7D, 0x53D6, 0x6D88 }) },
		{ "readonly-load", Text({ 0x53EA, 0x8BFB, 0x52A0, 0x8F7D }) },
		{ "mixed-symbols", Text({ U'T', U'P', U'T', U'-', U'Z', U'H', U'-', U'O', U'm', U'n', U'i', U'P', U'a', U'c', U'k', U' ', U'0', U'.', U'1', U'.', U'0', U'-', U't', U'e', U's', U't' }) },
		{ "alloy", Text({ 0x94DC, U' ', U'+', U' ', 0x9521, U' ', 0x2192, U' ', 0x9752, 0x94DC }) },
		{ "temperature", Text({ 0x6E29, 0x5EA6, U':', U' ', U'1', U'0', U'0', U'0', 0x00B0, U'C' }) },
		{ "settings", Text({ 0x8BBE, 0x7F6E }) },
		{ "cancel", Text({ 0x53D6, 0x6D88 }) },
	}};

	if (!WriteReport(output / "font-render-probe.tsv", texts))
	{
		std::cerr << "font probe encountered an empty glyph or could not write its report\n";
		return 1;
	}
	if (argc == 3 && !ValidateCatalog(argv[2], output / "font-render-probe.tsv"))
	{
		std::cerr << "font probe could not fully measure and render the Chinese catalog\n";
		return 1;
	}

	Graphics graphics;
	graphics.Clear();
	int y = 12;
	for (auto const &entry : texts)
	{
		graphics.BlendText({ 8, y }, entry.text, (0xFFFFFF_rgb).WithAlpha(255));
		y += FONT_H + 4;
	}
	auto frame = graphics.DumpFrame();
	if (auto png = frame.ToPNG(); png && Platform::WriteFile(*png, (output / "font-render-probe.png").string()))
	{
		std::cout << "font_render_probe: PASS output=" << output.string() << " measured_width="
			<< Graphics::TextSize(texts.front().text).X << '\n';
		return 0;
	}
	std::cerr << "could not write rendered PNG\n";
	return 1;
}
