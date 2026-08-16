/**
 * @file json_loader.hpp
 * @brief Load motion data (joint_pos, joint_vel) from JSON files.
 * @author DEEPRobotics
 * @date 2026-07-30
 *
 * @copyright Copyright (c) 2026 DEEPRobotics
 *
 */

#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <Eigen/Dense>
#include "json.hpp"

namespace deep_robotics::dr02_pro {

using VecXf = Eigen::VectorXf;
using json = nlohmann::json;

/**
 * @class JsonLoader
 * @brief Load and parse motion data from JSON files.
 *
 * Reads a JSON file containing key-framed motion data (e.g. joint positions
 * and velocities per frame), and provides access to each key as a vector
 * of Eigen vectors.
 *
 * Expected JSON format:
 *   { "joint_pos": [[...], ...], "joint_vel": [[...], ...] }
 */
class JsonLoader {
public:
    /**
     * @brief Load and parse a JSON file.
     *
     * Reads the entire file into memory, then parses it with strict
     * exception handling (rejects NaN/Infinity).
     *
     * @param filename Path to the JSON file.
     * @return true if the file was loaded and parsed successfully.
     */
    bool load(const std::string& filename) {
        std::ifstream in_file(filename);
        if (!in_file.is_open()) {
            std::cerr << "Cannot open file: " << filename << std::endl;
            return false;
        }
        try {
            // Read entire file content into a string
            std::string content((std::istreambuf_iterator<char>(in_file)),
                                std::istreambuf_iterator<char>());
            // Parse with strict mode (throws on invalid JSON)
            data_ = json::parse(content, nullptr, true, true);
        } catch (const std::exception& e) {
            std::cerr << "JSON parse error: " << e.what() << std::endl;
            return false;
        }
        return true;
    }

    /**
     * @brief Extract a keyed array of float vectors from the loaded JSON.
     *
     * Each element under @p key must be a JSON array of numbers.
     * Non-array elements are silently skipped.
     *
     * @param key      Top-level key in the JSON (e.g. "joint_pos").
     * @param out_data Output vector of Eigen vectors (cleared before filling).
     * @return true if the key exists and at least partial data was extracted.
     */
    bool get_key_data(const std::string& key, std::vector<VecXf>& out_data) {
        out_data.clear();
        if (!data_.contains(key)) {
            std::cerr << "Key not found: " << key << std::endl;
            return false;
        }
        try {
            const auto& array = data_.at(key);
            if (!array.is_array()) return false;

            // Convert each JSON array element into an Eigen vector
            for (const auto& item : array) {
                if (!item.is_array()) continue;
                VecXf vec(item.size());
                for (size_t i = 0; i < item.size(); ++i) {
                    vec[i] = item.at(i).get<float>();
                }
                out_data.push_back(vec);
            }
        } catch (const std::exception& e) {
            std::cerr << "Data parse error: " << e.what() << std::endl;
            return false;
        }
        return true;
    }

private:
    json data_;  ///< Parsed JSON document.
};

}  // namespace deep_robotics::dr02_pro
