#include "utils/json_parser.hpp"
#include "utils/loading_state.hpp"

#include <array>
#include <cstddef>
#include <iostream>
#include <simdjson.h>
#include <sstream>
#include <string>
#include <utility>
#include <zlib.h>

namespace {
constexpr double BYTES_TO_GB = 1024.0 * 1024.0 * 1024.0;
constexpr size_t BATCH_SIZE = 1024ULL * 1024ULL;  // 1MB batch for NDJSON
constexpr size_t PROGRESS_INTERVAL = 100000;
constexpr size_t GZ_BUFFER_SIZE = 128 * 1024;  // 128KB read buffer

bool ends_with(const std::string& str, const std::string& suffix) {
    if (suffix.size() > str.size()) {
        return false;
    }
    return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string decompress_gzip(const std::string& file_path) {
    LoadingState& state = get_loading_state();
    state.status_message = "Decompressing gzip file...";

    gzFile gz_file = gzopen(file_path.c_str(), "rb");
    if (gz_file == nullptr) {
        state.error_message = "Error opening gzip file";
        return "";
    }

    std::string result;
    std::array<char, GZ_BUFFER_SIZE> buffer{};
    int bytes_read = 0;

    while ((bytes_read = gzread(gz_file, buffer.data(), GZ_BUFFER_SIZE)) > 0) {
        result.append(buffer.data(), static_cast<size_t>(bytes_read));
    }

    gzclose(gz_file);
    return result;
}

std::string format_size(size_t bytes) {
    std::ostringstream oss;
    double size_gb = static_cast<double>(bytes) / BYTES_TO_GB;
    if (size_gb >= 1.0) {
        oss << bytes << " bytes (" << size_gb << " GB)";
    } else {
        double size_mb = static_cast<double>(bytes) / (1024.0 * 1024.0);
        oss << bytes << " bytes (" << size_mb << " MB)";
    }
    return oss.str();
}
}  // namespace

void json_parser(const std::string& file_path) {
    static simdjson::ondemand::parser parser;
    LoadingState& state = get_loading_state();

    state.reset();
    state.is_loading = true;
    state.status_message = "Loading file...";

    simdjson::padded_string json;

    // Handle gzipped files
    if (ends_with(file_path, ".gz")) {
        std::string decompressed = decompress_gzip(file_path);
        if (decompressed.empty()) {
            state.is_loading = false;
            return;
        }
        json = simdjson::padded_string(decompressed);
    } else {
        auto json_result = simdjson::padded_string::load(file_path);
        if (json_result.error() != simdjson::SUCCESS) {
            state.error_message = "Error loading file";
            state.is_loading = false;
            return;
        }
        json = std::move(json_result.value());
    }

    state.file_size_bytes = json.size();
    state.status_message = "File loaded: " + format_size(json.size());

    // NDJSON streaming
    if (ends_with(file_path, ".ndjson") || ends_with(file_path, ".ndjson.gz")) {
        state.status_message = "NDJSON detected, streaming...";

        simdjson::ondemand::document_stream stream;
        auto stream_error = parser.iterate_many(json, BATCH_SIZE).get(stream);
        if (stream_error != simdjson::SUCCESS) {
            state.error_message = "Error creating stream";
            state.is_loading = false;
            return;
        }

        size_t doc_count = 0;
        for (auto doc : stream) {
            if (doc.error() != simdjson::SUCCESS) {
                state.error_message = "Error at document " + std::to_string(doc_count);
                break;
            }
            doc_count++;
            state.documents_loaded = doc_count;

            if (doc_count % PROGRESS_INTERVAL == 0) {
                state.status_message = "Processed " + std::to_string(doc_count) + " documents...";
            }
        }

        state.status_message = "Complete! Total: " + std::to_string(doc_count) + " documents";
        state.is_loading = false;
        state.is_complete = true;
        return;
    }

    // Regular single-document parsing
    if (parser.capacity() < json.size()) {
        auto alloc_error = parser.allocate(json.size());
        if (alloc_error != simdjson::SUCCESS) {
            state.error_message = "Error allocating parser";
            state.is_loading = false;
            return;
        }
    }

    auto doc = parser.iterate(json);
    if (doc.error() != simdjson::SUCCESS) {
        state.error_message = "Error parsing JSON";
        state.is_loading = false;
        return;
    }

    state.documents_loaded = 1;
    state.status_message = "Parsed successfully";
    state.is_loading = false;
    state.is_complete = true;
}
