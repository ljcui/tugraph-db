/**
 * Copyright 2024 AntGroup CO., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */

//
// Created by botu.wzy
//
#include "graph_db.h"

#include <filesystem>
#include <future>
#include <string_view>

#include "common/byte_utils.h"
#include "common/logger.h"
#include "meta_info.h"
#include "proto/meta.pb.h"
#include "transaction/transaction.h"
namespace fs = std::filesystem;
using namespace boost::endian;
using common::AsChars;
using common::ReadValue;
namespace graphdb {
namespace {

void ThrowIfIteratorError(rocksdb::Iterator* iter, std::string_view action);

std::string BuildFullTextIndexPath(const std::string& graph_path,
                                   const std::string& index_name,
                                   uint32_t index_id) {
  return graph_path + "/ft/" + index_name + "_" +
         std::to_string(big_to_native(index_id));
}

std::string BuildMetaKey(MetaDataType type, const std::string& name) {
  std::string key;
  key.append(1, static_cast<char>(type));
  key.append(name);
  return key;
}

const std::string kRaftApplyIndexKey(
    1, static_cast<char>(MetaDataType::RaftApplyIndex));

uint64_t LoadVisibleMaxWalId(rocksdb::TransactionDB* db, GraphCF* graph_cf,
                             uint32_t index_id,
                             const rocksdb::Snapshot* snapshot) {
  std::string prefix(AsChars(index_id), sizeof(index_id));
  std::string seek_key(prefix);
  seek_key.append(sizeof(uint64_t), static_cast<char>(0xFF));
  rocksdb::ReadOptions ro;
  ro.snapshot = snapshot;
  std::unique_ptr<rocksdb::Iterator> iter(db->NewIterator(ro, graph_cf->wal));
  iter->SeekForPrev(seek_key);
  if (!iter->Valid()) {
    ThrowIfIteratorError(iter.get(),
                         "index wal iterator failed while loading max wal id");
    return 0;
  }
  auto key = iter->key();
  if (!key.starts_with(prefix)) {
    ThrowIfIteratorError(iter.get(),
                         "index wal iterator failed while loading max wal id");
    return 0;
  }
  key.remove_prefix(sizeof(index_id));
  if (key.size() != sizeof(uint64_t)) {
    THROW_CODE(StorageEngineError,
               "index wal key has invalid size while loading max wal id, "
               "expect {}, actual {}",
               sizeof(uint64_t), key.size());
  }
  ThrowIfIteratorError(iter.get(),
                       "index wal iterator failed while loading max wal id");
  return big_to_native(ReadValue<uint64_t>(key.data()));
}

void DeletePropertyIndexRanges(rocksdb::TransactionDB* db, GraphCF* graph_cf,
                               uint32_t index_id) {
  rocksdb::WriteBatch wb;
  std::string start_key(AsChars(index_id), sizeof(index_id));
  std::string end_key(AsChars(index_id), sizeof(index_id));
  end_key.append(128, static_cast<char>(0xFF));
  wb.DeleteRange(graph_cf->index, start_key, end_key);

  std::string wal_start(AsChars(index_id), sizeof(index_id));
  std::string wal_end(AsChars(index_id), sizeof(index_id));
  wal_end.append(sizeof(uint64_t), static_cast<char>(0xFF));
  wb.DeleteRange(graph_cf->wal, wal_start, wal_end);

  rocksdb::TransactionDBWriteOptimizations two;
  two.skip_concurrency_control = true;
  two.skip_duplicate_key_check = true;
  auto s = db->Write({}, two, &wb);
  if (!s.ok()) THROW_CODE(StorageEngineError, s.ToString());
}

void DeletePropertyIndexWalRange(rocksdb::TransactionDB* db, GraphCF* graph_cf,
                                 uint32_t index_id) {
  rocksdb::WriteBatch wb;
  std::string wal_start(AsChars(index_id), sizeof(index_id));
  std::string wal_end(AsChars(index_id), sizeof(index_id));
  wal_end.append(sizeof(uint64_t), static_cast<char>(0xFF));
  wb.DeleteRange(graph_cf->wal, wal_start, wal_end);
  rocksdb::TransactionDBWriteOptimizations two;
  two.skip_concurrency_control = true;
  two.skip_duplicate_key_check = true;
  auto s = db->Write({}, two, &wb);
  if (!s.ok()) THROW_CODE(StorageEngineError, s.ToString());
}

void DeleteFullTextIndexRanges(rocksdb::TransactionDB* db, GraphCF* graph_cf,
                               uint32_t index_id) {
  std::string start_key(AsChars(index_id), sizeof(index_id));
  start_key.append(sizeof(int64_t), static_cast<char>(0x00));
  std::string end_key(AsChars(index_id), sizeof(index_id));
  end_key.append(sizeof(int64_t), static_cast<char>(0xFF));

  rocksdb::WriteBatch wb;
  wb.DeleteRange(graph_cf->index, start_key, end_key);
  wb.DeleteRange(graph_cf->wal, start_key, end_key);

  rocksdb::WriteOptions wo;
  rocksdb::TransactionDBWriteOptimizations two;
  two.skip_concurrency_control = true;
  two.skip_duplicate_key_check = true;
  auto s = db->Write(wo, two, &wb);
  if (!s.ok()) THROW_CODE(StorageEngineError, s.ToString());
}

void DeleteVectorIndexRanges(rocksdb::TransactionDB* db, GraphCF* graph_cf,
                             uint32_t index_id) {
  rocksdb::WriteBatch wb;
  std::string wal_start(AsChars(index_id), sizeof(index_id));
  std::string wal_end(AsChars(index_id), sizeof(index_id));
  wal_end.append(sizeof(uint64_t), static_cast<char>(0xFF));
  wb.DeleteRange(graph_cf->wal, wal_start, wal_end);

  rocksdb::TransactionDBWriteOptimizations two;
  two.skip_concurrency_control = true;
  two.skip_duplicate_key_check = true;
  auto s = db->Write({}, two, &wb);
  if (!s.ok()) THROW_CODE(StorageEngineError, s.ToString());
}

void ResetFullTextIndexPath(const std::string& path) {
  std::error_code ec;
  fs::remove_all(path, ec);
  if (ec) {
    THROW_CODE(StorageEngineError,
               "failed to remove stale fulltext index directory {}: {}", path,
               ec.message());
  }
  fs::create_directories(path, ec);
  if (ec) {
    THROW_CODE(StorageEngineError,
               "failed to create fulltext index directory {}: {}", path,
               ec.message());
  }
}

void ResetIndexPath(const std::string& path, const std::string& kind) {
  std::error_code ec;
  fs::remove_all(path, ec);
  if (ec) {
    THROW_CODE(StorageEngineError, "failed to remove stale {} directory {}: {}",
               kind, path, ec.message());
  }
  fs::create_directories(path, ec);
  if (ec) {
    THROW_CODE(StorageEngineError, "failed to create {} directory {}: {}", kind,
               path, ec.message());
  }
}

void DeleteAllEntriesInColumnFamily(rocksdb::TransactionDB* db,
                                    rocksdb::ColumnFamilyHandle* cf,
                                    rocksdb::WriteBatch* wb) {
  rocksdb::ReadOptions ro;
  std::unique_ptr<rocksdb::Iterator> iter(db->NewIterator(ro, cf));
  iter->SeekToFirst();
  if (!iter->Valid()) {
    return;
  }
  std::string begin = iter->key().ToString();
  iter->SeekToLast();
  if (!iter->Valid()) {
    return;
  }
  std::string end = iter->key().ToString();
  end.push_back('\0');
  wb->DeleteRange(cf, begin, end);
}

void ThrowIfIteratorError(rocksdb::Iterator* iter, std::string_view action) {
  auto status = iter->status();
  if (!status.ok()) {
    THROW_CODE(StorageEngineError, "{}: {}", action, status.ToString());
  }
}

}  // namespace

std::unique_ptr<GraphDB> GraphDB::Open(const std::string& path,
                                       const GraphDBOptions& graph_options) {
  std::string rocksdb_path = path + "/data";
  std::filesystem::create_directories(rocksdb_path);
  rocksdb::Options options;
  options.create_if_missing = true;
  options.create_missing_column_families = true;
  options.enable_pipelined_write = true;
  rocksdb::BlockBasedTableOptions table_options;
  table_options.cache_index_and_filter_blocks = true;
  if (graph_options.block_cache) {
    table_options.block_cache = graph_options.block_cache;
  } else {
    table_options.block_cache = rocksdb::NewLRUCache(1 * 1024 * 1024 * 1024L);
  }
  table_options.data_block_index_type =
      rocksdb::BlockBasedTableOptions::kDataBlockBinaryAndHash;
  table_options.partition_filters = true;
  table_options.index_type =
      rocksdb::BlockBasedTableOptions::IndexType::kTwoLevelIndexSearch;
  options.table_factory.reset(
      rocksdb::NewBlockBasedTableFactory(table_options));
  options.IncreaseParallelism();
  options.OptimizeLevelStyleCompaction();
  std::vector<std::string> built_in_cfs = {rocksdb::kDefaultColumnFamilyName,
                                           "graph_topology",
                                           "vertex_property",
                                           "edge_property",
                                           "vertex_label_vid",
                                           "edge_type_eid",
                                           "meta_info",
                                           "index",
                                           "wal"};
  std::vector<rocksdb::ColumnFamilyDescriptor> cfs;
  cfs.reserve(built_in_cfs.size());
  for (const auto& name : built_in_cfs) {
    cfs.emplace_back(name, options);
  }
  std::vector<rocksdb::ColumnFamilyHandle*> cf_handles;
  rocksdb::TransactionDBOptions txn_db_options;
  rocksdb::TransactionDB* db;
  auto s = rocksdb::TransactionDB::Open(options, txn_db_options, rocksdb_path,
                                        cfs, &cf_handles, &db);
  if (!s.ok()) THROW_CODE(StorageEngineError, s.ToString());
  auto graph_db = std::make_unique<GraphDB>();
  graph_db->db_ = db;
  graph_db->path_ = path;
  graph_db->graph_cf_.graph_topology = cf_handles[1];
  graph_db->graph_cf_.vertex_property = cf_handles[2];
  graph_db->graph_cf_.edge_property = cf_handles[3];
  graph_db->graph_cf_.vertex_label_vid = cf_handles[4];
  graph_db->graph_cf_.edge_type_eid = cf_handles[5];
  graph_db->graph_cf_.meta_info = cf_handles[6];
  graph_db->graph_cf_.index = cf_handles[7];
  graph_db->graph_cf_.wal = cf_handles[8];
  graph_db->cf_handles_ = std::move(cf_handles);
  graph_db->options_ = graph_options;
  auto* self = graph_db.get();
  graph_db->service_threads_.emplace_back([self]() {
    pthread_setname_np(pthread_self(), "assistant");
    boost::asio::io_service::work holder(self->assistant_);
    self->assistant_.run();
  });
  graph_db->meta_info_.Init(graph_db->db_, graph_db->assistant_,
                            &graph_db->graph_cf_,
                            graph_db->options_.ft_apply_interval_,
                            graph_db->options_.ft_writer_threads_,
                            graph_db->options_.ft_writer_memory_budget_,
                            graph_db->options_.vt_apply_interval_);
  graph_db->ResumeBackgroundIndexBuilds();

  return graph_db;
}

GraphDB::~GraphDB() {
  LOG_INFO("Close graph: {}", db_meta_.graph_name());
  {
    std::unique_lock<std::shared_mutex> lock(raft_mutex_);
    if (raft_driver_) {
      raft_driver_->Stop();
      raft_driver_.reset();
    }
  }
  for (const auto& index : meta_info_.GetVertexVectorIndexes()) {
    index->Stop();
  }
  for (const auto& index : meta_info_.GetVertexFullTextIndexes()) {
    index->Stop();
  }
  assistant_.stop();
  for (auto& t : service_threads_) {
    t.join();
  }
  meta_info_.ClearVertexVectorIndexes();
  meta_info_.ClearVertexFullTextIndexes();
  for (auto handle : cf_handles_) {
    auto s = db_->DestroyColumnFamilyHandle(handle);
    assert(s.ok());
  }
  auto s = db_->Close();
  if (!s.ok()) {
    LOG_WARN("graph db close error : {}", s.ToString());
  }
  delete db_;
  db_ = nullptr;
  if (drop_on_close_) {
    std::filesystem::remove_all(path_);
    LOG_INFO("filesystem remove_all {}", path_);
  }
}

std::unique_ptr<txn::Transaction> GraphDB::BeginTransaction() {
  rocksdb::WriteOptions wo;
  rocksdb::TransactionOptions to;
  rocksdb::Transaction* txn = db_->BeginTransaction(wo, to);
  return std::make_unique<txn::Transaction>(txn, this);
}

raft::RaftDriver* GraphDB::raft_driver() const {
  std::shared_lock<std::shared_mutex> lock(raft_mutex_);
  return raft_driver_.get();
}

void GraphDB::SetRaftDriver(std::unique_ptr<raft::RaftDriver> raft_driver) {
  std::unique_lock<std::shared_mutex> lock(raft_mutex_);
  raft_driver_ = std::move(raft_driver);
}

uint64_t GraphDB::GetRaftApplyIndex() const {
  std::string val;
  auto s = db_->Get({}, graph_cf_.meta_info, kRaftApplyIndexKey, &val);
  if (s.IsNotFound()) {
    return 0;
  }
  if (!s.ok()) {
    THROW_CODE(StorageEngineError, "failed to load raft apply index: {}",
               s.ToString());
  }
  if (val.size() != sizeof(uint64_t)) {
    THROW_CODE(StorageEngineError,
               "raft apply index has invalid size, expect {}, actual {}",
               sizeof(uint64_t), val.size());
  }
  return ReadValue<uint64_t>(val.data());
}

rocksdb::Status GraphDB::SetRaftApplyIndex(uint64_t apply_index,
                                           rocksdb::WriteBatch* wb) const {
  return wb->Put(graph_cf_.meta_info, kRaftApplyIndexKey,
                 std::string(AsChars(apply_index), sizeof(apply_index)));
}

void GraphDB::PersistVertexPropertyIndexMeta(
    const std::shared_ptr<VertexPropertyIndex>& index) {
  auto s = db_->Put(
      {}, graph_cf_.meta_info,
      BuildMetaKey(MetaDataType::VertexPropertyIndex, index->meta().name()),
      index->meta().SerializeAsString());
  if (!s.ok()) THROW_CODE(StorageEngineError, s.ToString());
}

void GraphDB::PersistVertexFullTextIndexMeta(
    const std::shared_ptr<VertexFullTextIndex>& index) {
  auto s = db_->Put(
      {}, graph_cf_.meta_info,
      BuildMetaKey(MetaDataType::VertexFullTextIndex, index->meta().name()),
      index->meta().SerializeAsString());
  if (!s.ok()) THROW_CODE(StorageEngineError, s.ToString());
}

void GraphDB::PersistVertexVectorIndexMeta(
    const std::shared_ptr<VertexVectorIndex>& index) {
  auto s = db_->Put(
      {}, graph_cf_.meta_info,
      BuildMetaKey(MetaDataType::VertexVectorIndex, index->meta().name()),
      index->meta().SerializeAsString());
  if (!s.ok()) THROW_CODE(StorageEngineError, s.ToString());
}

void GraphDB::ResumeBackgroundIndexBuilds() {
  for (const auto& index : meta_info_.GetBuildingVertexPropertyIndexes()) {
    ScheduleVertexPropertyIndexBuild(index, true);
  }
  for (const auto& index : meta_info_.GetBuildingVertexFullTextIndexes()) {
    ScheduleVertexFullTextIndexBuild(index, true);
  }
  for (const auto& index : meta_info_.GetBuildingVertexVectorIndexes()) {
    ScheduleVertexVectorIndexBuild(index, true);
  }
}

void GraphDB::DrainAssistant() {
  std::promise<void> drained;
  auto future = drained.get_future();
  boost::asio::post(assistant_, [&drained]() mutable { drained.set_value(); });
  future.wait();
}

void GraphDB::ScheduleVertexPropertyIndexBuild(
    const std::shared_ptr<VertexPropertyIndex>& index, bool reset_existing) {
  boost::asio::post(assistant_, [this, index, reset_existing]() {
    const rocksdb::Snapshot* snapshot = nullptr;
    try {
      if (reset_existing) {
        std::lock_guard<std::mutex> ddl_lock(index_ddl_mutex_);
        std::lock_guard<std::mutex> commit_lock(property_index_commit_mutex_);
        if (index->IsDeleted()) {
          return;
        }
        DeletePropertyIndexRanges(db_, &graph_cf_, index->index_id());
        index->ResetForBuild();
        index->SetState(meta::IndexBuildState::BUILDING);
        PersistVertexPropertyIndexMeta(index);
      }

      snapshot = db_->GetSnapshot();
      uint64_t snapshot_wal_id =
          LoadVisibleMaxWalId(db_, &graph_cf_, index->index_id(), snapshot);
      index->meta().set_build_start_wal_id(snapshot_wal_id + 1);
      index->Load(snapshot, snapshot_wal_id);
      db_->ReleaseSnapshot(snapshot);
      snapshot = nullptr;

      {
        std::lock_guard<std::mutex> ddl_lock(index_ddl_mutex_);
        if (index->IsDeleted()) {
          return;
        }
        index->SetState(meta::IndexBuildState::CATCHING_UP);
        index->SetBuildError("");
        PersistVertexPropertyIndexMeta(index);
      }

      index->ApplyWAL();

      {
        std::lock_guard<std::mutex> ddl_lock(index_ddl_mutex_);
        std::lock_guard<std::mutex> commit_lock(property_index_commit_mutex_);
        if (index->IsDeleted()) {
          return;
        }
        index->ApplyWAL();
        DeletePropertyIndexWalRange(db_, &graph_cf_, index->index_id());
        index->SetState(meta::IndexBuildState::READY);
        index->SetBuildError("");
        PersistVertexPropertyIndexMeta(index);
        meta_info_.PublishVertexPropertyIndex(index->Name());
      }
    } catch (const std::exception& e) {
      if (snapshot) {
        db_->ReleaseSnapshot(snapshot);
      }
      LOG_ERROR("vertex property index [{}] build failed: {}", index->Name(),
                e.what());
      std::lock_guard<std::mutex> ddl_lock(index_ddl_mutex_);
      if (!index->IsDeleted()) {
        index->SetState(meta::IndexBuildState::FAILED);
        index->SetBuildError(e.what());
        PersistVertexPropertyIndexMeta(index);
      }
    } catch (...) {
      if (snapshot) {
        db_->ReleaseSnapshot(snapshot);
      }
      LOG_ERROR("vertex property index [{}] build failed with unknown error",
                index->Name());
      std::lock_guard<std::mutex> ddl_lock(index_ddl_mutex_);
      if (!index->IsDeleted()) {
        index->SetState(meta::IndexBuildState::FAILED);
        index->SetBuildError("unknown error");
        PersistVertexPropertyIndexMeta(index);
      }
    }
  });
}

void GraphDB::ScheduleVertexFullTextIndexBuild(
    const std::shared_ptr<VertexFullTextIndex>& index, bool reset_existing) {
  boost::asio::post(assistant_, [this, index, reset_existing]() {
    const rocksdb::Snapshot* snapshot = nullptr;
    try {
      if (reset_existing) {
        std::lock_guard<std::mutex> ddl_lock(index_ddl_mutex_);
        std::lock_guard<std::mutex> commit_lock(fulltext_index_commit_mutex_);
        if (index->IsDeleted()) {
          return;
        }
        DeleteFullTextIndexRanges(db_, &graph_cf_, index->index_id());
        index->ReleaseResources();
        ResetFullTextIndexPath(index->meta().path());
        index->ResetForClear();
        index->SetState(meta::IndexBuildState::BUILDING);
        PersistVertexFullTextIndexMeta(index);
      }

      snapshot = db_->GetSnapshot();
      uint64_t snapshot_wal_id =
          LoadVisibleMaxWalId(db_, &graph_cf_, index->index_id(), snapshot);
      index->meta().set_build_start_wal_id(snapshot_wal_id + 1);
      index->Load(snapshot, snapshot_wal_id);
      db_->ReleaseSnapshot(snapshot);
      snapshot = nullptr;

      {
        std::lock_guard<std::mutex> ddl_lock(index_ddl_mutex_);
        if (index->IsDeleted()) {
          return;
        }
        index->SetState(meta::IndexBuildState::CATCHING_UP);
        index->SetBuildError("");
        PersistVertexFullTextIndexMeta(index);
      }

      index->ApplyWAL();

      {
        std::lock_guard<std::mutex> ddl_lock(index_ddl_mutex_);
        std::lock_guard<std::mutex> commit_lock(fulltext_index_commit_mutex_);
        if (index->IsDeleted()) {
          return;
        }
        index->ApplyWAL();
        index->SetState(meta::IndexBuildState::READY);
        index->SetBuildError("");
        PersistVertexFullTextIndexMeta(index);
        meta_info_.PublishVertexFullTextIndex(index->Name());
        index->Start();
      }
    } catch (const std::exception& e) {
      if (snapshot) {
        db_->ReleaseSnapshot(snapshot);
      }
      LOG_ERROR("vertex fulltext index [{}] build failed: {}", index->Name(),
                e.what());
      std::lock_guard<std::mutex> ddl_lock(index_ddl_mutex_);
      if (!index->IsDeleted()) {
        index->SetState(meta::IndexBuildState::FAILED);
        index->SetBuildError(e.what());
        PersistVertexFullTextIndexMeta(index);
      }
    } catch (...) {
      if (snapshot) {
        db_->ReleaseSnapshot(snapshot);
      }
      LOG_ERROR("vertex fulltext index [{}] build failed with unknown error",
                index->Name());
      std::lock_guard<std::mutex> ddl_lock(index_ddl_mutex_);
      if (!index->IsDeleted()) {
        index->SetState(meta::IndexBuildState::FAILED);
        index->SetBuildError("unknown error");
        PersistVertexFullTextIndexMeta(index);
      }
    }
  });
}

void GraphDB::ScheduleVertexVectorIndexBuild(
    const std::shared_ptr<VertexVectorIndex>& index, bool reset_existing) {
  boost::asio::post(assistant_, [this, index, reset_existing]() {
    const rocksdb::Snapshot* snapshot = nullptr;
    try {
      if (reset_existing) {
        std::lock_guard<std::mutex> ddl_lock(index_ddl_mutex_);
        std::lock_guard<std::mutex> commit_lock(vector_index_commit_mutex_);
        if (index->IsDeleted()) {
          return;
        }
        DeleteVectorIndexRanges(db_, &graph_cf_, index->index_id());
        index->ReleaseResources();
        ResetIndexPath(index->meta().path(), "vector index");
        index->ResetForClear();
        index->SetState(meta::IndexBuildState::BUILDING);
        PersistVertexVectorIndexMeta(index);
      }

      snapshot = db_->GetSnapshot();
      uint64_t snapshot_wal_id =
          LoadVisibleMaxWalId(db_, &graph_cf_, index->index_id(), snapshot);
      index->meta().set_build_start_wal_id(snapshot_wal_id + 1);
      index->Load(snapshot, snapshot_wal_id);
      db_->ReleaseSnapshot(snapshot);
      snapshot = nullptr;

      {
        std::lock_guard<std::mutex> ddl_lock(index_ddl_mutex_);
        if (index->IsDeleted()) {
          return;
        }
        index->SetState(meta::IndexBuildState::CATCHING_UP);
        index->SetBuildError("");
        PersistVertexVectorIndexMeta(index);
      }

      index->ApplyWAL();

      {
        std::lock_guard<std::mutex> ddl_lock(index_ddl_mutex_);
        std::lock_guard<std::mutex> commit_lock(vector_index_commit_mutex_);
        if (index->IsDeleted()) {
          return;
        }
        index->ApplyWAL();
        index->SetState(meta::IndexBuildState::READY);
        index->SetBuildError("");
        PersistVertexVectorIndexMeta(index);
        meta_info_.PublishVertexVectorIndex(index->meta().name());
        index->Start();
      }
    } catch (const std::exception& e) {
      if (snapshot) {
        db_->ReleaseSnapshot(snapshot);
      }
      LOG_ERROR("vertex vector index [{}] build failed: {}",
                index->meta().name(), e.what());
      std::lock_guard<std::mutex> ddl_lock(index_ddl_mutex_);
      if (!index->IsDeleted()) {
        index->SetState(meta::IndexBuildState::FAILED);
        index->SetBuildError(e.what());
        PersistVertexVectorIndexMeta(index);
      }
    } catch (...) {
      if (snapshot) {
        db_->ReleaseSnapshot(snapshot);
      }
      LOG_ERROR("vertex vector index [{}] build failed with unknown error",
                index->meta().name());
      std::lock_guard<std::mutex> ddl_lock(index_ddl_mutex_);
      if (!index->IsDeleted()) {
        index->SetState(meta::IndexBuildState::FAILED);
        index->SetBuildError("unknown error");
        PersistVertexVectorIndexMeta(index);
      }
    }
  });
}

void GraphDB::ClearData() {
  std::lock_guard<std::mutex> clear_lock(clear_data_mutex_);
  std::vector<std::shared_ptr<VertexFullTextIndex>> ft_indexes;
  std::vector<std::shared_ptr<VertexVectorIndex>> vector_indexes;
  {
    std::lock_guard<std::mutex> ddl_lock(index_ddl_mutex_);
    ft_indexes = meta_info_.GetVertexFullTextIndexes();
    vector_indexes = meta_info_.GetVertexVectorIndexes();
  }

  for (const auto& index : ft_indexes) {
    index->Stop();
  }
  for (const auto& index : vector_indexes) {
    index->Stop();
  }
  DrainAssistant();

  std::lock_guard<std::mutex> ddl_lock(index_ddl_mutex_);
  std::unique_lock<std::mutex> property_commit_lock(
      property_index_commit_mutex_, std::defer_lock);
  std::unique_lock<std::mutex> fulltext_commit_lock(
      fulltext_index_commit_mutex_, std::defer_lock);
  std::unique_lock<std::mutex> vector_commit_lock(vector_index_commit_mutex_,
                                                  std::defer_lock);
  std::lock(property_commit_lock, fulltext_commit_lock, vector_commit_lock);

  auto property_indexes = meta_info_.GetVertexPropertyIndexes();
  ft_indexes = meta_info_.GetVertexFullTextIndexes();
  vector_indexes = meta_info_.GetVertexVectorIndexes();

  rocksdb::WriteBatch wb;
  DeleteAllEntriesInColumnFamily(db_, graph_cf_.graph_topology, &wb);
  DeleteAllEntriesInColumnFamily(db_, graph_cf_.vertex_property, &wb);
  DeleteAllEntriesInColumnFamily(db_, graph_cf_.edge_property, &wb);
  DeleteAllEntriesInColumnFamily(db_, graph_cf_.vertex_label_vid, &wb);
  DeleteAllEntriesInColumnFamily(db_, graph_cf_.edge_type_eid, &wb);
  DeleteAllEntriesInColumnFamily(db_, graph_cf_.index, &wb);
  DeleteAllEntriesInColumnFamily(db_, graph_cf_.wal, &wb);

  rocksdb::WriteOptions wo;
  rocksdb::TransactionDBWriteOptimizations two;
  two.skip_concurrency_control = true;
  two.skip_duplicate_key_check = true;
  auto s = db_->Write(wo, two, &wb);
  if (!s.ok()) THROW_CODE(StorageEngineError, s.ToString());

  for (const auto& index : property_indexes) {
    index->ResetForBuild();
  }
  for (const auto& index : ft_indexes) {
    index->ReleaseResources();
    ResetFullTextIndexPath(index->meta().path());
    index->ResetForClear();
    if (index->IsReady()) {
      index->Start();
    }
  }
  for (const auto& index : vector_indexes) {
    index->ReleaseResources();
    ResetIndexPath(index->meta().path(), "vector index");
    index->ResetForClear();
    if (index->IsReady()) {
      index->Start();
    }
  }
}

void GraphDB::AddVertexPropertyIndex(
    const std::string& index_name, bool unique, const std::string& label,
    const std::vector<std::string>& properties) {
  std::lock_guard<std::mutex> clear_lock(clear_data_mutex_);
  std::lock_guard<std::mutex> ddl_lock(index_ddl_mutex_);
  if (index_name.empty() || label.empty() || properties.empty()) {
    THROW_CODE(InvalidParameter);
  }
  if (meta_info_.GetVertexPropertyIndex(index_name)) {
    THROW_CODE(VertexIndexAlreadyExist, "Vertex index name {} already exists",
               index_name);
  }
  auto lid = id_generator().GetOrCreateLid(label);
  std::vector<uint32_t> pids;
  pids.reserve(properties.size());
  std::unordered_set<uint32_t> pid_set;
  for (const auto& property : properties) {
    if (property.empty()) {
      THROW_CODE(InvalidParameter);
    }
    auto pid = id_generator().GetOrCreatePid(property);
    if (!pid_set.insert(pid).second) {
      THROW_CODE(InvalidParameter, "Duplicate property [{}] in index [{}]",
                 property, index_name);
    }
    pids.push_back(pid);
  }
  if (meta_info_.GetVertexPropertyIndex(lid, pids)) {
    THROW_CODE(VertexIndexAlreadyExist,
               "Vertex index [label:{}, property_count:{}] already exists",
               big_to_native(lid), pids.size());
  }

  auto index_id = id_generator().GetNextIndexId();
  meta::VertexPropertyIndex meta_val;
  meta_val.set_name(index_name);
  meta_val.set_is_unique(unique);
  meta_val.set_label(label);
  meta_val.set_label_id(big_to_native(lid));
  meta_val.set_index_id(big_to_native(index_id));
  meta_val.set_build_start_wal_id(0);
  meta_val.set_applied_wal_id(0);
  meta_val.clear_build_error();
  for (size_t i = 0; i < properties.size(); ++i) {
    meta_val.add_properties(properties[i]);
    meta_val.add_property_ids(big_to_native(pids[i]));
  }

  auto vpi = std::make_shared<VertexPropertyIndex>(
      db_, &graph_cf_, meta_val, graph_cf_.index, index_id, lid, pids);

  vpi->SetState(meta::IndexBuildState::BUILDING);
  auto s = db_->Put({}, graph_cf_.meta_info,
                    BuildMetaKey(MetaDataType::VertexPropertyIndex, index_name),
                    vpi->meta().SerializeAsString());
  if (!s.ok()) THROW_CODE(StorageEngineError, s.ToString());
  auto ret = meta_info_.AddVertexPropertyIndex(vpi);
  assert(ret);
  LOG_INFO(
      "Begin online build vertex index: [lid:{}, property_count:{}, "
      "is_unique:{}]",
      big_to_native(lid), pids.size(), unique);
  ScheduleVertexPropertyIndexBuild(vpi, false);
}

void GraphDB::DeleteVertexPropertyIndex(const std::string& index_name) {
  std::lock_guard<std::mutex> clear_lock(clear_data_mutex_);
  std::lock_guard<std::mutex> ddl_lock(index_ddl_mutex_);
  std::lock_guard<std::mutex> commit_lock(property_index_commit_mutex_);
  auto index = meta_info_.GetVertexPropertyIndex(index_name);
  if (!index) {
    THROW_CODE(VertexUniqueIndexNotFound, "No such vertex index [{}]",
               index_name);
  }
  index->MarkDeleted();
  uint32_t index_id = index->index_id();
  meta_info_.DeleteVertexPropertyIndex(index_name);

  rocksdb::WriteBatch wb;
  wb.Delete(graph_cf_.meta_info,
            BuildMetaKey(MetaDataType::VertexPropertyIndex, index_name));
  std::string start_key(AsChars(index_id), sizeof(index_id));
  std::string end_key(AsChars(index_id), sizeof(index_id));
  end_key.append(128, static_cast<char>(0xFF));
  wb.DeleteRange(graph_cf_.index, start_key, end_key);
  std::string wal_start(AsChars(index_id), sizeof(index_id));
  std::string wal_end(AsChars(index_id), sizeof(index_id));
  wal_end.append(sizeof(uint64_t), static_cast<char>(0xFF));
  wb.DeleteRange(graph_cf_.wal, wal_start, wal_end);

  rocksdb::TransactionDBWriteOptimizations two;
  two.skip_concurrency_control = true;
  two.skip_duplicate_key_check = true;
  auto s = db_->Write({}, two, &wb);
  if (!s.ok()) THROW_CODE(StorageEngineError, s.ToString());
  LOG_INFO("Delete vertex index: {}", index_name);
}

void GraphDB::AddVertexFullTextIndex(
    const std::string& index_name, const std::vector<std::string>& labels,
    const std::vector<std::string>& properties) {
  std::lock_guard<std::mutex> clear_lock(clear_data_mutex_);
  std::lock_guard<std::mutex> ddl_lock(index_ddl_mutex_);
  if (index_name.empty() || labels.empty() || properties.empty()) {
    THROW_CODE(InvalidParameter);
  }
  if (meta_info_.GetVertexFullTextIndex(index_name)) {
    THROW_CODE(VertexFullTextIndexAlreadyExist,
               "Vertex fulltext index [{}] already exists", index_name);
  }
  std::unordered_set<uint32_t> lids, native_lids;
  std::unordered_set<uint32_t> pids, native_pids;
  for (const auto& label : labels) {
    auto lid = id_generator().GetOrCreateLid(label);
    lids.insert(lid);
    native_lids.insert(big_to_native(lid));
  }
  for (const auto& prop : properties) {
    auto pid = id_generator().GetOrCreatePid(prop);
    pids.insert(pid);
    native_pids.insert(big_to_native(pid));
  }
  uint32_t index_id = id_generator().GetNextIndexId();
  std::string ft_index_pth =
      BuildFullTextIndexPath(path_, index_name, index_id);
  DeleteFullTextIndexRanges(db_, &graph_cf_, index_id);
  ResetFullTextIndexPath(ft_index_pth);

  meta::VertexFullTextIndex meta;
  meta.set_index_id(big_to_native(index_id));
  meta.set_name(index_name);
  meta.set_path(ft_index_pth);
  meta.set_state(meta::IndexBuildState::BUILDING);
  meta.set_build_start_wal_id(0);
  meta.set_applied_wal_id(0);
  meta.clear_build_error();
  *meta.mutable_labels() = {labels.begin(), labels.end()};
  *meta.mutable_properties() = {properties.begin(), properties.end()};
  *meta.mutable_label_ids() = {native_lids.begin(), native_lids.end()};
  *meta.mutable_property_ids() = {native_pids.begin(), native_pids.end()};

  auto v_ft_index = std::make_shared<VertexFullTextIndex>(
      db_, assistant_, &graph_cf_, &id_generator(), meta, index_id,
      options_.ft_writer_threads_, options_.ft_writer_memory_budget_, lids,
      pids, options_.ft_apply_interval_);
  auto s = db_->Put({}, graph_cf_.meta_info,
                    BuildMetaKey(MetaDataType::VertexFullTextIndex, index_name),
                    meta.SerializeAsString());
  if (!s.ok()) THROW_CODE(StorageEngineError, s.ToString());
  meta_info_.AddVertexFullTextIndex(v_ft_index);
  LOG_INFO("Begin online build vertex full text index: [lids:{}, pids:{}]",
           native_lids, native_pids);
  ScheduleVertexFullTextIndexBuild(v_ft_index, false);
}

void GraphDB::DeleteVertexFullTextIndex(const std::string& index_name) {
  std::lock_guard<std::mutex> clear_lock(clear_data_mutex_);
  std::lock_guard<std::mutex> ddl_lock(index_ddl_mutex_);
  std::lock_guard<std::mutex> commit_lock(fulltext_index_commit_mutex_);
  auto ft_index = meta_info_.GetVertexFullTextIndex(index_name);
  if (!ft_index) {
    THROW_CODE(FullTextIndexNotFound, "No such vertex fulltext index [{}]",
               index_name);
  }
  std::string path = ft_index->meta().path();
  uint32_t index_id = ft_index->index_id();
  ft_index->MarkDeleted();
  ft_index->Stop();
  ft_index->ReleaseResources();
  meta_info_.DeleteVertexFullTextIndex(index_name);

  rocksdb::WriteBatch wb;
  wb.Delete(graph_cf_.meta_info,
            BuildMetaKey(MetaDataType::VertexFullTextIndex, index_name));

  std::string start_key(AsChars(index_id), sizeof(index_id));
  start_key.append(sizeof(int64_t), static_cast<char>(0x00));
  std::string end_key(AsChars(index_id), sizeof(index_id));
  end_key.append(sizeof(int64_t), static_cast<char>(0xFF));
  wb.DeleteRange(graph_cf_.index, start_key, end_key);
  wb.DeleteRange(graph_cf_.wal, start_key, end_key);

  rocksdb::TransactionDBWriteOptimizations two;
  two.skip_concurrency_control = true;
  two.skip_duplicate_key_check = true;
  auto s = db_->Write({}, two, &wb);
  if (!s.ok()) THROW_CODE(StorageEngineError, s.ToString());
  try {
    std::filesystem::remove_all(path);
    LOG_INFO("remove fulltext index data {}", path);
  } catch (const std::exception& e) {
    LOG_ERROR("[DeleteVertexFullTextIndex] remove_all {} meet error: {}", path,
              e.what());
  }
}

void GraphDB::AddVertexVectorIndex(const std::string& index_name,
                                   const std::string& label,
                                   const std::string& property, int dimension,
                                   std::string distance_type, int hnsw_m,
                                   int hnsw_ef_construction) {
  std::lock_guard<std::mutex> clear_lock(clear_data_mutex_);
  std::lock_guard<std::mutex> ddl_lock(index_ddl_mutex_);
  if (index_name.empty() || label.empty() || property.empty()) {
    THROW_CODE(InvalidParameter);
  }
  if (dimension < 1 || dimension > 4096) {
    THROW_CODE(InvalidParameter,
               "dimension should be an integer in the range [1, 4096]");
  }
  if (hnsw_m < 5 || hnsw_m > 64) {
    THROW_CODE(InvalidParameter,
               "hnsw.m should be an integer in the range [5, 64]");
  }
  if (hnsw_ef_construction < hnsw_m || hnsw_ef_construction > 1000) {
    THROW_CODE(
        InvalidParameter,
        "hnsw.efConstruction should be an integer in the range [hnsw.m,1000]");
  }
  meta::VectorDistanceType dist_type;
  if (distance_type == "l2") {
    dist_type = meta::VectorDistanceType::L2;
  } else if (distance_type == "ip") {
    dist_type = meta::VectorDistanceType::IP;
  } else {
    THROW_CODE(InvalidParameter, "Distance Type {} not supported",
               distance_type);
  }
  if (meta_info_.GetVertexVectorIndex(index_name)) {
    THROW_CODE(VertexVectorIndexAlreadyExist,
               "Vertex vector index [{}] already exists", index_name);
  }
  auto lid = id_generator().GetOrCreateLid(label);
  auto pid = id_generator().GetOrCreatePid(property);
  if (meta_info_.GetVertexVectorIndex(lid, pid)) {
    THROW_CODE(VertexVectorIndexAlreadyExist,
               "Vertex vector index [label:{}, property:{}] already exists",
               big_to_native(lid), big_to_native(pid));
  }
  uint32_t index_id = id_generator().GetNextIndexId();
  std::string vt_index_pth = path_ + "/vt/" + index_name;
  DeleteVectorIndexRanges(db_, &graph_cf_, index_id);
  ResetIndexPath(vt_index_pth, "vector index");
  meta::VectorIndexType index_type = meta::VectorIndexType::HNSW;
  meta::VertexVectorIndex meta;
  meta.set_index_id(big_to_native(index_id));
  meta.set_path(vt_index_pth);
  meta.set_name(index_name);
  meta.set_label(label);
  meta.set_property(property);
  meta.set_label_id(big_to_native(lid));
  meta.set_property_id(big_to_native(pid));
  meta.set_dimensions(dimension);
  meta.set_index_type(index_type);
  meta.set_distance_type(dist_type);
  meta.set_hnsw_m(hnsw_m);
  meta.set_hnsw_ef_construction(hnsw_ef_construction);
  meta.set_state(meta::IndexBuildState::BUILDING);
  meta.set_build_start_wal_id(0);
  meta.set_applied_wal_id(0);
  meta.clear_build_error();

  auto vvi = std::make_shared<VertexVectorIndex>(db_, assistant_, &graph_cf_,
                                                 index_id, lid, pid, meta,
                                                 options_.vt_apply_interval_);
  auto s = db_->Put({}, graph_cf_.meta_info,
                    BuildMetaKey(MetaDataType::VertexVectorIndex, index_name),
                    meta.SerializeAsString());
  if (!s.ok()) THROW_CODE(StorageEngineError, s.ToString());
  meta_info_.AddVertexVectorIndex(vvi);
  LOG_INFO("Begin online build vertex vector index: [lid:{}, pid:{}]",
           big_to_native(lid), big_to_native(pid));
  ScheduleVertexVectorIndexBuild(vvi, false);
}

void GraphDB::DeleteVertexVectorIndex(const std::string& index_name) {
  std::lock_guard<std::mutex> clear_lock(clear_data_mutex_);
  std::lock_guard<std::mutex> ddl_lock(index_ddl_mutex_);
  std::lock_guard<std::mutex> commit_lock(vector_index_commit_mutex_);
  auto index = meta_info_.GetVertexVectorIndex(index_name);
  if (!index) {
    THROW_CODE(VectorIndexNotFound, "No such vertex vector index [{}]",
               index_name);
  }
  auto path = index->meta().path();
  uint32_t index_id = index->index_id();
  index->MarkDeleted();
  index->Stop();
  index->ReleaseResources();
  meta_info_.DeleteVertexVectorIndex(index_name);
  rocksdb::WriteBatch wb;
  wb.Delete(graph_cf_.meta_info,
            BuildMetaKey(MetaDataType::VertexVectorIndex, index_name));

  std::string wal_start(AsChars(index_id), sizeof(index_id));
  std::string wal_end(AsChars(index_id), sizeof(index_id));
  wal_end.append(sizeof(uint64_t), static_cast<char>(0xFF));
  wb.DeleteRange(graph_cf_.wal, wal_start, wal_end);

  rocksdb::TransactionDBWriteOptimizations two;
  two.skip_concurrency_control = true;
  two.skip_duplicate_key_check = true;
  auto s = db_->Write({}, two, &wb);
  if (!s.ok()) THROW_CODE(StorageEngineError, s.ToString());

  try {
    std::filesystem::remove_all(path);
    LOG_INFO("remove vector index data {}", path);
  } catch (const std::exception& e) {
    LOG_ERROR("[DeleteVertexVectorIndex] remove_all {} meet error: {}", path,
              e.what());
  }
}

}  // namespace graphdb
