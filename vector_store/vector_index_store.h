#include <vsag/vsag.h>
#include <boost/asio/io_service.hpp>
#include <thread>
#include <shared_mutex>

class VectorIndex {
public:
    VectorIndex(int index_id, std::shared_ptr<vsag::Index> index)
        : index_id_(index_id), index_(std::move(index)) {
        deletes_ = std::make_shared<roaring::Roaring64Map>();
    }
    void Add(const vsag::DatasetPtr& dataset);
    void Delete(uint64_t vector_id);
    int64_t Size() {
        return index_->GetNumElements();
    }
    void Freeze(uint64_t apply_id) {
        apply_id_ = apply_id;
        immutable_ = true;
    }
    [[nodiscard]] int index_id() const {return index_id_;}
    [[nodiscard]] uint64_t apply_id() const {return apply_id_;}
    std::shared_ptr<vsag::Index>& index() {return index_;}
    std::shared_ptr<roaring::Roaring64Map>& deletes() {return deletes_;}
    std::vector<std::vector<float>>& vectors() {return vectors_;}
    std::shared_mutex& rw_mutex() {return rw_mutex_;}
private:
    int index_id_ = 0;
    bool immutable_ = false;
    uint64_t apply_id_ = 0;
    std::shared_ptr<vsag::Index> index_;
    std::shared_mutex rw_mutex_;
    std::shared_ptr<roaring::Roaring64Map> deletes_;
    std::vector<std::vector<float>> vectors_;
};

struct MetaInfo {
    std::vector<uint64_t> ids;
    uint64_t applied_id = 0;
    std::string Serialize();
};

struct IndexStoreOptions {
    int mutable_index_max_size = 10000;
    std::string index_build_parameters;
};

class VectorIndexStore {
public:
    VectorIndexStore(const std::string& path, const IndexStoreOptions& options);
    void Add(int64_t id, const float* vectors, uint64_t apply_id);
    void Delete(uint64_t id);
    std::vector<std::pair<int64_t, float>> KnnSearch(const float* query, int64_t top_k, int ef_search);
    int64_t VectorNum();
    uint64_t DeleteNum();
    ~VectorIndexStore();
private:
    void Flush(const std::shared_ptr<VectorIndex>& index);
    std::shared_ptr<VectorIndex> NewEmptyIndex(uint64_t index_id);
    void LoadIndex();
    std::string path_;
    IndexStoreOptions options_;
    int64_t dim_;
    boost::asio::io_service flush_service_;
    std::shared_ptr<VectorIndex> mutable_index_;
    std::vector<std::shared_ptr<VectorIndex>> immutable_indexes_;
    std::unordered_map<uint64_t, std::shared_ptr<roaring::Roaring64Map>> immutable_deletes_;
    std::vector<std::thread> threads_;
    uint64_t next_index_id_ = 0;
    MetaInfo metaInfo_;
};