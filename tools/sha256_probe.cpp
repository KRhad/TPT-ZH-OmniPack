#include "common/Sha256.h"
#include <iostream>
#include <span>
#include <string>

namespace
{
	bool Check(const std::string &input, const char *expected)
	{
		auto actual = Sha256Hex(std::span<const char>(input.data(), input.size()));
		if (actual == expected)
			return true;
		std::cerr << "SHA-256 mismatch: " << actual << std::endl;
		return false;
	}
}

int main()
{
	return Check("", "E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855") &&
		Check("abc", "BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD")
		? 0 : 1;
}
