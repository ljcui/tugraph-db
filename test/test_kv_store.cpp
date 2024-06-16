/**
 * Copyright 2022 AntGroup CO., Ltd.
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

#include <future>
#include <random>
#include "fma-common/configuration.h"
#include "fma-common/file_system.h"
#include "fma-common/string_util.h"
#include "fma-common/fma_stream.h"
#include "gtest/gtest.h"
#include "core/kv_store.h"

#include "./test_tools.h"
#include "./ut_utils.h"
using namespace lgraph;
using namespace fma_common;
using namespace std::chrono;
class TestKvStore : public TuGraphTest {};

#define MCHECK(test, msg) ((test) ? (void)0 : ((void)fprintf(stderr, \
    "%s:%d: %s: %s\n", __FILE__, __LINE__, msg, mdb_strerror(rc)), abort()))
#define RES(err, expr) ((rc = expr) == (err) || (MCHECK(!rc, #expr), 0))
#define E(expr) MCHECK((rc = (expr)) == MDB_SUCCESS, #expr)

class RandomNumberGenerator {
    std::random_device rd;
    std::mt19937_64 gen;
    std::uniform_int_distribution<uint64_t> dis;
 public:
    RandomNumberGenerator(uint64_t start, uint64_t end)
        :gen(rd()), dis(start, end) {
    }
    uint64_t next() {
        return dis(gen);
    }
};

std::string random_str(int len) {
    const std::string chars = "AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz0123456789";
    std::string ret(len, 0);
    RandomNumberGenerator rng(0, chars.size()-1);
    for (int64_t i = 0; i < ret.size(); i++) {
        ret[i] = chars[rng.next()];
    }
    return ret;
};

namespace fs = std::filesystem;
void createOrClearDirectory(const fs::path& path) {
    if(fs::exists(path)) {
        if(fs::is_directory(path)) {
            for(const auto& entry : fs::directory_iterator(path)) {
                fs::remove_all(entry.path());
            }
            std::cout << "Directory exists, cleared its contents." << std::endl;
        } else {
            std::cout << "Path exists but is not a directory!" << std::endl;
        }
    } else {
        if(fs::create_directories(path)) {
            std::cout << "Directory created successfully." << std::endl;
        } else {
            std::cout << "Failed to create directory!" << std::endl;
        }
    }
}

TEST_F(TestKvStore, benchmark) {
    auto create_db = [](std::string name) {
        int rc;
        MDB_env *env;
        MDB_dbi primary_dbi, secondary_dbi;
        MDB_txn *txn;
        E(mdb_env_create(&env));
        E(mdb_env_set_mapsize(env, (size_t)1 << 40));
        E(mdb_env_set_maxdbs(env, 255));
        unsigned int flags = MDB_NOMEMINIT | MDB_NORDAHEAD;
        E(mdb_env_open(env, name.c_str(), flags, 0664));
        E(mdb_txn_begin(env, nullptr, MDB_NOSYNC, &txn));
        E(mdb_dbi_open(txn, "primary", MDB_CREATE, &primary_dbi));
        E(mdb_dbi_open(txn, "secondary", MDB_CREATE, &secondary_dbi));
        E(mdb_txn_commit(txn));
        return std::make_tuple(env, primary_dbi, secondary_dbi);
    };
    auto close_db = [](const std::tuple<MDB_env*, MDB_dbi, MDB_dbi>& ins) {
        mdb_dbi_close(std::get<0>(ins), std::get<1>(ins));
        mdb_dbi_close(std::get<0>(ins), std::get<2>(ins));
        mdb_env_close(std::get<0>(ins));
    };
    createOrClearDirectory("tugraph");
    createOrClearDirectory("sf");
    createOrClearDirectory("rf");
    auto tugraph = create_db("tugraph");
    auto sf = create_db("sf");
    auto rf = create_db("rf");

    RandomNumberGenerator vid_rng(1, 100000);
    RandomNumberGenerator label_rng(1, 10);
    RandomNumberGenerator timestamp_rng(1686983797, 1718519797);
    MDB_txn *txn;
    int rc;
    for (auto i = 0; i < 100000; i++) {
        uint64_t src = vid_rng.next();
        uint64_t edge_label = label_rng.next();
        uint64_t dst = vid_rng.next();
        uint64_t edge_key = i;
        uint64_t timestamp = timestamp_rng.next();
        std::string edge_data = random_str(10);


        // tugraph
        E(mdb_txn_begin(std::get<0>(tugraph), nullptr, MDB_NOSYNC, &txn));
        // Primary Index: [src] [edge_label] [dst] [edge_key] -> [timestamp]
        {
            std::string key;
            key.append((const char *)&src, sizeof(uint64_t));
            key.append((const char *)&edge_label, sizeof(uint64_t));
            key.append((const char *)&dst, sizeof(uint64_t));
            key.append((const char *)&edge_key, sizeof(uint64_t));
            std::string val;
            val.append((const char *)&timestamp, sizeof(uint64_t));

            MDB_val k = {key.size(), (void *)key.data()};
            MDB_val v = {val.size(), nullptr};
            E(mdb_put(txn, std::get<1>(tugraph), &k, &v, MDB_RESERVE));
            memcpy((char *)(v.mv_data), val.data(), val.size());
        }
        // Secondary Index: [src] [edge_label] [timestamp] [dst] [edge_key] -> [other_edge_data]
        {
            std::string key;
            key.append((const char *)&src, sizeof(uint64_t));
            key.append((const char *)&edge_label, sizeof(uint64_t));
            key.append((const char *)&timestamp, sizeof(uint64_t));
            key.append((const char *)&dst, sizeof(uint64_t));
            key.append((const char *)&edge_key, sizeof(uint64_t));
            std::string val = edge_data;

            MDB_val k = {key.size(), (void *)key.data()};
            MDB_val v = {val.size(), nullptr};
            E(mdb_put(txn, std::get<2>(tugraph), &k, &v, MDB_RESERVE));
            memcpy((char *)(v.mv_data), val.data(), val.size());
        }
        E(mdb_txn_commit(txn));

        // SF
        E(mdb_txn_begin(std::get<0>(sf), nullptr, MDB_NOSYNC, &txn));
        // Primary Index: [src] [edge_label] [dst] [edge_key] -> [timestamp] [other_edge_data]
        {
            std::string key;
            key.append((const char *)&src, sizeof(uint64_t));
            key.append((const char *)&edge_label, sizeof(uint64_t));
            key.append((const char *)&dst, sizeof(uint64_t));
            key.append((const char *)&edge_key, sizeof(uint64_t));
            std::string val;
            val.append((const char *)&timestamp, sizeof(uint64_t));
            val.append(edge_data);

            MDB_val k = {key.size(), (void *)key.data()};
            MDB_val v = {val.size(), nullptr};
            E(mdb_put(txn, std::get<1>(sf), &k, &v, MDB_RESERVE));
            memcpy((char *)(v.mv_data), val.data(), val.size());
        }
        // Secondary Index: [src] [edge_label] [timestamp] [dst] [edge_key] -> []
        {
            std::string key;
            key.append((const char *)&src, sizeof(uint64_t));
            key.append((const char *)&edge_label, sizeof(uint64_t));
            key.append((const char *)&timestamp, sizeof(uint64_t));
            key.append((const char *)&dst, sizeof(uint64_t));
            key.append((const char *)&edge_key, sizeof(uint64_t));
            std::string val;

            MDB_val k = {key.size(), (void *)key.data()};
            MDB_val v = {val.size(), nullptr};
            E(mdb_put(txn, std::get<2>(sf), &k, &v, MDB_RESERVE));
            memcpy((char *)(v.mv_data), val.data(), val.size());
        }
        E(mdb_txn_commit(txn));

        // RF
        E(mdb_txn_begin(std::get<0>(rf), nullptr, MDB_NOSYNC, &txn));
        // Primary Index: [src] [edge_label] [dst] [edge_key] -> [timestamp] [other_edge_data]
        {
            std::string key;
            key.append((const char *)&src, sizeof(uint64_t));
            key.append((const char *)&edge_label, sizeof(uint64_t));
            key.append((const char *)&dst, sizeof(uint64_t));
            key.append((const char *)&edge_key, sizeof(uint64_t));
            std::string val;
            val.append((const char *)&timestamp, sizeof(uint64_t));
            val.append(edge_data);

            MDB_val k = {key.size(), (void *)key.data()};
            MDB_val v = {val.size(), nullptr};
            E(mdb_put(txn, std::get<1>(rf), &k, &v, MDB_RESERVE));
            memcpy((char *)(v.mv_data), val.data(), val.size());
        }
        // Secondary Index: [src] [edge_label] [timestamp] [dst] [edge_key] -> [other_edge_data]
        {
            std::string key;
            key.append((const char *)&src, sizeof(uint64_t));
            key.append((const char *)&edge_label, sizeof(uint64_t));
            key.append((const char *)&timestamp, sizeof(uint64_t));
            key.append((const char *)&dst, sizeof(uint64_t));
            key.append((const char *)&edge_key, sizeof(uint64_t));
            std::string val;
            val.append(edge_data);

            MDB_val k = {key.size(), (void *)key.data()};
            MDB_val v = {val.size(), nullptr};
            E(mdb_put(txn, std::get<2>(rf), &k, &v, MDB_RESERVE));
            memcpy((char *)(v.mv_data), val.data(), val.size());
        }
        E(mdb_txn_commit(txn));
    }
    std::cout << "tugraph size: " << std::filesystem::file_size("tugraph/data.mdb") << std::endl;
    std::cout << "sf size: " << std::filesystem::file_size("sf/data.mdb") << std::endl;
    std::cout << "rf size: " << std::filesystem::file_size("rf/data.mdb") << std::endl;

    // tugraph
    {
        int count = 0;
        auto t1 = steady_clock::now();
        E(mdb_txn_begin(std::get<0>(tugraph), nullptr, MDB_RDONLY, &txn));
        MDB_cursor *cursor;
        E(mdb_cursor_open(txn, std::get<2>(tugraph), &cursor));
        for (uint64_t i = 0; i < 100000; i++) {
            if (i % 2 != 0) continue;

            std::string prefix;
            prefix.append((const char *)&i, sizeof(uint64_t));
            MDB_val k = {prefix.size(), (void *)prefix.data()};
            MDB_val v;
            if (mdb_cursor_get(cursor, &k, &v, MDB_SET_RANGE) != MDB_SUCCESS) {
                continue;
            }
            do {
                auto p = (uint64_t *)k.mv_data;
                uint64_t src = *(p);
                if (src != i) {
                    break;
                }
                p++;
                // uint64_t edge_label = *(p);
                p++;
                uint64_t timestamp = *(p);
                if (timestamp < 1702535797) {
                    continue;
                }
                p++;
                // uint64_t dst = *(p);
                p++;
                // uint64_t edge_key = *(p);
                count++;
            } while (mdb_cursor_get(cursor, &k, &v, MDB_NEXT) == MDB_SUCCESS);
        }
        mdb_cursor_close(cursor);
        mdb_txn_abort(txn);
        auto t2 = steady_clock::now();
        std::cout << "tugraph: " << duration_cast<nanoseconds>(t2 - t1).count() / 1000000 << " ms "
                  << std::endl
                  << std::flush;
        std::cout << "count: " << count << std::endl;
    }

    // sf
    {
        int count = 0;
        auto t1 = steady_clock::now();
        E(mdb_txn_begin(std::get<0>(sf), nullptr, MDB_RDONLY, &txn));
        MDB_cursor *cursor;
        E(mdb_cursor_open(txn, std::get<2>(sf), &cursor));
        for (uint64_t i = 0; i < 100000; i++) {
            if (i % 2 != 0) continue;

            std::string prefix;
            prefix.append((const char *)&i, sizeof(uint64_t));
            MDB_val k = {prefix.size(), (void *)prefix.data()};
            MDB_val v;
            if (mdb_cursor_get(cursor, &k, &v, MDB_SET_RANGE) != MDB_SUCCESS) {
                continue;
            }
            do {
                UT_EXPECT_EQ(k.mv_size, 40);
                auto p = (uint64_t *)k.mv_data;
                uint64_t src = *(p);
                if (src != i) {
                    break;
                }
                p++;
                uint64_t edge_label = *(p);
                p++;
                uint64_t timestamp = *(p);
                if (timestamp < 1702535797) {
                    continue;
                }
                p++;
                uint64_t dst = *(p);
                p++;
                uint64_t edge_key = *(p);

                std::string pri_key;
                pri_key.append((const char *)&src, sizeof(uint64_t));
                pri_key.append((const char *)&edge_label, sizeof(uint64_t));
                pri_key.append((const char *)&dst, sizeof(uint64_t));
                pri_key.append((const char *)&edge_key, sizeof(uint64_t));
                MDB_val k1 = {pri_key.size(), (void*)pri_key.data()};
                MDB_val v1;
                E(mdb_get(txn, std::get<1>(sf), &k1, &v1));
                count++;
            } while (mdb_cursor_get(cursor, &k, &v, MDB_NEXT) == MDB_SUCCESS);
        }
        mdb_cursor_close(cursor);
        mdb_txn_abort(txn);
        auto t2 = steady_clock::now();
        std::cout << "sf: " << duration_cast<nanoseconds>(t2 - t1).count() / 1000000 << " ms "
                  << std::endl
                  << std::flush;
        std::cout << "count: " << count << std::endl;
    }

    // rf
    {
        int count = 0;
        auto t1 = steady_clock::now();
        E(mdb_txn_begin(std::get<0>(rf), nullptr, MDB_RDONLY, &txn));
        MDB_cursor *cursor;
        E(mdb_cursor_open(txn, std::get<2>(rf), &cursor));
        for (uint64_t i = 0; i < 100000; i++) {
            if (i % 2 != 0) continue;

            std::string prefix;
            prefix.append((const char *)&i, sizeof(uint64_t));
            MDB_val k = {prefix.size(), (void *)prefix.data()};
            MDB_val v;
            if (mdb_cursor_get(cursor, &k, &v, MDB_SET_RANGE) != MDB_SUCCESS) {
                continue;
            }
            do {
                auto p = (uint64_t *)k.mv_data;
                uint64_t src = *(p);
                if (src != i) {
                    break;
                }
                p++;
                // uint64_t edge_label = *(p);
                p++;
                uint64_t timestamp = *(p);
                if (timestamp < 1702535797) {
                    continue;
                }
                p++;
                // uint64_t dst = *(p);
                p++;
                // uint64_t edge_key = *(p);
                count++;
            } while (mdb_cursor_get(cursor, &k, &v, MDB_NEXT) == MDB_SUCCESS);
        }
        mdb_cursor_close(cursor);
        mdb_txn_abort(txn);
        auto t2 = steady_clock::now();
        std::cout << "rf: " << duration_cast<nanoseconds>(t2 - t1).count() / 1000000 << " ms "
                  << std::endl
                  << std::flush;
        std::cout << "count: " << count << std::endl;
    }

    close_db(tugraph);
    close_db(sf);
    close_db(rf);
}
