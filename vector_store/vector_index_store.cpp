#include <roaring/roaring64map.hh>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <boost/asio/post.hpp>
#include <boost/algorithm/string.hpp>
#include <fstream>
#include "vector_index_store.h"
#include "exceptions.h"
#include "logger.h"

template <typename T>
void write_binary_pod(std::ostream& out, const T& podRef) {
    out.write((char*)&podRef, sizeof(T));
}

template <typename T>
void read_binary_pod(std::istream& in, T& podRef) {
    in.read((char*)&podRef, sizeof(T));
}

void write_binary(std::ostream& out, const char*data, size_t size) {
    out.write(data, size);
}

void read_binary(std::istream& in, char* buffer, size_t size) {
    in.read(buffer, size);
}

struct IndexFileInfo {
    bool is_hnsw = false;
    std::string index;
    std::string deletes;
    std::string segment;
};

void VectorIndexStore::LoadIndex() {
    std::string meta_file;
    std::unordered_map<uint64_t, IndexFileInfo> files;
    for (const auto& entry : std::filesystem::directory_iterator(path_)) {
        std::string filename = entry.path().filename();
        if (filename == "meta.json") {
            meta_file = entry.path().string();
            continue;
        }
        std::vector<std::string> strs;
        boost::split(strs, filename, boost::is_any_of("."));
        if (strs.size() == 2) {
            uint64_t id = std::stoull(strs[0]);
            if (strs[1] == "hnsw") {
                files[id].is_hnsw = true;
                files[id].index = entry.path().string();
            } else if (strs[1] == "deletes") {
                files[id].deletes = entry.path().string();
            } else if (strs[1] == "segment") {
                files[id].segment = entry.path().string();
            }
        }
    }
    std::ifstream meta_fd(meta_file, std::ios::in);
    nlohmann::json meta;
    meta_fd >> meta;
    meta_fd.close();
    metaInfo_.applied_id = meta["applied_id"].get<uint64_t>();
    metaInfo_.ids = meta["ids"].get<std::vector<uint64_t>>();
    for (auto id : metaInfo_.ids) {
        next_index_id_ = std::max(next_index_id_, id);
        auto& info = files.at(id);
        if (info.is_hnsw) {
            // load hnsw index
            // [version][keys_size][key_len][key_data][index_len][index_data]...
            std::ifstream index_fd(info.index, std::ios::in);
            char version;
            size_t keys_size = 0;
            read_binary_pod(index_fd, version);
            read_binary_pod(index_fd, keys_size);
            vsag::BinarySet bs;
            for (size_t i = 0; i < keys_size; i++) {
                size_t key_len = 0;
                read_binary_pod(index_fd, key_len);
                std::string key(key_len, '\0');
                read_binary(index_fd, key.data(), key_len);
                vsag::Binary b;
                read_binary_pod(index_fd, b.size);
                b.data.reset(new int8_t[b.size]);
                read_binary(index_fd, (char*)b.data.get(), b.size);
                bs.Set(key, b);
            }
            index_fd.close();
            auto index = NewEmptyIndex(id);
            index->index()->Deserialize(bs);
            // load deletes
            std::ifstream deletes_fd(info.deletes, std::ios::in|std::ios::binary);
            deletes_fd.seekg(0, std::ios::end);
            size_t size = deletes_fd.tellg();
            deletes_fd.seekg(0, std::ios::beg);
            std::vector<char> buffer(size);
            read_binary(deletes_fd, buffer.data(), size);
            *index->deletes() = roaring::Roaring64Map::read(buffer.data(), false);
            immutable_indexes_.emplace_back(std::move(index));
        }
    }
    next_index_id_++;
}

VectorIndexStore::VectorIndexStore(const std::string& path, const IndexStoreOptions& options) {
    path_ = path;
    options_ = options;
    if (!std::filesystem::exists(path)) {
        std::filesystem::create_directories(path_);
        std::string meta_path = fmt::format("{}/meta.json", path_);
        std::ofstream meta_fd(meta_path, std::ios::binary);
        meta_fd << metaInfo_.Serialize();
        meta_fd.close();
    }
    LoadIndex();
    auto parameters = nlohmann::json::parse(options_.index_build_parameters);
    dim_ = parameters["dim"].get<int64_t>();
    mutable_index_ = NewEmptyIndex(next_index_id_++);
    threads_.emplace_back([this]() {
        pthread_setname_np(pthread_self(), "flush_service");
        boost::asio::io_service::work holder(flush_service_);
        flush_service_.run();
    });
}

VectorIndexStore::~VectorIndexStore() {
    flush_service_.stop();
    for (auto& t : threads_) {
        t.join();
    }
    threads_.clear();
}

void VectorIndexStore::Add(int64_t id, const float* vectors, uint64_t apply_id) {
    auto* ids = new int64_t[1];
    ids[0] = id;
    auto dataset = vsag::Dataset::Make();
    dataset->Dim(dim_)->NumElements(1)->Ids(ids)->Float32Vectors(vectors);
    mutable_index_->Add(dataset);
    if (mutable_index_->Size() >= 10000) {
        mutable_index_->Freeze(apply_id);
        immutable_deletes_[mutable_index_->index_id()] = mutable_index_->deletes();
        immutable_indexes_.push_back(mutable_index_);
        boost::asio::post(flush_service_, [this, index = mutable_index_](){
            Flush(index);
        });
        mutable_index_ = NewEmptyIndex(next_index_id_++);
    }
}

void VectorIndex::Add(const vsag::DatasetPtr& dataset) {
    std::unique_lock<std::shared_mutex> lock(rw_mutex_);
    auto result = index_->Add(dataset);
    if (result.has_value()) {
        if (!result.value().empty()) {
            THROW_CODE(VectorIndexException,
                       "failed to insert {} ids into vector index",
                       result.value().size());
        }
    } else {
        THROW_CODE(VectorIndexException, result.error().message);
    }
    vectors_.emplace_back(dataset->GetFloat32Vectors(), dataset->GetFloat32Vectors() + dataset->GetDim());
}

void VectorIndex::Delete(uint64_t vector_id) {
    std::unique_lock<std::shared_mutex> lock(rw_mutex_);
    deletes_->add(vector_id);
}

std::string MetaInfo::Serialize() {
    auto obj = nlohmann::json::object();
    obj["ids"] = ids;
    obj["applied_id"] = applied_id;
    return obj.dump(4);
}

std::shared_ptr<VectorIndex> VectorIndexStore::NewEmptyIndex(uint64_t index_id) {
    auto ret = vsag::Factory::CreateIndex("hnsw", options_.index_build_parameters);
    if (ret.has_value()) {
        return std::make_shared<VectorIndex>(index_id, std::move(ret.value()));
    } else {
        THROW_CODE(VectorIndexException, "Failed to create vector index");
    }
}

void VectorIndexStore::Flush(const std::shared_ptr<VectorIndex>& index) {
    // flush index
    std::string index_path = fmt::format("{}/{}.hnsw", path_, index->index_id());
    if (auto bs = index->index()->Serialize(); bs.has_value()) {
        std::ofstream index_fd(index_path, std::ios::binary);
        auto keys = bs->GetKeys();
        // [version][keys_size][key_len][key_data][index_len][index_data]...
        char version = 1;
        write_binary_pod(index_fd, version);
        write_binary_pod(index_fd, keys.size());
        for (auto& key : keys) {
            write_binary_pod(index_fd, key.size());
            write_binary(index_fd, key.data(), key.size());
            vsag::Binary b = bs->Get(key);
            write_binary_pod(index_fd, b.size);
            write_binary(index_fd, (const char*)b.data.get(), b.size);
        }
        index_fd.close();
    } else if (bs.error().type == vsag::ErrorType::NO_ENOUGH_MEMORY) {
        LOG_ERROR("no enough memory to serialize index {}", index_path);
        return;
    } else {
        LOG_ERROR("failed to serialize index {}", index_path);
        return;
    }

    // flush deletes
    std::string deletes_path = fmt::format("{}/{}.deletes", path_, index->index_id());
    auto size = index->deletes()->getSizeInBytes(false);
    std::vector<char> buffer(size);
    index->deletes()->write(buffer.data(), false);
    std::ofstream deletes_fd(deletes_path, std::ios::binary);
    if (!deletes_fd) {
        LOG_ERROR("failed to open {}", deletes_path);
        return;
    }
    write_binary(deletes_fd, buffer.data(), size);
    deletes_fd.close();

    // flush segment
    std::string segment_path = fmt::format("{}/{}.segment", path_, index->index_id());
    std::ofstream segment_fd(segment_path, std::ios::binary);
    for (auto& vector : index->vectors()) {
        write_binary(segment_fd, reinterpret_cast<const char *>(vector.data()), vector.size() * sizeof(float));
    }
    segment_fd.close();

    // update meta info
    metaInfo_.applied_id = index->apply_id();
    metaInfo_.ids.push_back(index->index_id());
    std::string meta_path = fmt::format("{}/meta.json", path_);
    std::string tmp_meta_path = fmt::format("{}/meta.json.tmp", path_);
    std::ofstream tmp_meta_fd(tmp_meta_path, std::ios::binary);
    tmp_meta_fd << metaInfo_.Serialize();
    tmp_meta_fd.close();
    std::filesystem::rename(tmp_meta_path, meta_path);
}

std::vector<std::pair<int64_t, float>> VectorIndexStore::KnnSearch(const float *query, int64_t top_k, int ef_search) {
    auto dataset = vsag::Dataset::Make();
    dataset->Dim(dim_)
            ->NumElements(1)
            ->Float32Vectors(query)
            ->Owner(false);
    nlohmann::json parameters {
            {"hnsw", {{"ef_search", ef_search}}},
    };
    auto param_str = parameters.dump();
    auto cmp = [](const std::pair<int64_t, float>& a,
                  const std::pair<int64_t, float>& b) {
        return a.second > b.second;
    };
    std::priority_queue<
            std::pair<int64_t, float>,
            std::vector<std::pair<int64_t, float>>,
            decltype(cmp)
    > merge(cmp);

    std::shared_lock<std::shared_mutex> read_lock(mutable_index_->rw_mutex());
    for (auto& index : immutable_indexes_) {
        auto result = index->index()->KnnSearch(dataset, top_k, param_str, [this](int64_t id)->bool {
            for (auto& [_, deletes] : immutable_deletes_) {
                if (deletes->contains((uint64_t)id)) {
                    return true;
                }
            }
            if (mutable_index_->deletes()->contains((uint64_t)id)) {
                return true;
            }
            return false;
        });
        if (result.has_value()) {
            for (int64_t i = 0; i < result.value()->GetDim(); i++) {
                merge.emplace(result.value()->GetIds()[i], result.value()->GetDistances()[i]);
            }
        } else {
            THROW_CODE(VectorIndexException, result.error().message);
        }
    }
    auto result = mutable_index_->index()->KnnSearch(dataset, top_k, param_str, [this](int64_t id)->bool {
        return mutable_index_->deletes()->contains(uint64_t(id));
    });
    read_lock.unlock();

    if (result.has_value()) {
        for (int64_t i = 0; i < result.value()->GetDim(); i++) {
            merge.emplace(result.value()->GetIds()[i], result.value()->GetDistances()[i]);
        }
    } else {
        THROW_CODE(VectorIndexException, result.error().message);
    }

    std::vector<std::pair<int64_t, float>> final;
    while (!merge.empty() && final.size() < (size_t)top_k) {
        final.emplace_back(merge.top());
        merge.pop();
    }
    return final;
}

void VectorIndexStore::Delete(uint64_t id) {
    mutable_index_->Delete(id);
}

int64_t VectorIndexStore::VectorNum() {
    int64_t num = 0;
    for (auto& index : immutable_indexes_) {
        num += index->index()->GetNumElements();
    }
    num += mutable_index_->index()->GetNumElements();
    return num;
}

uint64_t VectorIndexStore::DeleteNum() {
    uint64_t num = 0;
    for (auto& index : immutable_indexes_) {
        num += index->deletes()->cardinality();
    }
    num += mutable_index_->deletes()->cardinality();
    return num;
}