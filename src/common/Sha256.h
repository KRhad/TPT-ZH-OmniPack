#pragma once

#include "common/String.h"
#include <span>

ByteString Sha256Hex(std::span<const char> data);
