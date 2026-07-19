#pragma once

#include <exception>
#include <string>

bool ValidateGOLName(const std::string &value);
int ParseGOLString(const std::string &value);
std::string SerialiseGOLRule(int rule);
