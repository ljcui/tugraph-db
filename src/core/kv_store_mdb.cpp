﻿/* Copyright (c) 2022 AntGroup. All Rights Reserved. */

#if (!LGRAPH_USE_MOCK_KV)
#include <chrono>
#include "core/kv_store_mdb.h"
#include "core/wal.h"

using namespace std::chrono_literals;

namespace lgraph {

static const std::string DATA_FILE_NAME = "data.mdb";  // NOLINT
std::atomic<int64_t> KvStore::last_op_id_(-1);

void KvStore::Open(const std::string& path, size_t db_size, bool durable,
                   bool create_if_not_exist) {
    auto& fs = fma_common::FileSystem::GetFileSystem(fma_common::FilePath::SchemeType::LOCAL);
    if (create_if_not_exist) {
        if (!fs.IsDir(path) && !fs.Mkdir(path)) {
            throw KvException(std::string("Failed to create data directory ") + path);
        }
    } else {
        if (!fs.IsDir(path) || !fs.FileExists(path + "/" + DATA_FILE_NAME)) {
            throw KvException("Data directory " + path + " does not contain valid data.");
        }
    }
    THROW_ON_ERR(mdb_env_create(&env_));
    THROW_ON_ERR(mdb_env_set_mapsize(env_, db_size));
    THROW_ON_ERR(mdb_env_set_maxdbs(env_, 255));
    THROW_ON_ERR(mdb_env_set_maxreaders(env_, 240));
#if LGRAPH_SHARE_DIR
    unsigned int flags = MDB_NOMEMINIT | MDB_NORDAHEAD | MDB_NOTLS | MDB_NOSYNC;
#else
    unsigned int flags = MDB_NOMEMINIT | MDB_NORDAHEAD;
    if (durable) {
        flags |= MDB_NOSYNC;
    }
#endif
    THROW_ON_ERR(mdb_env_open(env_, path.c_str(), flags, 0664));
    path_ = path;
    db_size_ = db_size;
    durable_ = durable;
    // update last op id of all stores with the value stored in this one
    MDB_txn* txn;
    THROW_ON_ERR(mdb_txn_begin(env_, nullptr, MDB_RDONLY, &txn));
    int64_t last_op_id = mdb_txn_last_op_id(txn);
    mdb_txn_abort(txn);
    KvStore::UpdateLastOpIdWithStoredValue(last_op_id);
    // start wal
    wal_.reset();
    if (durable) {
        // TODO(hct): set wal flush interval
        wal_.reset(new Wal(env_, path_, 60 * 1000));
    }
}

void KvStore::ReopenFromSnapshot(const std::string& snapshot_path) {
    wal_.reset();
    mdb_env_close(env_);
    // copy the snapshot file
    fma_common::FileSystem& fs = fma_common::FileSystem::GetFileSystem(snapshot_path);
    std::string src = snapshot_path + "/" + DATA_FILE_NAME;
    std::string dst = path_ + "/" + DATA_FILE_NAME;
    fs.RemoveDir(dst);
    if (!fs.CopyToLocal(src, dst)) {
        throw KvException("Failed to copy snapshot file from " + src + " to " + dst);
    }
    Open(path_, db_size_, durable_, true);
}

KvStore::KvStore(const std::string& path, size_t db_size, bool durable,
                 bool create_if_not_exist) {
    Open(path, db_size, durable, create_if_not_exist);
    finished_ = false;
    validator_ = std::thread([this]() { this->ServeValidation(); });
}

KvStore::~KvStore() {
    if (env_) {
        wal_.reset();
        mdb_env_close(env_);
    }
    finished_ = true;
    queue_cv_.notify_one();
    validator_.join();
}

KvTransaction KvStore::CreateReadTxn() { return KvTransaction(*this, true, false, false); }

KvTransaction KvStore::CreateWriteTxn(bool optimistic, bool flush) {
    return KvTransaction(*this, false, optimistic, flush);
}

KvTable KvStore::OpenTable(KvTransaction& txn, const std::string& table_name,
                           bool create_if_not_exist, const ComparatorDesc& desc) {
    std::lock_guard<std::mutex> l(mutex_);
    KvTable t = KvTable(txn, table_name, create_if_not_exist, desc);
    return t;
}

KvTable KvStore::_OpenTable_(KvTransaction& txn, const std::string& table_name,
                           bool create_if_not_exist, const KeySortFunc& func) {
    std::lock_guard<std::mutex> l(mutex_);
    KvTable t = KvTable(txn, table_name, create_if_not_exist,
        ComparatorDesc::DefaultComparator());
    if (func) mdb_set_compare(txn.GetTxn(), t.dbi_, func);
    return t;
}

bool KvStore::DeleteTable(KvTransaction& txn, const std::string& table_name) {
    std::lock_guard<std::mutex> l(mutex_);
    auto tbl = KvTable(txn, table_name, true, ComparatorDesc::DefaultComparator());
    tbl.Drop(txn);
    return true;
}

std::vector<std::string> KvStore::ListAllTables(KvTransaction& txn) {
    std::vector<std::string> tables;
    KvTable unamed_table = KvTable(txn, "", false, ComparatorDesc::DefaultComparator());
    KvIterator it = unamed_table.GetIterator(txn);
    while (it.IsValid()) {
        Value name = it.GetKey();
        tables.emplace_back(name.AsString());
        it.Next();
    }
    return tables;
}

void KvStore::Flush() { THROW_ON_ERR(mdb_env_sync(env_, true)); }

void KvStore::DropAll(KvTransaction& txn) {
    auto all_tables = ListAllTables(txn);
    for (auto& tbl : all_tables) {
        DeleteTable(txn, tbl);
    }
}

void KvStore::DumpStat(KvTransaction& txn, size_t& memory_size, size_t& height) {
    memory_size = 0;
    height = 0;
    MDB_stat stat;
    stat.ms_psize = 4096;
    auto tables = ListAllTables(txn);
    for (auto tbl : tables) {
        KvTable t(txn, tbl, false, ComparatorDesc::DefaultComparator());
        THROW_ON_ERR(mdb_stat(txn.GetTxn(), t.GetDbi(), &stat));
        // LOG() << "Table (" << tbl << "):"
        //    << "\n\tbranch_pages: " << stat.ms_branch_pages
        //    << "\n\tleaf_pages: " << stat.ms_leaf_pages
        //    << "\n\toverflow_pages: " << stat.ms_overflow_pages
        //    << "\n\tdepth: " << stat.ms_depth;
        memory_size += stat.ms_branch_pages + stat.ms_leaf_pages + stat.ms_overflow_pages;
        height = std::max<size_t>(height, stat.ms_depth);
    }
    memory_size *= stat.ms_psize;
}

size_t KvStore::Backup(const std::string& path, bool compact) {
    size_t last_txn_id = 0;
    THROW_ON_ERR(mdb_env_copy2(env_, path.c_str(), compact ? MDB_CP_COMPACT : 0, &last_txn_id));
    return last_txn_id;
}

void KvStore::Snapshot(KvTransaction& txn, const std::string& path, bool compaction) {
    int flags = 0;
    if (compaction) flags |= MDB_CP_COMPACT;
    THROW_ON_ERR(mdb_env_copy_txn(env_, path.c_str(), txn.GetTxn(), flags));
}

void KvStore::LoadSnapshot(const std::string& snapshot_path) { ReopenFromSnapshot(snapshot_path); }

void KvStore::WarmUp(size_t* size) {
    auto txn = CreateReadTxn();
    auto tables = ListAllTables(txn);
    size_t s = 0;
    for (auto tbl : tables) {
        KvTable t(txn, tbl, false, ComparatorDesc::DefaultComparator());
        for (auto it = t.GetIterator(txn); it.IsValid(); it.Next()) {
            s += it.GetKey().Size();
            s += it.GetValue().Size();
        }
    }
    if (size) *size = s;
}

void KvStore::ServeValidation() {
    while (true) {
        std::unique_lock<std::mutex> queue_lock(queue_mutex_);
        while (!queue_cv_.wait_for(queue_lock, 100ms,
                                   [this]() { return !queue_.empty() || finished_; })) {
        }
        if (queue_.empty() && finished_) break;
        std::vector<KvTransaction*> txns;
        while (!queue_.empty()) {
            txns.emplace_back(queue_.front());
            queue_.pop();
        }
        queue_lock.unlock();
        MDB_txn* root_txn;
        int txn_begin_flags = 0;
        if (!durable_) txn_begin_flags = MDB_NOSYNC;
        int ec;
        ec = mdb_txn_begin(env_, nullptr, txn_begin_flags, &root_txn);
        if (ec != MDB_SUCCESS) {
            for (size_t i = 0; i < txns.size(); i++) {
                txns[i]->commit_status_ = -1;
                txns[i]->cv_.notify_one();
            }
            continue;
        }
        size_t write_version = mdb_txn_id(root_txn);
        if (wal_.get())
            wal_->WriteTxnBegin(write_version);
        std::vector<int> results;
        for (KvTransaction* txn : txns) {
            MDB_txn* child_txn;
            ec = mdb_txn_begin(env_, root_txn, txn_begin_flags, &child_txn);
            if (ec != MDB_SUCCESS) {
                results.emplace_back(ec);
                continue;
            }
            if (wal_.get())
                wal_->WriteTxnBegin(write_version, true);
            bool successful = true;
            for (auto it = txn->deltas_.begin(); it != txn->deltas_.end(); it++) {
                const auto& dbi = it->first;
                const auto& delta = it->second;
                MDB_cursor* cursor;
                ec = mdb_cursor_open(child_txn, dbi, &cursor);
                if (ec != MDB_SUCCESS) {
                    successful = false;
                    break;
                }
                for (auto it = delta.write_set_.begin(); it != delta.write_set_.end(); it++) {
                    const auto& key = it->first;
                    const auto& packed_value = it->second;
                    size_t read_version = *(size_t*)(packed_value.data());
                    int8_t op_type = *(int8_t*)(packed_value.data() + sizeof(size_t));
                    MDB_val mdb_key = {key.size(), (char*)key.data()};
                    MDB_val mdb_value = {0, nullptr};
                    ec = mdb_cursor_get(cursor, &mdb_key, &mdb_value, MDB_SET_KEY);
                    size_t latest_version = 0;
                    if (ec == MDB_SUCCESS) {
                        latest_version = *(size_t*)(mdb_value.mv_data);
                    } else if (ec != MDB_NOTFOUND) {
                        successful = false;
                        break;
                    }
                    if (latest_version != read_version) {
                        // FMA_LOG() << key << " expects " << read_version << " but got " <<
                        // latest_version << " op_type: " << (op_type==1 ? "Put" : ((op_type == 0) ?
                        // "GetForUpdate" : "Delete"));
                        successful = false;
                        ec = MDB_CONFLICTS;
                        break;
                    }
                    if (op_type == 1) {
                        mdb_key = {key.size(), (char*)key.data()};
                        Value tmp(packed_value.size() - sizeof(int8_t));
                        *(size_t*)(tmp.Data()) = write_version;
                        memcpy(tmp.Data() + sizeof(size_t),
                               packed_value.data() + sizeof(size_t) + sizeof(int8_t),
                               packed_value.size() - sizeof(size_t) - sizeof(int8_t));
                        MDB_val mdb_value = tmp.MakeMdbVal();
                        int flags = 0;
                        ec = mdb_cursor_put(cursor, &mdb_key, &mdb_value, flags);
                        if (ec != MDB_SUCCESS) {
                            successful = false;
                            break;
                        }
                        if (wal_.get())
                            wal_->WriteKvPut(dbi,
                                             Value((const char*)mdb_key.mv_data, mdb_key.mv_size),
                                             tmp);
                    } else if (op_type == -1) {
                        ec = mdb_cursor_del(cursor, 0);
                        if (ec != MDB_SUCCESS) {
                            successful = false;
                            break;
                        }
                        if (wal_.get())
                            wal_->WriteKvDel(dbi, Value::ConstRef(key));
                    }
                }
                if (!successful) break;
            }
            if (successful) {
                mdb_txn_set_last_op_id(child_txn, GetLastOpIdOfAllStores());
                ec = mdb_txn_commit(child_txn);
                if (wal_.get() && ec == MDB_SUCCESS)
                    wal_->WriteTxnCommit(write_version, true);
                else if (wal_.get() && ec != MDB_SUCCESS)
                    wal_->WriteTxnAbort(write_version, true);
                results.emplace_back(ec);
            } else {
                mdb_txn_abort(child_txn);
                if (wal_.get())
                    wal_->WriteTxnAbort(write_version, true);
                results.emplace_back(ec);
            }
        }
        // set last op id
        mdb_txn_set_last_op_id(root_txn, GetLastOpIdOfAllStores());
        if (wal_.get())
            wal_->WriteTxnCommit(write_version);
        ec = mdb_txn_commit(root_txn);
        if (ec == MDB_SUCCESS) {
            for (size_t i = 0; i < txns.size(); i++) {
                txns[i]->commit_status_ = (results[i] == MDB_SUCCESS) ? 1 : -1;
                txns[i]->commit_ec_ = results[i];
                txns[i]->cv_.notify_one();
            }
        } else {
            for (size_t i = 0; i < txns.size(); i++) {
                txns[i]->commit_status_ = -1;
                txns[i]->commit_ec_ = (results[i] == MDB_SUCCESS) ? ec : results[i];
                txns[i]->cv_.notify_one();
            }
        }
    }
}

}  // namespace lgraph
#endif
