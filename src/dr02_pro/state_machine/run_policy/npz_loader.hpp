/**
 * @file npz_loader.hpp
 * @brief Load motion data (joint_pos, joint_vel) from NPZ files.
 * @author DEEPRobotics
 * @date 2026-09-06
 *
 * @copyright Copyright (c) 2026 DEEPRobotics
 *
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <cstring>
#include <cstdint>
#include <iostream>
#include <Eigen/Dense>

namespace deep_robotics::dr02_pro {

using VecXf = Eigen::VectorXf;

/**
 * @class NpzLoader
 * @brief Load and parse motion data from NPZ (zipped .npy) files.
 *
 * An NPZ file is a ZIP archive whose entries are .npy arrays. This loader
 * supports uncompressed (ZIP_STORED) entries, which is what numpy.savez
 * produces, and exposes each array as a vector of Eigen vectors.
 *
 * Expected NPZ content:
 *   joint_pos.npy: float32 array of shape (num_frames, num_joints)
 *   joint_vel.npy: float32 array of shape (num_frames, num_joints)
 */
class NpzLoader {
public:
    /**
     * @brief Load and parse an NPZ file.
     *
     * Reads the entire file into memory, walks the ZIP central directory
     * and stores each uncompressed entry (key = name without ".npy").
     * Compressed (deflated) entries are not supported.
     *
     * @param filename Path to the NPZ file.
     * @return true if the file was loaded and at least one entry parsed.
     */
    bool load(const std::string& filename) {
        entries_.clear();

        std::ifstream in_file(filename, std::ios::binary);
        if (!in_file.is_open()) {
            std::cerr << "Cannot open file: " << filename << std::endl;
            return false;
        }
        std::vector<uint8_t> buf((std::istreambuf_iterator<char>(in_file)),
                                 std::istreambuf_iterator<char>());

        // --- Locate the End Of Central Directory record (EOCD) ---
        const size_t kMinEocdSize = 22;
        const size_t kMaxCommentSize = 65535;
        if (buf.size() < kMinEocdSize) {
            std::cerr << "NPZ parse error: file too small" << std::endl;
            return false;
        }
        size_t scan_end = buf.size() - kMinEocdSize;
        size_t scan_begin =
            scan_end > kMaxCommentSize ? scan_end - kMaxCommentSize : 0;
        size_t eocd = std::string::npos;
        for (size_t i = scan_end + 1; i-- > scan_begin;) {
            if (ReadU32(buf.data() + i) == 0x06054b50u) {
                eocd = i;
                break;
            }
        }
        if (eocd == std::string::npos) {
            std::cerr << "NPZ parse error: EOCD not found" << std::endl;
            return false;
        }

        // --- Walk the central directory entries ---
        const uint16_t entry_count = ReadU16(buf.data() + eocd + 10);
        size_t p = ReadU32(buf.data() + eocd + 16);
        for (uint16_t e = 0; e < entry_count; ++e) {
            if (p + 46 > buf.size() ||
                ReadU32(buf.data() + p) != 0x02014b50u) {
                std::cerr << "NPZ parse error: bad central directory entry"
                          << std::endl;
                return false;
            }

            const uint16_t method = ReadU16(buf.data() + p + 10);
            const uint32_t uncomp_size = ReadU32(buf.data() + p + 24);
            const uint16_t name_len = ReadU16(buf.data() + p + 28);
            const uint16_t extra_len = ReadU16(buf.data() + p + 30);
            const uint16_t comment_len = ReadU16(buf.data() + p + 32);
            const uint32_t local_offset = ReadU32(buf.data() + p + 42);
            const std::string name(
                reinterpret_cast<const char*>(buf.data() + p + 46), name_len);

            p += 46 + name_len + extra_len + comment_len;

            if (!name.empty() && name.back() == '/') continue;  // directory

            // --- Read the local file header to find the entry data ---
            if (local_offset + 30 > buf.size() ||
                ReadU32(buf.data() + local_offset) != 0x04034b50u) {
                std::cerr << "NPZ parse error: bad local header for " << name
                          << std::endl;
                return false;
            }
            const uint16_t l_name_len = ReadU16(buf.data() + local_offset + 26);
            const uint16_t l_extra_len = ReadU16(buf.data() + local_offset + 28);
            const size_t data_start = local_offset + 30 + l_name_len + l_extra_len;

            if (method != 0) {
                std::cerr << "NPZ parse error: compressed entry " << name
                          << " not supported (use numpy.savez, not"
                          << " numpy.savez_compressed)" << std::endl;
                return false;
            }
            if (data_start + uncomp_size > buf.size()) {
                std::cerr << "NPZ parse error: entry " << name
                          << " exceeds file size" << std::endl;
                return false;
            }

            // Key = entry name without the ".npy" suffix
            std::string key = name;
            if (key.size() > 4 &&
                key.compare(key.size() - 4, 4, ".npy") == 0) {
                key.erase(key.size() - 4);
            }
            entries_[key].assign(buf.begin() + data_start,
                                 buf.begin() + data_start + uncomp_size);
        }

        if (entries_.empty()) {
            std::cerr << "NPZ parse error: no entries found" << std::endl;
            return false;
        }
        return true;
    }

    /**
     * @brief Extract a keyed 2-D float32 array from the loaded NPZ.
     *
     * The .npy entry under @p key must be a little-endian float32 array
     * of shape (num_frames, dim) in C order. Each row becomes one Eigen
     * vector.
     *
     * @param key      Array key in the NPZ (e.g. "joint_pos").
     * @param out_data Output vector of Eigen vectors (cleared before filling).
     * @return true if the key exists and data was extracted.
     */
    bool get_key_data(const std::string& key, std::vector<VecXf>& out_data) {
        out_data.clear();
        auto it = entries_.find(key);
        if (it == entries_.end()) {
            std::cerr << "Key not found in npz: " << key << std::endl;
            return false;
        }

        // --- Parse the .npy header ---
        const std::vector<uint8_t>& raw = it->second;
        const size_t kMagicLen = 10;  // magic(6) + version(2) + header len(2)
        if (raw.size() < kMagicLen ||
            std::memcmp(raw.data(), "\x93NUMPY", 6) != 0) {
            std::cerr << "NPY parse error: bad magic for " << key << std::endl;
            return false;
        }
        const uint8_t major = raw[6];
        size_t header_begin;
        size_t header_len;
        if (major <= 1) {
            header_begin = 10;
            header_len = ReadU16(raw.data() + 8);
        } else {
            header_begin = 12;
            header_len = ReadU32(raw.data() + 8);
        }
        if (header_begin + header_len > raw.size()) {
            std::cerr << "NPY parse error: bad header length for " << key
                      << std::endl;
            return false;
        }
        const std::string header(
            reinterpret_cast<const char*>(raw.data()) + header_begin,
            header_len);
        const size_t data_offset = header_begin + header_len;

        // --- Parse descr / fortran_order / shape fields ---
        const std::string descr = ParseQuotedValue(header, "'descr'");
        const size_t fo_pos = header.find("fortran_order");
        const bool fortran_order =
            fo_pos != std::string::npos &&
            header.find("True", fo_pos) < header.find("}", fo_pos);
        std::vector<size_t> shape;
        ParseShape(header, shape);

        if (descr != "<f4") {
            std::cerr << "NPY parse error: " << key
                      << " must be little-endian float32, got " << descr
                      << std::endl;
            return false;
        }
        if (fortran_order) {
            std::cerr << "NPY parse error: " << key
                      << " must be in C order" << std::endl;
            return false;
        }
        if (shape.size() != 2) {
            std::cerr << "NPY parse error: " << key
                      << " must be a 2-D array" << std::endl;
            return false;
        }

        const size_t n = shape[0];
        const size_t d = shape[1];
        if (data_offset + n * d * sizeof(float) > raw.size()) {
            std::cerr << "NPY parse error: " << key
                      << " data exceeds entry size" << std::endl;
            return false;
        }

        // --- Copy each row into an Eigen vector ---
        out_data.resize(n);
        const float* data = reinterpret_cast<const float*>(raw.data() + data_offset);
        for (size_t i = 0; i < n; ++i) {
            out_data[i] = VecXf(d);
            std::memcpy(out_data[i].data(), data + i * d, d * sizeof(float));
        }
        return true;
    }

private:
    std::map<std::string, std::vector<uint8_t>> entries_;  ///< Raw .npy entries.

    /**
     * @brief Read a little-endian 16-bit unsigned integer.
     * @param p Pointer to the byte sequence.
     * @return Decoded value.
     */
    static uint16_t ReadU16(const uint8_t* p) {
        return static_cast<uint16_t>(p[0]) |
               (static_cast<uint16_t>(p[1]) << 8);
    }

    /**
     * @brief Read a little-endian 32-bit unsigned integer.
     * @param p Pointer to the byte sequence.
     * @return Decoded value.
     */
    static uint32_t ReadU32(const uint8_t* p) {
        return static_cast<uint32_t>(p[0]) |
               (static_cast<uint32_t>(p[1]) << 8) |
               (static_cast<uint32_t>(p[2]) << 16) |
               (static_cast<uint32_t>(p[3]) << 24);
    }

    /**
     * @brief Extract the single-quoted value following a dict key.
     *
     * @param header .npy header dictionary string.
     * @param key    Dict key to look up (e.g. "'descr'").
     * @return Quoted value without quotes, or "" if not found.
     */
    static std::string ParseQuotedValue(const std::string& header,
                                        const std::string& key) {
        const size_t key_pos = header.find(key);
        if (key_pos == std::string::npos) return "";
        const size_t begin = header.find('\'', key_pos + key.size());
        if (begin == std::string::npos) return "";
        const size_t end = header.find('\'', begin + 1);
        if (end == std::string::npos) return "";
        return header.substr(begin + 1, end - begin - 1);
    }

    /**
     * @brief Parse the shape tuple from an .npy header.
     *
     * @param header .npy header dictionary string.
     * @param shape  Output dimension list.
     */
    static void ParseShape(const std::string& header,
                           std::vector<size_t>& shape) {
        shape.clear();
        const size_t key_pos = header.find("'shape'");
        if (key_pos == std::string::npos) return;
        const size_t begin = header.find('(', key_pos);
        const size_t end = header.find(')', key_pos);
        if (begin == std::string::npos || end == std::string::npos ||
            end <= begin) {
            return;
        }
        size_t num = 0;
        bool has_digit = false;
        for (size_t i = begin + 1; i < end; ++i) {
            if (std::isdigit(static_cast<unsigned char>(header[i]))) {
                num = num * 10 + (header[i] - '0');
                has_digit = true;
            } else if (has_digit) {
                shape.push_back(num);
                num = 0;
                has_digit = false;
            }
        }
        if (has_digit) shape.push_back(num);
    }
};

}  // namespace deep_robotics::dr02_pro
