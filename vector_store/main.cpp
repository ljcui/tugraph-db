#include <iostream>
#include <nlohmann/json.hpp>
#include <roaring/roaring64map.hh>
#include "vector_index_store.h"
#include "logger.h"

int main(int angc, char* argv[]) {
    //SetupLogger();
    {
        auto hnsw_build_paramesters = R"(
    {
        "dtype": "float32",
        "metric_type": "l2",
        "dim": 128,
        "hnsw": {
            "max_degree": 16,
            "ef_construction": 100
        }
    }
    )";
        IndexStoreOptions options;
        options.index_build_parameters = hnsw_build_paramesters;
        VectorIndexStore store("index_db", options);
        int dim = 128;
        std::mt19937 rng;
        rng.seed(47);
        std::uniform_real_distribution<> distrib_real;
        uint64_t wal_id = 0;
        for (int index_id = 0; index_id < 20000; index_id++) {
            auto vector = new float[dim];
            for (int i = 0; i < dim; i++) {
                vector[i] = distrib_real(rng);
            }
            store.Add(index_id, vector, wal_id++);
        }
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }

    {
        auto hnsw_build_paramesters = R"(
    {
        "dtype": "float32",
        "metric_type": "l2",
        "dim": 128,
        "hnsw": {
            "max_degree": 16,
            "ef_construction": 100
        }
    }
    )";
        IndexStoreOptions options;
        options.index_build_parameters = hnsw_build_paramesters;
        VectorIndexStore store("index_db", options);
        LOG_INFO("VectorNum: {}, DeleteNum:{}", store.VectorNum(), store.DeleteNum());
        std::this_thread::sleep_for(std::chrono::seconds(10));

    }

}