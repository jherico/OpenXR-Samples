//
//  Created by Bradley Austin Davis on 2019/09/22
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <filesystem>
#include <mutex>
#include <fstream>
#include <stdexcept>

namespace assets {
using path = std::filesystem::path;

inline const path& getAssetPath() {
    static path result;
    static std::once_flag once;
    std::call_once(once, [] { result = path(__FILE__).parent_path().parent_path().parent_path() / "data"; });
    return result;
}

inline path getAssetPath(const std::string& relative) {
    return getAssetPath() / relative;
}

inline std::string getAssetPathString(const std::string& relative) {
    return getAssetPath(relative).string();
}

inline std::string getAssetContents(const std::string& relative) {
    std::ifstream file(getAssetPath(relative));
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

inline std::vector<uint8_t> getAssetContentsBinary(const std::string& relative) {
    std::ifstream file(getAssetPath(relative), std::ios::binary | std::ios::ate);
    auto size = file.tellg();
    if (!size) {
        return {};
    }
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> result;
    result.resize(size);
    if (!file.read((char*)result.data(), size)) {
        throw std::runtime_error("Failed to read file");
    }
    return result;
}

}  // namespace assets
