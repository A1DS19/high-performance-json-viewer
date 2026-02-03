#include "utils/json_data_store.hpp"

JsonDataStore& JsonDataStore::instance() {
    static JsonDataStore store;
    return store;
}

JsonDataStore& get_json_data() {
    return JsonDataStore::instance();
}

void JsonDataStore::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    raw_data_.reset();
    index_.clear();
    cache_list_.clear();
    cache_map_.clear();
    is_ready_ = false;
    generation_++;
}

void JsonDataStore::set_raw_data(simdjson::padded_string&& data) {
    std::lock_guard<std::mutex> lock(mutex_);
    raw_data_ = std::make_unique<simdjson::padded_string>(std::move(data));
}

void JsonDataStore::add_document_index(size_t offset, size_t length) {
    std::lock_guard<std::mutex> lock(mutex_);
    index_.push_back({offset, length});
}

void JsonDataStore::set_complete() {
    is_ready_ = true;
}

size_t JsonDataStore::document_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return index_.size();
}

bool JsonDataStore::is_ready() const {
    return is_ready_.load();
}

size_t JsonDataStore::generation() const {
    return generation_.load();
}

std::string JsonDataStore::get_document(size_t index) {
    std::lock_guard<std::mutex> lock(mutex_);
    return get_cached_or_parse(index);
}

std::string JsonDataStore::get_cached_or_parse(size_t index) const {
    // Check cache first
    auto cache_iter = cache_map_.find(index);
    if (cache_iter != cache_map_.end()) {
        // Move to front (most recently used)
        cache_list_.splice(cache_list_.begin(), cache_list_, cache_iter->second);
        return cache_iter->second->second;
    }

    // Not in cache, parse from raw data
    if (index >= index_.size() || !raw_data_) {
        return "";
    }

    const DocumentIndex& doc_idx = index_.at(index);
    std::string doc(raw_data_->data() + doc_idx.byte_offset_, doc_idx.byte_length_);

    // Add to cache
    add_to_cache(index, doc);

    return doc;
}

void JsonDataStore::add_to_cache(size_t index, const std::string& doc) const {
    // Evict oldest if cache is full
    if (cache_list_.size() >= CACHE_SIZE) {
        size_t oldest_index = cache_list_.back().first;
        cache_map_.erase(oldest_index);
        cache_list_.pop_back();
    }

    // Add to front
    cache_list_.emplace_front(index, doc);
    cache_map_[index] = cache_list_.begin();
}
