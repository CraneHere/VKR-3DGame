#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdio>
#include <glm/glm.hpp>

struct HouseData {
    glm::vec3 position{0.0f};
    float scale = 0.02f;
    float rotationY = 0.0f;
};

namespace HouseSerializer {

inline bool SaveHouses(const std::string& path, const std::vector<HouseData>& houses)
{
    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "HouseSerializer: cannot open for writing: " << path << std::endl;
        return false;
    }

    file << "[\n";
    for (size_t i = 0; i < houses.size(); i++) {
        const auto& h = houses[i];
        char buf[256];
        std::snprintf(buf, sizeof(buf),
            "  { \"x\": %.3f, \"y\": %.3f, \"z\": %.3f, \"scale\": %.4f, \"rotationY\": %.2f }",
            h.position.x, h.position.y, h.position.z, h.scale, h.rotationY);
        file << buf;
        if (i + 1 < houses.size()) file << ",";
        file << "\n";
    }
    file << "]\n";
    file.close();

    std::cout << "Saved " << houses.size() << " houses to " << path << std::endl;
    return true;
}

inline std::vector<HouseData> LoadHouses(const std::string& path)
{
    std::vector<HouseData> result;

    std::ifstream file(path);
    if (!file.is_open()) {
        std::cout << "HouseSerializer: no file found at " << path << " (starting empty)" << std::endl;
        return result;
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    file.close();

    size_t pos = 0;
    while ((pos = content.find('{', pos)) != std::string::npos) {
        size_t end = content.find('}', pos);
        if (end == std::string::npos) break;

        std::string block = content.substr(pos, end - pos + 1);
        HouseData h;

        if (std::sscanf(block.c_str(),
                "{ \"x\": %f, \"y\": %f, \"z\": %f, \"scale\": %f, \"rotationY\": %f }",
                &h.position.x, &h.position.y, &h.position.z, &h.scale, &h.rotationY) == 5)
        {
            result.push_back(h);
        }

        pos = end + 1;
    }

    std::cout << "Loaded " << result.size() << " houses from " << path << std::endl;
    return result;
}

}
