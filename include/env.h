#pragma once

#include <string>

namespace negiysem {

// Reads a config value from the process environment, falling back to a .env
// file in the working directory (KEY=VALUE lines, # comments). Returns an
// empty string when the key is set nowhere.
std::string getConfigValue(const std::string& key);

}  // namespace negiysem
