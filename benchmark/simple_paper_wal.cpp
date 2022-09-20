#include "fma-common/configuration.h"

#include "lgraph/lgraph.h"

#include <omp.h>

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>

using namespace fma_common;
using namespace lgraph_api;

typedef int64_t VertexId;

typedef int64_t EdgeId;

int answer_path(Transaction& txn, int hops, std::unordered_map<VertexId, VertexId>& parent,
                std::unordered_map<VertexId, VertexId>& child, VertexId vid_from, VertexId vid_a,
                VertexId vid_b, VertexId vid_to) {
    std::vector<VertexId> path_vids;
    path_vids.reserve(hops + 1);
    VertexId vid;

    vid = vid_a;
    while (vid != vid_from) {
        path_vids.push_back(vid);
        vid = parent[vid];
    }
    path_vids.push_back(vid);
    std::reverse(path_vids.begin(), path_vids.end());

    vid = vid_b;
    while (vid != vid_to) {
        path_vids.push_back(vid);
        vid = child[vid];
    }
    path_vids.push_back(vid);
    assert(path_vids.size() == hops + 1);

    std::vector<std::tuple<VertexId, VertexId, EdgeId> > path_triplets(hops);
    for (size_t i = 0; i < path_vids.size(); i++) {
        VertexId vid = path_vids[i];
        auto vit = txn.GetVertexIterator(vid);
        assert(vit.IsValid());
        if (i != path_vids.size() - 1) {
            auto eit = vit.GetOutEdgeIterator(EdgeUid(vid, path_vids[i + 1], 0, 0, 0), true);
            assert(eit.IsValid());
            EdgeId eid = eit.GetEdgeId();
            path_triplets[i] = std::make_tuple(path_vids[i], path_vids[i + 1], eid);
        }
    }
    assert(path_triplets.size() == hops);
    return hops;
}

size_t compute_shortest_path(Transaction& txn, VertexId vid_from, VertexId vid_to, int max_hops) {
    if (vid_from == vid_to) {
        return 0;
    }
    std::unordered_map<VertexId, VertexId> parent;
    std::unordered_map<VertexId, VertexId> child;
    std::vector<VertexId> forward_q, backward_q;
    parent[vid_from] = vid_from;
    child[vid_to] = vid_to;
    forward_q.push_back(vid_from);
    backward_q.push_back(vid_to);
    VertexId forward_front = 0, forward_rear = 1;
    VertexId backward_front = 0, backward_rear = 1;
    int hops = 0;
    while (hops++ < max_hops) {
        std::vector<VertexId> new_front;
        // decide which way to search first
        if (forward_q.size() <= backward_q.size()) {
            // search forward
            for (VertexId vid : forward_q) {
                auto vit = txn.GetVertexIterator(vid);
                assert(vit.IsValid());
                std::vector<VertexId> dstIds = vit.ListDstVids();
                for (VertexId dst : dstIds) {
                    if (child.find(dst) != child.end()) {
                        // found the path
                        return answer_path(txn, hops, parent, child, vid_from, vid, dst, vid_to);
                    }
                    auto it = parent.find(dst);
                    if (it == parent.end()) {
                        parent.emplace_hint(it, dst, vid);
                        new_front.push_back(dst);
                    }
                }
            }
            if (new_front.empty()) break;
            forward_q.swap(new_front);
        } else {
            for (VertexId vid : backward_q) {
                auto vit = txn.GetVertexIterator(vid);
                assert(vit.IsValid());
                std::vector<VertexId> srcIds = vit.ListSrcVids();
                for (VertexId src : srcIds) {
                    if (parent.find(src) != parent.end()) {
                        // found the path
                        return answer_path(txn, hops, parent, child, vid_from, src, vid, vid_to);
                    }
                    auto it = child.find(src);
                    if (it == child.end()) {
                        child.emplace_hint(it, src, vid);
                        new_front.push_back(src);
                    }
                }
            }
            if (new_front.empty()) break;
            backward_q.swap(new_front);
        }
    }
    return 0;
}

class RandomNumberGenerator {
    uint64_t s[2];

 public:
    RandomNumberGenerator(uint64_t seed = 0) {
        s[0] = seed;
        s[1] = seed + 1;
    }
    uint64_t next() {
        uint64_t x = s[0];
        uint64_t y = s[1];
        s[0] = y;
        x ^= (x << 23);
        s[1] = x ^ y ^ (x >> 17) ^ (y >> 26);
        return s[1] + y;
    }
};

int batch;
std::vector<RandomNumberGenerator> rngs;

class BenchmarkLightningGraph {
    const std::string chars = "AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz0123456789";

    GraphDB& db;
    size_t vertices;
    RandomNumberGenerator rng;

    std::string encode_no(size_t no) {
        std::string no_s = std::to_string(no);
        while (no_s.size() < 10) {
            no_s = "0" + no_s;
        }
        return no_s;
    }
    std::string random_no() { return encode_no(rng.next() % vertices); }
    std::string random_name() {
        std::string name = "";
        for (int i = 0; i < 10; i++) {
            name += chars[rng.next() % chars.size()];
        }
        return name;
    }
    void write_vertex(std::string no, std::string name) {
        Transaction txn = db.CreateWriteTxn();
        VertexId vid =
            txn.AddVertex(std::string("person"), std::vector<std::string>({"no", "name"}),
                          std::vector<std::string>({no, name}));
        txn.Commit();
    }
    void write_edge(std::string no_from, std::string no_to) {
        Transaction txn = db.CreateWriteTxn();
        VertexId vid_from;
        VertexId vid_to;
        bool ok = true;
        do {
            FieldData val_from(no_from);
            VertexIndexIterator it_from = txn.GetVertexIndexIterator("person", "no", val_from, val_from);
            if (it_from.IsValid()) {
                vid_from = it_from.GetVid();
            } else {
                ok = false;
                break;
            }
            FieldData val_to(no_to);
            VertexIndexIterator it_to = txn.GetVertexIndexIterator("person", "no", val_to, val_to);
            if (it_to.IsValid()) {
                vid_to = it_to.GetVid();
            } else {
                ok = false;
                break;
            }
        } while (0);
        if (ok) {
            txn.AddEdge(vid_from, vid_to, std::string("knows"),
                        std::vector<std::string>(),
                        std::vector<std::string>());
        } else {
            std::cerr << "insertion of edge(" + no_from + ", " + no_to + ") failed." << std::endl;
        }
        txn.Commit();
    }
    size_t read_neighbour(std::string no) {
        size_t ret = 0;
        Transaction txn = db.CreateReadTxn();
        bool ok = true;
        do {
            VertexId vid;
            FieldData val_no(no);
            auto it_no = txn.GetVertexIndexIterator("person", "no", val_no, val_no);
            // std::cout << no << std::endl;
            if (it_no.IsValid()) {
                vid = it_no.GetVid();
            } else {
                ok = false;
                break;
            }
            // std::cout << vid << std::endl;
            auto vit = txn.GetVertexIterator(vid);
            assert(vit.IsValid());
            std::vector<VertexId> ids;
            for (auto eit = vit.GetInEdgeIterator(); eit.IsValid(); eit.Next()) {
                VertexId nbr = eit.GetSrc();
                // std::cout << nbr << std::endl;
                ids.push_back(nbr);
            }
            for (auto eit = vit.GetOutEdgeIterator(); eit.IsValid(); eit.Next()) {
                VertexId nbr = eit.GetDst();
                // std::cout << nbr << std::endl;
                ids.push_back(nbr);
            }
            ret = ids.size();
            // std::cout << ret << std::endl;
        } while (0);
        if (!ok) {
            std::cerr << "vertex(" + no + ") not found." << std::endl;
        }
        txn.Abort();
        return ret;
    }
    size_t shortest_path(std::string no_from, std::string no_to, int max_hops) {
        size_t ret = 0;
        Transaction txn = db.CreateReadTxn();
        bool ok = true;
        do {
            VertexId vid_from;
            VertexId vid_to;
            FieldData val_from(no_from);
            VertexIndexIterator it_from = txn.GetVertexIndexIterator("person", "no", val_from, val_from);
            if (it_from.IsValid()) {
                vid_from = it_from.GetVid();
            } else {
                ok = false;
                break;
            }
            FieldData val_to(no_to);
            VertexIndexIterator it_to = txn.GetVertexIndexIterator("person", "no", val_to, val_to);
            if (it_to.IsValid()) {
                vid_to = it_to.GetVid();
            } else {
                ok = false;
                break;
            }

            // bidirectional bfs
            ret = compute_shortest_path(txn, vid_from, vid_to, max_hops);
        } while (0);
        if (!ok) {
            // std::cerr << "vertex(" + no_from + ") or (" + no_to + ") not found." << std::endl;
        }
        txn.Abort();
        return ret;
    }

 public:
    BenchmarkLightningGraph(GraphDB& db, size_t vertices) : db(db), vertices(vertices) {}
    double test_write_vertex(size_t count) {
        double time_start = GetTime();
        for (int i = 0; i < count; i++) {
            std::string no = encode_no(vertices++);
            std::string name = random_name();
            write_vertex(no, name);
        }
        double time_taken = GetTime() - time_start;
        return count / time_taken;
    }
    double test_write_edge(size_t count) {
        double time_start = GetTime();
        for (int i = 0; i < count; i++) {
            std::string no_from = random_no();
            std::string no_to = random_no();
            write_edge(no_from, no_to);
        }
        double time_taken = GetTime() - time_start;
        return count / time_taken;
    }
    double test_read_neighbour(size_t count) {
        double time_start = GetTime();
        size_t checksum = 0;
        for (int i = 0; i < count; i++) {
            std::string no = random_no();
            checksum += read_neighbour(no);
        }
        std::cout << "checksum: " << checksum << std::endl;
        double time_taken = GetTime() - time_start;
        return count / time_taken;
    }
    double test_shortest_path(size_t count, int max_hops = 3) {
        double time_start = GetTime();
        size_t checksum = 0;
        for (int i = 0; i < count; i++) {
            std::string no_from = random_no();
            std::string no_to = random_no();
            checksum += shortest_path(no_from, no_to, max_hops);
        }
        std::cout << "checksum: " << checksum << std::endl;
        double time_taken = GetTime() - time_start;
        return count / time_taken;
    }
    double test_read_neighbour_mt(size_t count, size_t num_threads) {
        omp_set_num_threads(num_threads);
        size_t local_count = count / num_threads;
        double throughput = 0;
#pragma omp parallel
        {
            int thread_id = omp_get_thread_num();
            RandomNumberGenerator rng(thread_id);
            double time_start = GetTime();
            size_t checksum = 0;
            for (int i = 0; i < local_count; i++) {
                std::string no = encode_no(rng.next() % vertices);
                checksum += read_neighbour(no);
            }
            assert(checksum != 0);
            double time_taken = GetTime() - time_start;
            double local_throughput = local_count / time_taken;
#pragma omp atomic
            throughput += local_throughput;
        }
        return throughput;
    }
    double test_shortest_path_mt(size_t count, size_t num_threads, int max_hops = 3) {
        omp_set_num_threads(num_threads);
        size_t local_count = count / num_threads;
        double throughput = 0;
#pragma omp parallel
        {
            int thread_id = omp_get_thread_num();
            RandomNumberGenerator rng(thread_id);
            double time_start = GetTime();
            size_t checksum = 0;
            for (int i = 0; i < local_count; i++) {
                std::string no_from = encode_no(rng.next() % vertices);
                std::string no_to = encode_no(rng.next() % vertices);
                checksum += shortest_path(no_from, no_to, max_hops);
            }
            assert(checksum != 0);
            double time_taken = GetTime() - time_start;
            double local_throughput = local_count / time_taken;
#pragma omp atomic
            throughput += local_throughput;
        }
        return throughput;
    }
    double test_write_vertex_batch(size_t count) {
        double time_start = GetTime();
        for (size_t i = 0; i < count; i += batch) {
            Transaction txn = db.CreateWriteTxn();
            for (int j = 0; j < batch; j++) {
                std::string no = encode_no(vertices++);
                std::string name = random_name();
                VertexId vid =
                    txn.AddVertex(std::string("person"), std::vector<std::string>({"no", "name"}),
                                  std::vector<std::string>({no, name}));
            }
            txn.Commit();
        }
        double time_taken = GetTime() - time_start;
        return count / time_taken;
    }
    double test_write_edge_batch(size_t count) {
        double time_start = GetTime();
        for (size_t i = 0; i < count; i += batch) {
            Transaction txn = db.CreateWriteTxn();
            for (int j = 0; j < batch; j++) {
                std::string no_from = random_no();
                std::string no_to = random_no();
                VertexId vid_from;
                VertexId vid_to;
                bool ok = true;
                do {
                    FieldData val_from(no_from);
                    VertexIndexIterator it_from =
                        txn.GetVertexIndexIterator("person", "no", val_from, val_from);
                    if (it_from.IsValid()) {
                        vid_from = it_from.GetVid();
                    } else {
                        ok = false;
                        break;
                    }
                    FieldData val_to(no_to);
                    VertexIndexIterator it_to = txn.GetVertexIndexIterator("person", "no", val_to, val_to);
                    if (it_to.IsValid()) {
                        vid_to = it_to.GetVid();
                    } else {
                        ok = false;
                        break;
                    }
                } while (0);
                if (ok) {
                    txn.AddEdge(vid_from, vid_to, std::string("knows"),
                                std::vector<std::string>({}), std::vector<std::string>({}));
                } else {
                    std::cerr << "insertion of edge(" + no_from + ", " + no_to + ") failed."
                              << std::endl;
                }
            }
            txn.Commit();
        }
        double time_taken = GetTime() - time_start;
        return count / time_taken;
    }
    size_t get_vertices() { return vertices; }
    double warm_up() {
        double time_start = GetTime();
        Transaction txn = db.CreateReadTxn();
        {
            auto vit = txn.GetVertexIterator();
            while (vit.IsValid()) {
                vit.Next();
            }
        }
        txn.Abort();
        double time_taken = GetTime() - time_start;
        return vertices / time_taken;
    }
};

int main(int argc, char** argv) {
    // Simple Benchmark Server
    bool durable = std::atoi(argv[1]);
    size_t vertex_num = std::atol(argv[2]);

    // create GraphDB, cleaning (TODO)
    Galaxy galaxy("lgraph_data", durable, true);
    galaxy.SetCurrentUser("admin", "73@TuGraph");
    auto db = galaxy.OpenGraph("default");

    // add schemas
    assert(db.AddVertexLabel(
        "person", std::vector<FieldSpec>({{"no", STRING, false}, {"name", STRING, false}}), "no"));
    assert(db.AddEdgeLabel("knows", std::vector<FieldSpec>({
                                    })));

    BenchmarkLightningGraph bm(db, 0);

    std::cout << "Start..." << std::endl;

    std::cout << "test_write_vertex: " << bm.test_write_vertex(vertex_num) << std::endl;
    return 0;
}
