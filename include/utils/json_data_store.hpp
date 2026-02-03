#pragma once

#include <atomic>
#include <cstddef>
#include <list>
#include <memory>
#include <mutex>
#include <simdjson.h>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

struct DocumentIndex {
    size_t byte_offset_; // Start position in raw data
    size_t byte_length_; // Length of this document
};

class JsonDataStore {
public:
    static JsonDataStore& instance();

    // Called by parser
    void reset();
    void set_raw_data(simdjson::padded_string&& data);
    void add_document_index(size_t offset, size_t length);
    void set_complete();

    // Called by viewer panel
    size_t document_count() const;
    std::string get_document(size_t index); // On-demand parsing
    bool is_ready() const;
    size_t generation() const; // Increments on each reset

private:
    JsonDataStore() = default;

    std::unique_ptr<simdjson::padded_string> raw_data_;
    std::vector<DocumentIndex> index_;
    std::atomic<bool> is_ready_{false};
    std::atomic<size_t> generation_{0};
    mutable std::mutex mutex_;

    // LRU cache for recently accessed documents
    static constexpr size_t CACHE_SIZE = 100;
    mutable std::list<std::pair<size_t, std::string>> cache_list_;
    mutable std::unordered_map<size_t, decltype(cache_list_)::iterator> cache_map_;

    std::string get_cached_or_parse(size_t index) const;
    void add_to_cache(size_t index, const std::string& doc) const;
};

JsonDataStore& get_json_data();
