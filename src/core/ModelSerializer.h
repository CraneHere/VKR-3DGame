#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdio>
#include <glm/glm.hpp>

struct ModelInstanceData {
    std::string modelName;
    glm::vec3 position{0.0f};
    float scale = 0.02f;
    float rotationY = 0.0f;
};

namespace ModelSerializer {

inline bool Save(const std::string& path, const std::vector<ModelInstanceData>& instances)
{
    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "ModelSerializer: cannot open for writing: " << path << std::endl;
        return false;
    }

    file << "[\n";
    for (size_t i = 0; i < instances.size(); i++) {
        const auto& inst = instances[i];
        char buf[512];
        std::snprintf(buf, sizeof(buf),
            "  { \"model\": \"%s\", \"x\": %.3f, \"y\": %.3f, \"z\": %.3f, \"scale\": %.4f, \"rotationY\": %.2f }",
            inst.modelName.c_str(),
            inst.position.x, inst.position.y, inst.position.z,
            inst.scale, inst.rotationY);
        file << buf;
        if (i + 1 < instances.size()) file << ",";
        file << "\n";
    }
    file << "]\n";
    file.close();

    std::cout << "Saved " << instances.size() << " model instances to " << path << std::endl;
    return true;
}

inline float parseFloat(const std::string& block, const std::string& key, float fallback = 0.0f)
{
    size_t p = block.find("\"" + key + "\"");
    if (p == std::string::npos) return fallback;
    p = block.find(':', p);
    if (p == std::string::npos) return fallback;
    return std::stof(block.substr(p + 1));
}

inline std::string parseString(const std::string& block, const std::string& key)
{
    size_t p = block.find("\"" + key + "\"");
    if (p == std::string::npos) return "";
    p = block.find(':', p);
    if (p == std::string::npos) return "";
    size_t q1 = block.find('"', p + 1);
    size_t q2 = block.find('"', q1 + 1);
    if (q1 == std::string::npos || q2 == std::string::npos) return "";
    return block.substr(q1 + 1, q2 - q1 - 1);
}

inline std::vector<ModelInstanceData> Load(const std::string& path)
{
    std::vector<ModelInstanceData> result;

    std::ifstream file(path);
    if (!file.is_open()) {
        std::cout << "ModelSerializer: no file found at " << path << " (starting empty)" << std::endl;
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
        ModelInstanceData inst;
        inst.modelName = parseString(block, "model");
        inst.position.x = parseFloat(block, "x");
        inst.position.y = parseFloat(block, "y");
        inst.position.z = parseFloat(block, "z");
        inst.scale = parseFloat(block, "scale", 0.02f);
        inst.rotationY = parseFloat(block, "rotationY");

        if (!inst.modelName.empty())
            result.push_back(inst);

        pos = end + 1;
    }

    std::cout << "Loaded " << result.size() << " model instances from " << path << std::endl;
    return result;
}

}
