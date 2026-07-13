#include "env.h"

#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>

namespace negiysem {

namespace {

std::string trim(const std::string& s) {
    const auto begin = s.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) return "";
    const auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(begin, end - begin + 1);
}

const std::map<std::string, std::string>& dotEnv() {
    static const std::map<std::string, std::string> values = [] {
        std::map<std::string, std::string> parsed;
        std::ifstream file(".env");
        std::string line;
        while (std::getline(file, line)) {
            line = trim(line);
            if (line.empty() || line[0] == '#') continue;
            const auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            parsed[trim(line.substr(0, eq))] = trim(line.substr(eq + 1));
        }
        return parsed;
    }();
    return values;
}

}  // namespace

std::string getConfigValue(const std::string& key) {
    if (const char* value = std::getenv(key.c_str()); value && *value) {
        return value;
    }
    const auto& env = dotEnv();
    const auto it = env.find(key);
    return it == env.end() ? "" : it->second;
}

}  // namespace negiysem
