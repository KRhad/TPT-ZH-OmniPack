#pragma once

#include <sstream>
#include <vector>

namespace gfx
{
class VideoBuffer;
}

namespace Format
{
	const static char hex[] = "0123456789ABCDEF";

	template <typename T> std::string NumberToString(T number)
	{
		std::stringstream ss;
		ss << number;
		return ss.str();
	}

	template <typename T> T StringToNumber(const std::string & text)
	{
		std::stringstream ss(text);
		T number;
		return (ss >> number)?number:0;
	}

	template <typename T> T StringToNumberThrowing(const std::string & text)
	{
		std::stringstream ss(text);
		T number;
		ss >> number;
		if (ss.eof())
			return number;
		throw std::runtime_error("Not a number");
	}

	std::string ToLower(std::string text);
	std::string ToUpper(std::string text);

	std::string URLEncode(std::string value);
	std::string UnixtimeToDate(time_t unixtime, std::string dateFomat = "%d %b %Y");
	std::string UnixtimeToDateMini(time_t unixtime);
	std::string CleanString(std::string dirtyString, bool ascii, bool color, bool newlines, bool numeric = false);
	std::string CleanString(const char * dirtyData, bool ascii, bool color, bool newlines, bool numeric = false);
	std::vector<char> VideoBufferToPNG(const gfx::VideoBuffer & vidBuf);
	std::vector<char> VideoBufferToBMP(const gfx::VideoBuffer & vidBuf);
	std::vector<char> VideoBufferToPPM(const gfx::VideoBuffer & vidBuf);
	std::vector<char> VideoBufferToPTI(const gfx::VideoBuffer & vidBuf);
	gfx::VideoBuffer * PTIToVideoBuffer(std::vector<char> & data);
	unsigned long CalculateCRC(unsigned char * data, int length);
	std::string TemperatureToString(float temp, int scale, int precision = 2);
	float StringToTemperature(const std::string & str, int defaultScale);
}
