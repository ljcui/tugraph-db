#include <vsag/vsag.h>
#include <boost/asio/io_service.hpp>
#include <thread>
#include <shared_mutex>
#include <utility>

class VectorIndex {
public:
    VectorIndex(uint64_t index_id, std::shared_ptr<vsag::Index> index)
            : index_id_(index_id), index_(std::move(index)) {
        deletes_ = std::make_shared<roaring::Roaring64Map>();
    }
    VectorIndex(uint64_t index_id, std::shared_ptr<vsag::Index> index, std::shared_ptr<roaring::Roaring64Map> deletes)
            : index_id_(index_id), index_(std::move(index)), deletes_(std::move(deletes)) {}
    VectorIndex(VectorIndex&& index) = default;
    std::shared_ptr<vsag::Index>& index() {return index_;}
    std::shared_ptr<roaring::Roaring64Map>& deletes() {return deletes_;}
    //std::vector<std::vector<float>>& vectors() {return vectors_;}
    [[nodiscard]] uint64_t index_id() const {return index_id_;}
    int64_t Size() {
        return index_->GetNumElements();
    }
private:
    uint64_t index_id_ = 0;
    std::shared_ptr<vsag::Index> index_;
    std::shared_ptr<roaring::Roaring64Map> deletes_;
};

class MutableVectorIndex : public VectorIndex {
public:
    MutableVectorIndex(uint64_t index_id, std::shared_ptr<vsag::Index> index)
        : VectorIndex(index_id, std::move(index)) {}
    MutableVectorIndex(MutableVectorIndex&& index) = default;
    void Add(const vsag::DatasetPtr& dataset);
    void Delete(uint64_t vector_id);
    const std::vector<int64_t>& ids() {return ids_;}
    const std::vector<std::vector<float>>& vectors() {return vectors_;}
private:
    std::vector<int64_t> ids_;
    std::vector<std::vector<float>> vectors_;
};

class ImmutableVectorIndex : public MutableVectorIndex {
public:
    ImmutableVectorIndex(uint64_t apply_id, MutableVectorIndex&& index)
        : MutableVectorIndex(std::move(index)), apply_id_(apply_id) {}
    [[nodiscard]] uint64_t apply_id() const {return apply_id_;}
private:
    uint64_t apply_id_ = 0;
};

class PersistentVectorIndex : public VectorIndex {
public:
    PersistentVectorIndex(ImmutableVectorIndex&& index, std::string  index_path,
                          std::string deletes_path, std::string segment_path)
        : VectorIndex(std::move(index)), index_path_(std::move(index_path)),
        deletes_path_(std::move(deletes_path)), segment_path_(std::move(segment_path)) {}
    PersistentVectorIndex(uint64_t index_id,
                          std::shared_ptr<vsag::Index> index,
                          std::shared_ptr<roaring::Roaring64Map> deletes,
                          std::string index_path,
                          std::string deletes_path,
                          std::string segment_path)
        : VectorIndex(index_id, std::move(index), std::move(deletes)),
        index_path_(std::move(index_path)),
        deletes_path_(std::move(deletes_path)),
        segment_path_(std::move(segment_path)) {}
    const std::string& segment_path() {return segment_path_;}
    const std::string& index_path() {return index_path_;}
    const std::string& deletes_path() {return deletes_path_;}
private:
    std::string index_path_;
    std::string deletes_path_;
    std::string segment_path_;
};

/*class VectorIndex {
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
    void SetPersistent() {
        persistent_ = true;
    }
    [[nodiscard]] bool persistent() const {return persistent_;}
    [[nodiscard]] int index_id() const {return index_id_;}
    [[nodiscard]] uint64_t apply_id() const {return apply_id_;}
    std::shared_ptr<vsag::Index>& index() {return index_;}
    std::shared_ptr<roaring::Roaring64Map>& deletes() {return deletes_;}
    std::vector<std::vector<float>>& vectors() {return vectors_;}
private:
    int index_id_ = 0;
    bool immutable_ = false;
    bool persistent_ = false;
    uint64_t apply_id_ = 0;
    std::shared_ptr<vsag::Index> index_;
    std::shared_ptr<roaring::Roaring64Map> deletes_;
    std::vector<std::vector<float>> vectors_;
};*/

struct MetaInfo {
    std::set<uint64_t> ids;
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
    void Flush(const std::shared_ptr<ImmutableVectorIndex>& index);
    void MaybeMergeIndex();
    std::shared_ptr<MutableVectorIndex> NewEmptyMutableVectorIndex(uint64_t index_id) const;
    void LoadIndex();
    std::string path_;
    IndexStoreOptions options_;
    int64_t dim_;
    boost::asio::io_service flush_service_;
    boost::asio::io_service merge_service_;
    std::shared_ptr<MutableVectorIndex> mutable_index_;
    std::unordered_map<uint64_t, std::shared_ptr<ImmutableVectorIndex>> immutable_indexes_;
    std::unordered_map<uint64_t, std::shared_ptr<PersistentVectorIndex>> persistent_indexes_;
    std::unordered_map<uint64_t, std::shared_ptr<roaring::Roaring64Map>> immutable_deletes_;
    std::vector<std::thread> threads_;
    uint64_t next_index_id_ = 0;
    MetaInfo metaInfo_;
    std::shared_mutex rw_mutex_;
};