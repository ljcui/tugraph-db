#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/post.hpp>
#include "tools/lgraph_log.h"
#include "lgraph/lgraph_exceptions.h"
#include "core/data_type.h"
namespace lgraph {

using lgraph_api::EdgeUid;

lgraph_api::FieldData JsonToFieldData(const nlohmann::json& j_object) {
    if (j_object.is_number_integer()) {
        return lgraph_api::FieldData(j_object.get<int32_t>());
    } else if (j_object.is_string()) {
        return lgraph_api::FieldData(j_object.get<std::string>());
    } else if (j_object.is_number_float()) {
        return lgraph_api::FieldData(j_object.get<float>());
    } else if (j_object.is_boolean()) {
        return lgraph_api::FieldData(j_object.get<bool>());
    }
    throw std::runtime_error("unexpected json type, obj: " + j_object.dump());
};

struct VertexLine {
    size_t vertexLabel;
    size_t primaryFieldId;
    lgraph_api::FieldData primaryFieldValue;
    std::vector<size_t> field_ids;
    std::vector<lgraph_api::FieldData> field_values;
};

bool update_vertex(lgraph_api::GraphDB &db, const std::string &request, std::string &response) {
    using namespace std;
    std::string vertex_type;
    try {
        int add_nodes = 0;
        int update_nodes = 0;
        int filter_nodes = 0;
        nlohmann::json input = nlohmann::json::parse(request);
        vertex_type = input["type"].get<string>();
        string primary_field = input["primary"].get<string>();
        auto vertexes_count = input["vertexes"].size();
        if (vertexes_count == 0) {
            nlohmann::json output;
            output["ok"] = true;
            output["response"]= "No vertexes to update.";
            response = output.dump();
            return true;
        }
        std::vector<VertexLine> lines;
        {
            auto txn = db.CreateReadTxn();
            auto vertexLabel = txn.GetVertexLabelId(vertex_type);
            auto vertexPrimary = txn.GetVertexFieldId(vertexLabel, primary_field);
            for (auto& vertex : input["vertexes"]) {
                try {
                    auto id = JsonToFieldData(vertex[primary_field]);
                    vector<string> field_names;
                    vector<lgraph_api::FieldData> field_values;
                    for (auto& el : vertex.items()) {
                        if (el.key() == primary_field) {
                            continue;
                        }
                        field_names.push_back(el.key());
                        field_values.push_back(JsonToFieldData(el.value()));
                    }
                    auto field_ids = txn.GetVertexFieldIds(vertexLabel, field_names);
                    VertexLine line;
                    line.vertexLabel = vertexLabel;
                    line.primaryFieldId = vertexPrimary;
                    line.primaryFieldValue = id;
                    line.field_ids = std::move(field_ids);
                    line.field_values = std::move(field_values);
                    lines.emplace_back(std::move(line));
                } catch (const exception &e) {
                    continue;
                }
            }
            txn.Abort();
        }

        if (!lines.empty()) {
            auto txn = db.CreateWriteTxn();
            for (auto& line : lines) {
                try {
                    // try to find the vertex and update the property
                    auto vit = txn.GetVertexByUniqueIndex(line.vertexLabel, line.primaryFieldId, line.primaryFieldValue);
                    if (!line.field_ids.empty()) {
                        bool need_update = false;
                        auto fields = vit.GetFields(line.field_ids);
                        for (size_t i = 0; i < fields.size(); i++) {
                            if (fields[i] != line.field_values[i]) {
                                need_update = true;
                                break;
                            }
                        }
                        if (need_update) {
                            vit.SetFields(line.field_ids, line.field_values);
                            update_nodes++;
                        } else {
                            filter_nodes++;
                        }
                    } else {
                        filter_nodes++;
                    }
                } catch (const runtime_error& re) {
                    // not found, insert
                    line.field_ids.push_back(line.primaryFieldId);
                    line.field_values.push_back(line.primaryFieldValue);
                    try {
                        txn.AddVertex(line.vertexLabel, line.field_ids, line.field_values);
                    } catch (const exception& e) {
                        LOG_WARN() << "AddVertex, type: " << vertex_type << ", e: " << e.what() << ", re: " << re.what();
                        throw e;
                    }
                    add_nodes++;
                } catch (const std::exception& e) {
                    LOG_WARN() << "GetVertexByUniqueIndex, type: " << vertex_type << ", e: " << e.what();
                }
            }
            txn.Commit();
        }

        nlohmann::json output;
        output["ok"] = true;
        output["response"]["type"] = vertex_type;
        output["response"]["add_nodes"] = add_nodes;
        output["response"]["update_nodes"] = update_nodes;
        output["response"]["filter_nodes"] = filter_nodes;
        response = output.dump();
        return true;
    } catch (const exception &e) {
        auto err = e.what();
        LOG_WARN() << vertex_type << " update_nodes, exception: " << err;
        nlohmann::json output;
        output["ok"] = false;
        output["response"] = std::string("Error on vertex ") + vertex_type + (" processing: ") + err;
        response = output.dump();
        return false;
    }
}

struct LineEdge {
    int64_t fromVertexId;
    int64_t toVertexId;
    size_t edgeLabelId;
    std::vector<size_t> fieldIds;
    std::vector<FieldData> fieldValues;
};

bool update_edge(lgraph_api::GraphDB &db, const std::string &request, std::string &response) {
    using namespace std;
    std::string label;
    try {
        int updateEdges = 0;
        int insertEdges = 0;
        int filterEdges = 0;
        nlohmann::json input = nlohmann::json::parse(request);
        label = input["type"].get<string>();
        bool repeatable = input["repeatable"].get<bool>();
        string from_label = input["from_label"].get<string>();
        string to_label = input["to_label"].get<string>();

        auto edgesCount = input["edges"].size();
        if (edgesCount == 0) {
            nlohmann::json output;
            output["ok"] = true;
            output["response"]= "No edges to update.";
            response = output.dump();
            return true;
        }
        std::vector<LineEdge> lines;
        {
            auto txn = db.CreateReadTxn();
            auto labelId = txn.GetEdgeLabelId(label);
            for (auto& edge : input["edges"]) {
                try {
                    FieldData fromId = JsonToFieldData(edge["from"]);
                    auto v1iter = txn.GetVertexByUniqueIndex(
                        from_label, txn.GetVertexPrimaryField(from_label), fromId);
                    auto fromVertexId = v1iter.GetId();

                    FieldData toId = JsonToFieldData(edge["to"]);
                    auto v2iter = txn.GetVertexByUniqueIndex(
                        to_label, txn.GetVertexPrimaryField(to_label), toId);
                    auto toVertexId = v2iter.GetId();

                    vector<string> fieldNames;
                    vector<FieldData> fieldValues;
                    for (auto& el : edge["property"].items()) {
                        fieldNames.push_back(el.key());
                        fieldValues.push_back(JsonToFieldData(el.value()));
                    }
                    auto fieldIds = txn.GetEdgeFieldIds(labelId, fieldNames);
                    LineEdge line;
                    line.edgeLabelId = labelId;
                    line.fromVertexId = fromVertexId;
                    line.toVertexId = toVertexId;
                    line.fieldIds = std::move(fieldIds);
                    line.fieldValues = std::move(fieldValues);
                    lines.emplace_back(std::move(line));
                } catch (const exception &e) {
                    continue;
                }
            }
            txn.Abort();
        }

        if (!lines.empty()) {
            auto txn = db.CreateWriteTxn();
            for (auto& line : lines) {
                if (!repeatable) {
                    auto iter = txn.GetOutEdgeIterator(
                        {line.fromVertexId, line.toVertexId, (LabelId)line.edgeLabelId, 0, 0});
                    if (iter.IsValid()) {
                        if (!line.fieldIds.empty()) {
                            bool need_update = false;
                            auto fields = iter.GetFields(line.fieldIds);
                            for (size_t i = 0; i < fields.size(); i++) {
                                if (fields[i] != line.fieldValues[i]) {
                                    need_update = true;
                                    break;
                                }
                            }
                            if (need_update) {
                                iter.SetFields(line.fieldIds, line.fieldValues);
                                updateEdges++;
                            } else {
                                filterEdges++;
                            }
                        } else {
                            filterEdges++;
                        }
                    } else {
                        txn.AddEdge(line.fromVertexId, line.toVertexId, line.edgeLabelId,
                                    line.fieldIds, line.fieldValues);
                        insertEdges++;
                    }
                } else {
                    txn.AddEdge(line.fromVertexId, line.toVertexId, line.edgeLabelId,
                                line.fieldIds, line.fieldValues);
                    insertEdges++;
                }
            }
            txn.Commit();
        }

        nlohmann::json output;
        output["ok"] = true;
        output["response"]["type"] = label;
        output["response"]["add_edges"] = insertEdges;
        output["response"]["filter_edges"] = filterEdges;
        output["response"]["update_edges"] = updateEdges;
        response = output.dump();
        return true;
    } catch (const exception &e) {
        auto err = e.what();
        LOG_WARN() << label << " update_edges, exception: " << err;
        nlohmann::json output;
        output["ok"] = false;
        output["response"] = std::string("Error on edge ") + label +  (" processing: ") + err;
        response = output.dump();
        return false;
    }
}

struct contribution {
    int64_t vid = 0;
    int64_t count = 0;
};

std::vector<contribution> map_to_vec(const std::unordered_map<int64_t, int64_t>& map, size_t top_n) {
    std::vector<contribution> contributions;
    for (const auto& pair : map) {
        contribution con;
        con.vid = pair.first;
        con.count = pair.second;
        contributions.push_back(con);
    }
    std::sort(contributions.begin(), contributions.end(), [](const contribution& a, const contribution& b){
        return a.count > b.count;
    });
    if (contributions.size() > top_n) {
        contributions.resize(top_n);
    }
    return contributions;
}

std::vector<std::tuple<std::string,std::string,std::string>>
get_repo_contribution(lgraph_api::GraphDB &db, const std::string& request) {
    nlohmann::json input = nlohmann::json::parse(request);
    int32_t repo_id = input["repo_id"].get<int32_t>();
    int64_t start_timestamp = input["start_timestamp"].get<int64_t>();
    int64_t end_timestamp = input["end_timestamp"].get<int64_t>();
    int64_t top_n = input["top_n"].get<int64_t>();

    auto txn = db.CreateReadTxn();
    auto vit = txn.GetVertexByUniqueIndex("github_repo", "id", FieldData::Int32(repo_id));
    auto repo_vid = vit.GetId();

    int16_t push_id = txn.GetEdgeLabelId("push");

    int16_t has_issue_id = txn.GetEdgeLabelId("has_issue");
    int16_t open_issue_id = txn.GetEdgeLabelId("open_issue");
    int16_t comment_issue_id = txn.GetEdgeLabelId("comment_issue");

    int16_t has_pr_id = txn.GetEdgeLabelId("has_pr");
    int16_t open_pr_id = txn.GetEdgeLabelId("open_pr");
    int16_t comment_pr_id = txn.GetEdgeLabelId("comment_pr");
    int16_t review_pr_id = txn.GetEdgeLabelId("review_pr");

    EdgeUid has_issue_euid;
    has_issue_euid.lid = has_issue_id;
    EdgeUid open_issue_euid;
    open_issue_euid.lid = open_issue_id;
    EdgeUid comment_issue_euid;
    comment_issue_euid.lid = comment_issue_id;

    EdgeUid has_pr_euid;
    has_pr_euid.lid = has_pr_id;
    EdgeUid open_pr_euid;
    open_pr_euid.lid = open_pr_id;
    EdgeUid review_pr_euid;
    review_pr_euid.lid = review_pr_id;
    EdgeUid comment_pr_euid;
    comment_pr_euid.lid = comment_pr_id;

    std::unordered_map<int64_t, int64_t> push_statistics;
    std::unordered_map<int64_t, int64_t> open_issue_statistics;
    std::unordered_map<int64_t, int64_t> comment_issue_statistics;
    std::unordered_map<int64_t, int64_t> open_pr_statistics;
    std::unordered_map<int64_t, int64_t> review_pr_statistics;
    std::unordered_map<int64_t, int64_t> comment_pr_statistics;
    {
        lgraph_api::EdgeUid push_euid;
        push_euid.lid = push_id;
        for (auto eit = vit.GetInEdgeIterator(push_euid, true); eit.IsValid(); eit.Next()) {
            if (eit.GetLabelId() != push_id) {
                break;
            }
            auto created_at = eit.GetField("created_at").AsInt32();
            if (created_at >= start_timestamp && created_at <= end_timestamp) {
                push_statistics[eit.GetSrc()]++;
            }
        }
    }

    {
        std::vector<int64_t> issue_ids;
        for (auto eit = vit.GetOutEdgeIterator(has_issue_euid, true); eit.IsValid(); eit.Next()) {
            if (eit.GetLabelId() != has_issue_id) {
                break;
            }
            auto created_at = eit.GetField("created_at").AsInt32();
            if (created_at >= start_timestamp && created_at <= end_timestamp) {
                issue_ids.push_back(eit.GetDst());
            }
        }
        for (auto id : issue_ids) {
            auto iter = txn.GetVertexIterator(id);
            for (auto eit = iter.GetInEdgeIterator(open_issue_euid, true); eit.IsValid(); eit.Next()) {
                if (eit.GetLabelId() != open_issue_id) {
                    break;
                }
                open_issue_statistics[eit.GetSrc()]++;
                break;
            }
            for (auto eit = iter.GetInEdgeIterator(comment_issue_euid, true); eit.IsValid(); eit.Next()) {
                if (eit.GetLabelId() != comment_issue_id) {
                    break;
                }
                comment_issue_statistics[eit.GetSrc()]++;
                break;
            }
        }
    }
    {
        std::vector<int64_t> pr_ids;
        for (auto eit = vit.GetOutEdgeIterator(has_pr_euid, true); eit.IsValid(); eit.Next()) {
            if (eit.GetLabelId() != has_pr_id) {
                break;
            }
            auto created_at = eit.GetField("created_at").AsInt32();
            if (created_at >= start_timestamp && created_at <= end_timestamp) {
                pr_ids.push_back(eit.GetDst());
            }
        }
        for (auto id : pr_ids) {
            auto iter = txn.GetVertexIterator(id);
            for (auto eit = iter.GetInEdgeIterator(open_pr_euid, true); eit.IsValid(); eit.Next()) {
                if (eit.GetLabelId() != open_pr_id) {
                    break;
                }
                open_pr_statistics[eit.GetSrc()]++;
                break;
            }
            for (auto eit = iter.GetInEdgeIterator(review_pr_euid, true); eit.IsValid(); eit.Next()) {
                if (eit.GetLabelId() != review_pr_id) {
                    break;
                }
                review_pr_statistics[eit.GetSrc()]++;
            }
            for (auto eit = iter.GetInEdgeIterator(comment_pr_euid, true); eit.IsValid(); eit.Next()) {
                if (eit.GetLabelId() != comment_pr_id) {
                    break;
                }
                comment_pr_statistics[eit.GetSrc()]++;
            }
        }
    }

    std::vector<std::tuple<std::string,std::string,std::string>> ret;
    auto create_response = [top_n,repo_vid,&ret, &txn](const std::string& action, const std::unordered_map<int64_t, int64_t>& statistics){
        int count = 0;
        for (const auto& con : map_to_vec(statistics, top_n)) {
            ret.emplace_back();
            {
                nlohmann::json v_node;
                v_node["id"] = con.vid;
                v_node["type"] = "github_user";
                auto iter = txn.GetVertexIterator(con.vid);
                v_node["properties"]["name"] = iter.GetField("name").AsString();
                std::get<0>(ret.back()) = v_node.dump();
            }
            {
                nlohmann::json v_repl;
                v_repl["id"] = count++;
                v_repl["src"] = con.vid;
                v_repl["dst"] = repo_vid;
                v_repl["type"] = action;
                v_repl["properties"]["count"] = con.count;
                std::get<1>(ret.back()) = v_repl.dump();
            }
            {
                nlohmann::json v_node;
                v_node["id"] = repo_vid;
                v_node["type"] = "github_repo";
                auto iter = txn.GetVertexIterator(repo_vid);
                v_node["properties"]["name"] = iter.GetField("name").AsString();
                std::get<2>(ret.back()) = v_node.dump();
            }
        }
    };
    create_response("push", push_statistics);
    create_response("open_pr", open_pr_statistics);
    create_response("review_pr", review_pr_statistics);
    create_response("comment_pr", comment_pr_statistics);
    create_response("open_issue", open_issue_statistics);
    create_response("comment_issue", comment_issue_statistics);
    return ret;
}
std::vector<std::tuple<std::string, std::string, std::string>>
get_developer_contribution(lgraph_api::GraphDB &db, const std::string& request) {
    nlohmann::json input = nlohmann::json::parse(request);
    int32_t developer_id = input["developer_id"].get<int32_t>();
    int64_t top_n = input["top_n"].get<int64_t>();

    auto txn = db.CreateReadTxn();
    auto vit = txn.GetVertexByUniqueIndex("github_user", "id", FieldData::Int32(developer_id));
    auto developer_vid = vit.GetId();

    int16_t push_id = txn.GetEdgeLabelId("push");

    int16_t has_issue_id = txn.GetEdgeLabelId("has_issue");
    int16_t open_issue_id = txn.GetEdgeLabelId("open_issue");
    int16_t comment_issue_id = txn.GetEdgeLabelId("comment_issue");

    int16_t has_pr_id = txn.GetEdgeLabelId("has_pr");
    int16_t open_pr_id = txn.GetEdgeLabelId("open_pr");
    int16_t comment_pr_id = txn.GetEdgeLabelId("comment_pr");
    int16_t review_pr_id = txn.GetEdgeLabelId("review_pr");

    std::unordered_map<int64_t, int64_t> push_statistics;
    std::unordered_map<int64_t, int64_t> open_issue_statistics;
    std::unordered_map<int64_t, int64_t> comment_issue_statistics;
    std::unordered_map<int64_t, int64_t> open_pr_statistics;
    std::unordered_map<int64_t, int64_t> review_pr_statistics;
    std::unordered_map<int64_t, int64_t> comment_pr_statistics;

    EdgeUid has_issue_euid;
    has_issue_euid.lid = has_issue_id;
    EdgeUid has_pr_euid;
    has_pr_euid.lid = has_pr_id;

    EdgeUid open_issue_euid;
    open_issue_euid.lid = open_issue_id;
    EdgeUid comment_issue_euid;
    comment_issue_euid.lid = comment_issue_id;

    EdgeUid open_pr_euid;
    open_pr_euid.lid = open_pr_id;
    EdgeUid review_pr_euid;
    review_pr_euid.lid = review_pr_id;
    EdgeUid comment_pr_euid;
    comment_pr_euid.lid = comment_pr_id;

    {
        EdgeUid push_euid;
        push_euid.lid = push_id;
        for (auto eit = vit.GetOutEdgeIterator(push_euid, true); eit.IsValid(); eit.Next()) {
            if (eit.GetLabelId() != push_id) {
                break;
            }
            push_statistics[eit.GetDst()]++;
        }
    }

    auto make_statistics = [&vit,&txn](const EdgeUid& in, const EdgeUid& out, std::unordered_map<int64_t, int64_t>& statistics){
        for (auto out_eit = vit.GetOutEdgeIterator(out, true); out_eit.IsValid(); out_eit.Next()) {
            if (out_eit.GetLabelId() != out.lid) {
                break;
            }
            auto iter = txn.GetVertexIterator(out_eit.GetDst());
            for (auto in_eit = iter.GetInEdgeIterator(in, true); in_eit.IsValid(); in_eit.Next()) {
                if (in_eit.GetLabelId() != in.lid) {
                    break;
                }
                statistics[in_eit.GetSrc()]++;
                break;
            }
        }
    };

    make_statistics(has_issue_euid, open_issue_euid, open_issue_statistics);
    make_statistics(has_issue_euid, comment_issue_euid, comment_issue_statistics);

    make_statistics(has_pr_euid, open_pr_euid, open_pr_statistics);
    make_statistics(has_pr_euid, review_pr_euid, review_pr_statistics);
    make_statistics(has_pr_euid, comment_pr_euid, comment_pr_statistics);

    std::vector<std::tuple<std::string, std::string, std::string>> ret;
    auto create_response = [top_n, developer_vid, &txn, &ret](const std::string& action, const std::unordered_map<int64_t, int64_t>& statistics) {
        int count = 0;
        for (const auto& con : map_to_vec(statistics, top_n)) {
            ret.emplace_back();
            {
                nlohmann::json v_node;
                v_node["id"] = developer_vid;
                v_node["type"] = "github_user";
                auto iter = txn.GetVertexIterator(developer_vid);
                v_node["properties"]["name"] = iter.GetField("name").AsString();
                std::get<0>(ret.back()) = v_node.dump();
            }
            {
                nlohmann::json v_repl;
                v_repl["id"] = count++;
                v_repl["src"] = developer_vid;
                v_repl["dst"] = con.vid;
                v_repl["type"] = action;
                v_repl["properties"]["count"] = con.count;
                std::get<1>(ret.back()) = v_repl.dump();
            }
            {
                nlohmann::json v_node;
                v_node["id"] = con.vid;
                v_node["type"] = "github_repo";
                auto iter = txn.GetVertexIterator(con.vid);
                v_node["properties"]["name"] = iter.GetField("name").AsString();
                std::get<2>(ret.back()) = v_node.dump();
            }
        }
    };
    create_response("push", push_statistics);
    create_response("open_pr", open_pr_statistics);
    create_response("review_pr", review_pr_statistics);
    create_response("comment_pr", comment_pr_statistics);
    create_response("open_issue", open_issue_statistics);
    create_response("comment_issue", comment_issue_statistics);

    return ret;
}

std::unordered_map<int64_t,int64_t> get_repos_by_developer(lgraph_api::GraphDB &db, int64_t developer_vid) {
    auto txn = db.CreateReadTxn();
    auto vit = txn.GetVertexIterator(developer_vid);
    bool has_bot = false;
    auto developer_name = vit.GetField("name").AsString();
    if (boost::algorithm::ends_with(developer_name, "[bot]")) {
        return {};
    }
    auto pos = developer_name.find("bot");
    if (pos != std::string::npos) {
        has_bot = true;
    }
    pos = developer_name.find("Bot");
    if (pos != std::string::npos) {
        has_bot = true;
    }

    int16_t has_pr_id = txn.GetEdgeLabelId("has_pr");
    int16_t open_pr_id = txn.GetEdgeLabelId("open_pr");
    int16_t review_pr_id = txn.GetEdgeLabelId("review_pr");
    int16_t comment_pr_id = txn.GetEdgeLabelId("comment_pr");

    int16_t has_issue_id = txn.GetEdgeLabelId("has_issue");
    int16_t open_issue_id = txn.GetEdgeLabelId("open_issue");
    int16_t comment_issue_id = txn.GetEdgeLabelId("comment_issue");

    EdgeUid open_pr_euid;
    open_pr_euid.lid = open_pr_id;
    EdgeUid review_pr_euid;
    review_pr_euid.lid = review_pr_id;
    EdgeUid comment_pr_euid;
    comment_pr_euid.lid = comment_pr_id;
    EdgeUid has_pr_euid;
    has_pr_euid.lid = has_pr_id;

    EdgeUid open_issue_euid;
    open_issue_euid.lid = open_issue_id;
    EdgeUid comment_issue_euid;
    comment_issue_euid.lid = comment_issue_id;
    EdgeUid has_issue_euid;
    has_issue_euid.lid = has_issue_id;

    auto collect_out_vids = [&vit, &developer_name, has_bot](const EdgeUid& euid, std::unordered_map<int64_t, int64_t>& ids){
        int count = 0;
        for (auto eit = vit.GetOutEdgeIterator(euid, true); eit.IsValid(); eit.Next()) {
            if (eit.GetLabelId() != euid.lid) {
                break;
            }
            if (has_bot && ++count > 10000) {
                THROW_CODE(OSGraphInvalidDeveloper, "label: {}, invalid developer: {}", eit.GetLabel(), developer_name);
            }
            ids[eit.GetDst()]++;
        }
    };

    auto collect_in_vids = [&txn, &collect_out_vids](const std::vector<EdgeUid>& out_euids, const EdgeUid& in_euid, std::unordered_map<int64_t, int64_t>& ret_ids){
        std::unordered_map<int64_t, int64_t> ids;
        for (auto& euid : out_euids) {
            collect_out_vids(euid, ids);
        }
        for (auto& pair : ids) {
            auto v_iter = txn.GetVertexIterator(pair.first);
            for (auto eit = v_iter.GetInEdgeIterator(in_euid, true); eit.IsValid(); eit.Next()) {
                if (eit.GetLabelId() != in_euid.lid) {
                    break;
                }
                ret_ids[eit.GetSrc()] += pair.second;
                break;
            }
        }
    };

    //std::set<int64_t> repos;
    std::unordered_map<int64_t,int64_t> repo_contributions;
    try {
        collect_in_vids({open_pr_euid, review_pr_euid, comment_pr_euid}, has_pr_euid, repo_contributions);
        collect_in_vids({open_issue_euid, comment_issue_euid}, has_issue_euid, repo_contributions);
    } catch (lgraph_api::LgraphException& e) {
        if (e.code() == lgraph_api::ErrorCode::OSGraphInvalidDeveloper) {
            LOG_WARN() << e.what();
            return {};
        }
        throw;
    }
    return repo_contributions;
}

std::set<int64_t> get_developers_by_repo(lgraph_api::Transaction &txn, int64_t repo_vid) {
    auto vit = txn.GetVertexIterator(repo_vid);

    int16_t has_issue_id = txn.GetEdgeLabelId("has_issue");
    int16_t open_issue_id = txn.GetEdgeLabelId("open_issue");
    int16_t comment_issue_id = txn.GetEdgeLabelId("comment_issue");

    int16_t has_pr_id = txn.GetEdgeLabelId("has_pr");
    int16_t open_pr_id = txn.GetEdgeLabelId("open_pr");
    int16_t review_pr_id = txn.GetEdgeLabelId("review_pr");
    int16_t comment_pr_id = txn.GetEdgeLabelId("comment_pr");

    EdgeUid has_issue_euid;
    EdgeUid open_issue_euid;
    EdgeUid comment_issue_euid;
    has_issue_euid.lid = has_issue_id;
    open_issue_euid.lid = open_issue_id;
    comment_issue_euid.lid = comment_issue_id;

    EdgeUid has_pr_euid;
    EdgeUid open_pr_euid;
    EdgeUid review_pr_euid;
    EdgeUid comment_pr_euid;
    has_pr_euid.lid = has_pr_id;
    open_pr_euid.lid = open_pr_id;
    review_pr_euid.lid = review_pr_id;
    comment_pr_euid.lid = comment_pr_id;

    auto collect_developer_vids = [&vit,&txn](const EdgeUid& out_euid, const std::vector<EdgeUid>& in_euids, std::set<int64_t>& developers){
        std::vector<int64_t> ids;
        for (auto eit = vit.GetOutEdgeIterator(out_euid, true); eit.IsValid(); eit.Next()) {
            if (eit.GetLabelId() != out_euid.lid) {
                break;
            }
            ids.push_back(eit.GetDst());
        }
        for (auto id : ids) {
            auto iter = txn.GetVertexIterator(id);
            for (auto& euid : in_euids) {
                for (auto eit = iter.GetInEdgeIterator(euid, true); eit.IsValid(); eit.Next()) {
                    if (eit.GetLabelId() != euid.lid) {
                        break;
                    }
                    developers.insert(eit.GetSrc());
                    break;
                }
            }
        }
    };

    std::set<int64_t> developers;
    collect_developer_vids(has_issue_euid, {open_issue_euid, comment_issue_euid}, developers);
    collect_developer_vids(has_pr_euid, {open_pr_euid, review_pr_euid, comment_pr_euid}, developers);
    return developers;
}

struct common_developer {
    int64_t repo_vid1 = 0;
    int64_t repo_vid2 = 0;
    std::vector<int64_t> developers;
};

std::vector<std::tuple<std::string, std::string, std::string>>
get_repo_by_repo(lgraph_api::GraphDB &db, const std::string& request) {
        LOG_INFO() << "get_repo_by_repo begin";
        nlohmann::json input = nlohmann::json::parse(request);
        int64_t repo_id = input["repo_id"].get<int64_t>();
        int64_t top_n = input["top_n"].get<int64_t>();
        std::unordered_map<int64_t, std::set<int64_t>> res;
        auto txn = db.CreateReadTxn();
        auto repo_iter = txn.GetVertexByUniqueIndex("github_repo", "id", FieldData::Int32(repo_id));
        auto repo_vid = repo_iter.GetId();
        LOG_INFO() << "get_developers_by_repo begin";
        auto developers = get_developers_by_repo(txn, repo_vid);
        LOG_INFO() << "get_developers_by_repo end, developers size: " << developers.size();
        res[repo_vid] = developers;
        LOG_INFO() << "get_repos_by_developer begin";
        txn.Abort();
        boost::asio::thread_pool pool1(8);
        std::mutex lock;
        struct local_res {
            local_res(std::mutex& l, std::unordered_map<int64_t, std::set<int64_t>>& g) : lock(l),global(g) {};
            std::unordered_map<int64_t, std::set<int64_t>> local;
            std::mutex& lock;
            std::unordered_map<int64_t, std::set<int64_t>>& global;
            ~local_res() {
                std::lock_guard<std::mutex> guard(lock);
                for (auto& [k ,v] : local) {
                    global[k].merge(v);
                }
            }
        };
        for (auto developer : developers) {
            boost::asio::post(pool1, [developer, &db, &res, &lock](){
                static thread_local local_res local(lock, res);
                auto repos = get_repos_by_developer(db, developer);
                if (repos.empty()) {
                    return;
                }
                for (auto& pair : repos) {
                    local.local[pair.first].insert(developer);
                }
            });
        }
        pool1.join();
        LOG_INFO() << "get_repos_by_developer end, repos size: " << res.size();
        std::vector<common_developer> ret;
        boost::asio::thread_pool pool2(8);
        for (const auto& pair : res) {
            if (pair.first == repo_vid) {
                continue;
            }
            boost::asio::post(pool2, [repo_vid, &developers, &pair, &ret, &lock](){
                common_developer deve;
                deve.repo_vid1 = repo_vid;
                deve.repo_vid2 = pair.first;
                std::set_intersection(developers.begin(), developers.end(),
                                      pair.second.begin(), pair.second.end(),
                                      std::back_inserter(deve.developers));
                std::lock_guard<std::mutex> guard(lock);
                ret.emplace_back(std::move(deve));
            });
        }
        pool2.join();
        std::sort(ret.begin(), ret.end(), [](const common_developer& a, const common_developer& b){
            return a.developers.size() > b.developers.size();
        });
        if (ret.size() > (size_t)top_n)
            ret.resize(top_n);
        LOG_INFO() << "finish to merge common developers";
        std::vector<std::tuple<std::string, std::string, std::string>> records;
        txn = db.CreateReadTxn();
        int count = 0;
        for (auto& item : ret) {
            records.emplace_back();
            {
                nlohmann::json v_node;
                v_node["id"] = item.repo_vid1;
                v_node["type"] = "github_repo";
                auto iter = txn.GetVertexIterator(item.repo_vid1);
                v_node["properties"]["name"] = iter.GetField("name").AsString();
                std::get<0>(records.back()) = v_node.dump();
            }
            {
                nlohmann::json v_repl;
                v_repl["id"] = count++;
                v_repl["src"] = item.repo_vid1;
                v_repl["dst"] = item.repo_vid2;
                v_repl["type"] = "common_developer";
                v_repl["properties"]["count"] = item.developers.size();
                std::get<1>(records.back()) = v_repl.dump();
            }
            {
                nlohmann::json v_node;
                v_node["id"] = item.repo_vid2;
                v_node["type"] = "github_repo";
                auto iter = txn.GetVertexIterator(item.repo_vid2);
                v_node["properties"]["name"] = iter.GetField("name").AsString();
                std::get<2>(records.back()) = v_node.dump();
            }
        }
        txn.Abort();
        return records;
}

struct common_ids {
    int64_t developer_vid1 = 0;
    int64_t developer_vid2 = 0;
    std::vector<int64_t> ids;
};
std::vector<std::tuple<std::string,std::string,std::string>>
get_developer_by_developer(lgraph_api::GraphDB &db, const std::string& request) {
    LOG_INFO() << "get_developer_by_developer begin";
    nlohmann::json input = nlohmann::json::parse(request);
    int32_t developer_id = input["developer_id"].get<int32_t>();
    int64_t top_n = input["top_n"].get<int64_t>();
    auto txn = db.CreateReadTxn();
    auto iter = txn.GetVertexByUniqueIndex("github_user", "id", FieldData::Int32(developer_id));
    auto developer_vid = iter.GetId();

    int16_t has_issue_id = txn.GetEdgeLabelId("has_issue");
    int16_t open_issue_id = txn.GetEdgeLabelId("open_issue");
    int16_t comment_issue_id = txn.GetEdgeLabelId("comment_issue");

    int16_t has_pr_id = txn.GetEdgeLabelId("has_pr");
    int16_t open_pr_id = txn.GetEdgeLabelId("open_pr");
    int16_t review_pr_id = txn.GetEdgeLabelId("review_pr");
    int16_t comment_pr_id = txn.GetEdgeLabelId("comment_pr");

    int16_t star_id = txn.GetEdgeLabelId("star");

    EdgeUid has_issue_euid; has_issue_euid.lid = has_issue_id;
    EdgeUid open_issue_euid; open_issue_euid.lid = open_issue_id;
    EdgeUid comment_issue_euid; comment_issue_euid.lid = comment_issue_id;
    EdgeUid has_pr_euid; has_pr_euid.lid = has_pr_id;
    EdgeUid open_pr_euid; open_pr_euid.lid = open_pr_id;
    EdgeUid review_pr_euid; review_pr_euid.lid = review_pr_id;
    EdgeUid comment_pr_euid; comment_pr_euid.lid = comment_pr_id;
    EdgeUid star_euid; star_euid.lid = star_id;

    std::set<int64_t> issue_ids;
    std::set<int64_t> pr_ids;
    std::set<int64_t> star_repo_ids;
    std::set<int64_t> contri_repo_ids;
    auto get_out_ids = [&iter](const lgraph_api::EdgeUid& euid, std::set<int64_t>& ids){
        for (auto eit = iter.GetOutEdgeIterator(euid, true); eit.IsValid(); eit.Next()) {
            if (eit.GetLabelId() != euid.lid) {
                break;
            }
            ids.insert(eit.GetDst());
        }
    };
    get_out_ids(open_issue_euid, issue_ids);
    get_out_ids(comment_issue_euid, issue_ids);
    get_out_ids(open_pr_euid, pr_ids);
    get_out_ids(review_pr_euid, pr_ids);
    get_out_ids(comment_pr_euid, pr_ids);
    get_out_ids(star_euid, star_repo_ids);

    auto collect_developers = [](lgraph_api::VertexIterator& iter, const lgraph_api::EdgeUid& euid, std::unordered_map<int64_t, std::set<int64_t>>& res){
        for (auto eit = iter.GetInEdgeIterator(euid, true); eit.IsValid(); eit.Next()) {
            if (eit.GetLabelId() != euid.lid) {
                break;
            }
            res[eit.GetSrc()].insert(iter.GetId());
        }
    };

    std::vector<std::tuple<std::string,std::string,std::string>> records;

    auto get_common_ids = [top_n, developer_vid, &records, &txn,&collect_developers, &has_issue_euid, &has_pr_euid, &contri_repo_ids](const std::set<int64_t>& ids, const std::vector<lgraph_api::EdgeUid>& euids, int flag){
        std::unordered_map<int64_t, std::set<int64_t>> tmp;
        for (auto& id : ids) {
            auto v_iter = txn.GetVertexIterator(id);
            for (auto& euid : euids) {
                collect_developers(v_iter, euid, tmp);
            }
            if (flag < 2) {
                auto euid = (flag == 0 ? has_issue_euid : has_pr_euid);
                for (auto eit = v_iter.GetInEdgeIterator(euid, true); eit.IsValid(); eit.Next()) {
                    if (eit.GetLabelId() != euid.lid) {
                        break;
                    }
                    contri_repo_ids.insert(eit.GetSrc());
                    break;
                }
            }
        }
        std::vector<common_ids> common;
        for (auto& pair :  tmp) {
            if (pair.first == developer_vid) {
                continue;
            }
            common_ids com;
            com.developer_vid1 = developer_vid;
            com.developer_vid2 = pair.first;
            std::set_intersection(ids.begin(), ids.end(),
                                  pair.second.begin(), pair.second.end(),
                                  std::back_inserter(com.ids));
            common.emplace_back(std::move(com));
        }
        std::sort(common.begin(), common.end(), [](const common_ids& a, const common_ids& b){
            return a.ids.size() > b.ids.size();
        });
        if (common.size() > (size_t)top_n)
            common.resize(top_n);

        int count = 0;
        for (auto& item : common) {
            records.emplace_back();
            {
                nlohmann::json v_node;
                v_node["id"] = item.developer_vid1;
                v_node["type"] = "github_user";
                auto iter = txn.GetVertexIterator(item.developer_vid1);
                v_node["properties"]["name"] = iter.GetField("name").AsString();
                std::get<0>(records.back()) = v_node.dump();
            }
            {
                nlohmann::json v_repl;
                v_repl["id"] = count++;
                v_repl["src"] = item.developer_vid1;
                v_repl["dst"] = item.developer_vid2;
                if (flag == 0) {
                    v_repl["type"] = "common_issue";
                } else if (flag == 1) {
                    v_repl["type"] = "common_pr";
                } else {
                    v_repl["type"] = "common_star";
                }
                v_repl["properties"]["count"] = item.ids.size();
                std::get<1>(records.back()) = v_repl.dump();
            }
            {
                nlohmann::json v_node;
                v_node["id"] = item.developer_vid2;
                v_node["type"] = "github_user";
                auto iter = txn.GetVertexIterator(item.developer_vid2);
                v_node["properties"]["name"] = iter.GetField("name").AsString();
                std::get<2>(records.back()) = v_node.dump();
            }
        }
    };

    get_common_ids(issue_ids, {open_issue_euid, comment_issue_euid}, 0);
    get_common_ids(pr_ids, {open_pr_euid, comment_pr_euid, review_pr_euid}, 1);
    get_common_ids(star_repo_ids, {star_euid}, 2);

    {
        std::vector<common_ids> common;
        std::unordered_map<int64_t, std::set<int64_t>> developer_repos;
        for (auto id : contri_repo_ids) {
            auto developers = get_developers_by_repo(txn, id);
            for (auto deve_id : developers) {
                developer_repos[deve_id].insert(id);
            }
        }
        for (auto& [deve_id, repo_ids] : developer_repos) {
            if (deve_id == developer_vid) {
                continue;
            }
            auto viter = txn.GetVertexIterator(deve_id);
            auto name = viter.GetField("name").AsString();
            if (boost::algorithm::ends_with(name, "[bot]")) {
                continue;
            }

            common_ids com;
            com.developer_vid1 = developer_vid;
            com.developer_vid2 = deve_id;
            std::set_intersection(contri_repo_ids.begin(), contri_repo_ids.end(),
                                  repo_ids.begin(), repo_ids.end(),
                                  std::back_inserter(com.ids));
            common.emplace_back(std::move(com));
        }
        std::sort(common.begin(), common.end(), [](const common_ids& a, const common_ids& b){
            return a.ids.size() > b.ids.size();
        });
        if (common.size() > (size_t)top_n)
            common.resize(top_n);
        int count = 0;
        nlohmann::json ret = nlohmann::json::array();
        for (auto& item : common) {
            records.emplace_back();
            {
                nlohmann::json v_node;
                v_node["id"] = item.developer_vid1;
                v_node["type"] = "github_user";
                auto viter = txn.GetVertexIterator(item.developer_vid1);
                v_node["properties"]["name"] = viter.GetField("name").AsString();
                std::get<0>(records.back()) = v_node.dump();
            }
            {
                nlohmann::json v_repl;
                v_repl["id"] = count++;
                v_repl["src"] = item.developer_vid1;
                v_repl["dst"] = item.developer_vid2;
                v_repl["type"] = "common_repo";
                v_repl["properties"]["count"] = item.ids.size();
                std::get<1>(records.back()) = v_repl.dump();
            }
            {
                nlohmann::json v_node;
                v_node["id"] = item.developer_vid2;
                v_node["type"] = "github_user";
                auto viter = txn.GetVertexIterator(item.developer_vid2);
                v_node["properties"]["name"] = viter.GetField("name").AsString();
                std::get<2>(records.back()) = v_node.dump();
            }
        }
    }
    return records;
}

std::vector<std::tuple<std::string,std::string,std::string>>
get_repo_developers_profile(lgraph_api::GraphDB &db, const std::string& request) {
    nlohmann::json input = nlohmann::json::parse(request);
    int32_t repo_id = input["repo_id"].get<int32_t>();
    size_t company_topn = input["company_topn"].get<int32_t>();
    size_t country_topn = input["country_topn"].get<int32_t>();
    size_t developer_topn = input["developer_topn"].get<int32_t>();
    auto txn = db.CreateReadTxn();
    auto repo_iter = txn.GetVertexByUniqueIndex("github_repo", "id", FieldData::Int32(repo_id));
    auto repo_vid = repo_iter.GetId();

    int16_t has_pr_id = txn.GetEdgeLabelId("has_pr");
    int16_t star_id = txn.GetEdgeLabelId("star");
    int16_t open_pr_id = txn.GetEdgeLabelId("open_pr");

    std::vector<std::tuple<std::string,std::string,std::string>> records;

    EdgeUid has_pr_euid; has_pr_euid.lid = has_pr_id;
    EdgeUid open_pr_euid; open_pr_euid.lid = open_pr_id;
    std::unordered_map<int64_t, int64_t> developer_prs;
    std::unordered_map<std::string, std::set<int64_t>> company_developers;
    for (auto eit = repo_iter.GetOutEdgeIterator(has_pr_euid, true); eit.IsValid(); eit.Next()) {
        if (eit.GetLabelId() != has_pr_euid.lid) {
            break;
        }
        auto pr_iter = txn.GetVertexIterator(eit.GetDst());
        for (auto open_pr_eit = pr_iter.GetInEdgeIterator(open_pr_euid, true); open_pr_eit.IsValid(); open_pr_eit.Next()) {
            if (open_pr_eit.GetLabelId() != open_pr_euid.lid) {
                break;
            }
            developer_prs[open_pr_eit.GetSrc()]++;
            break;
        }
    }
    for (auto& [d_vid, pr_count] : developer_prs) {
        auto developer_iter = txn.GetVertexIterator(d_vid);
        auto company = developer_iter.GetField("company");
        if (company.IsString()) {
            const auto& val = company.AsString();
            if (!val.empty()) {
                company_developers[val].insert(d_vid);
            }
        }
    }
    std::vector<std::pair<std::string, std::set<int64_t>>> sort_tmp(company_developers.begin(), company_developers.end());
    std::sort(sort_tmp.begin(), sort_tmp.end(),[](const auto& a, const auto& b) {
                  return a.second.size() > b.second.size();
    });
    if (sort_tmp.size() > company_topn) {
        sort_tmp.resize(company_topn);
    }

    int64_t virtual_id = 0;
    std::unordered_map<std::string, int64_t> company_vid;
    for (const auto& pair : sort_tmp) {
        records.emplace_back();
        auto s_vid = --virtual_id;
        company_vid[pair.first] = s_vid;
        auto e_vid = --virtual_id;
        {
            nlohmann::json v_node;
            v_node["id"] = s_vid;
            v_node["type"] = "company";
            v_node["properties"]["name"] = pair.first;
            std::get<0>(records.back()) = v_node.dump();
        }
        {
            nlohmann::json v_repl;
            v_repl["id"] = e_vid;
            v_repl["src"] = s_vid;
            v_repl["dst"] = repo_vid;
            v_repl["type"] = "PR";
            int64_t count = 0;
            for (auto d_vid : pair.second) {
                count += developer_prs.at(d_vid);
            }
            v_repl["properties"]["count"] = count;
            std::get<1>(records.back()) = v_repl.dump();
        }
        {
            nlohmann::json v_node;
            v_node["id"] = repo_vid;
            v_node["type"] = "github_repo";
            v_node["properties"]["name"] = repo_iter.GetField("name").AsString();
            std::get<2>(records.back()) = v_node.dump();
        }
    }


    std::unordered_map<std::string, std::set<int64_t>> country_developers;
    EdgeUid star_euid; star_euid.lid = star_id;
    for (auto eit = repo_iter.GetInEdgeIterator(star_euid, true); eit.IsValid(); eit.Next()) {
        if (eit.GetLabelId() != star_euid.lid) {
            break;
        }
        auto developer_iter = txn.GetVertexIterator(eit.GetSrc());
        auto country = developer_iter.GetField("country");
        if (country.IsString()) {
            const auto& val = country.AsString();
            if (!val.empty()) {
                country_developers[val].insert(developer_iter.GetId());
            }
        }
    }
    std::vector<std::pair<std::string, std::set<int64_t>>> sort_tmp1(country_developers.begin(), country_developers.end());
    std::sort(sort_tmp1.begin(), sort_tmp1.end(),[](const auto& a, const auto& b) {
        return a.second.size() > b.second.size();
    });
    if (sort_tmp1.size() > country_topn) {
        sort_tmp1.resize(country_topn);
    }

    std::unordered_map<std::string, int64_t> country_vid;
    for (auto& [country, vids] : sort_tmp1) {
        records.emplace_back();
        auto s_vid = --virtual_id;
        auto e_vid = --virtual_id;
        country_vid[country] = s_vid;
        {
            nlohmann::json v_node;
            v_node["id"] = s_vid;
            v_node["type"] = "country";
            v_node["properties"]["name"] = country;
            std::get<0>(records.back()) = v_node.dump();
        }
        {
            nlohmann::json v_repl;
            v_repl["id"] = e_vid;
            v_repl["src"] = s_vid;
            v_repl["dst"] = repo_vid;
            v_repl["type"] = "Star";
            v_repl["properties"]["count"] = vids.size();
            std::get<1>(records.back()) = v_repl.dump();
        }
        {
            nlohmann::json v_node;
            v_node["id"] = repo_vid;
            v_node["type"] = "github_repo";
            v_node["properties"]["name"] = repo_iter.GetField("name").AsString();
            std::get<2>(records.back()) = v_node.dump();
        }
    }

    std::vector<std::pair<int64_t, int64_t>> sort_tmp2 (developer_prs.begin(), developer_prs.end());
    std::sort(sort_tmp2.begin(), sort_tmp2.end(),[](const auto& a, const auto& b) {
        return a.second > b.second;
    });
    std::unordered_map<std::string, std::set<int64_t>> country_core_developers;
    for (auto& [d_vid, count_pr] : sort_tmp2) {
        for (auto& pair : sort_tmp1) {
            if (pair.second.count(d_vid)) {
                if (country_core_developers[pair.first].size() < developer_topn) {
                    country_core_developers[pair.first].insert(d_vid);
                }
                break;
            }
        }
    }
    for (auto& [country, developers] : country_core_developers) {
        for (auto& d_vid : developers) {
            records.emplace_back();
            {
                nlohmann::json v_node;
                v_node["id"] = d_vid;
                v_node["type"] = "github_user";
                auto d_iter = txn.GetVertexIterator(d_vid);
                v_node["properties"]["name"] = d_iter.GetField("name").AsString();
                std::get<0>(records.back()) = v_node.dump();
            }
            {
                auto e_vid = --virtual_id;
                nlohmann::json v_repl;
                v_repl["id"] = e_vid;
                v_repl["src"] = d_vid;
                v_repl["dst"] = country_vid.at(country);
                v_repl["type"] = "belong_to";
                v_repl["properties"] = nlohmann::json::object();
                std::get<1>(records.back()) = v_repl.dump();
            }
            {
                nlohmann::json v_node;
                v_node["id"] = country_vid.at(country);
                v_node["type"] = "country";
                v_node["properties"]["name"] = country;
                std::get<2>(records.back()) = v_node.dump();
            }
        }
    }
    for (auto& [country, developers] : country_core_developers) {
        for (auto& d_vid : developers) {
            for (auto& item :  sort_tmp) {
                if (item.second.count(d_vid)) {
                    records.emplace_back();
                    {
                        nlohmann::json v_node;
                        v_node["id"] = d_vid;
                        v_node["type"] = "github_user";
                        auto d_iter = txn.GetVertexIterator(d_vid);
                        v_node["properties"]["name"] = d_iter.GetField("name").AsString();
                        std::get<0>(records.back()) = v_node.dump();
                    }
                    {
                        auto e_vid = --virtual_id;
                        nlohmann::json v_repl;
                        v_repl["id"] = e_vid;
                        v_repl["src"] = d_vid;
                        v_repl["dst"] = company_vid.at(item.first);
                        v_repl["type"] = "belong_to";
                        v_repl["properties"] = nlohmann::json::object();
                        std::get<1>(records.back()) = v_repl.dump();
                    }
                    {
                        nlohmann::json v_node;
                        v_node["id"] = company_vid.at(item.first);
                        v_node["type"] = "company";
                        v_node["properties"]["name"] = item.first;
                        std::get<2>(records.back()) = v_node.dump();
                    }
                    break;
                }
            }
        }
    }

    return records;
}

std::vector<std::tuple<std::string,std::string,std::string>>
get_developer_repos_profile(lgraph_api::GraphDB &db, const std::string& request) {
    nlohmann::json input = nlohmann::json::parse(request);
    int32_t developer_id = input["developer_id"].get<int32_t>();
    size_t topic_topn = input["topic_topn"].get<int32_t>();
    size_t repo_topn = input["repo_topn"].get<int32_t>();
    auto txn = db.CreateReadTxn();
    auto developer_iter = txn.GetVertexByUniqueIndex("github_user", "id", FieldData::Int32(developer_id));
    auto developer_vid = developer_iter.GetId();
    auto developer_name = developer_iter.GetField("name").AsString();
    txn.Abort();
    auto repos = get_repos_by_developer(db, developer_vid);

    std::unordered_map<std::string, std::set<int64_t>> topic_repos;
    txn = db.CreateReadTxn();
    int16_t has_topic_id = txn.GetEdgeLabelId("has_topic");
    EdgeUid has_topic_euid; has_topic_euid.lid = has_topic_id;
    for (auto& pair : repos) {
        auto iter = txn.GetVertexIterator(pair.first);
        for (auto eit = iter.GetOutEdgeIterator(has_topic_euid, true); eit.IsValid(); eit.Next()) {
            if (eit.GetLabelId() != has_topic_euid.lid) {
                break;
            }
            auto topic_vid = eit.GetDst();
            auto topic_iter = txn.GetVertexIterator(topic_vid);
            auto topic = topic_iter.GetField("name").AsString();
            topic_repos[topic].insert(pair.first);
        }
    }
    std::vector<std::pair<std::string, std::set<int64_t>>> sort_temp(topic_repos.begin(), topic_repos.end());
    std::sort(sort_temp.begin(), sort_temp.end(),[](const auto& a, const auto& b) {
        return a.second.size() > b.second.size();
    });
    if (sort_temp.size() > topic_topn) {
        sort_temp.resize(topic_topn);
    }
    int64_t virtual_id = 0;
    std::vector<std::tuple<std::string,std::string,std::string>> records;
    std::unordered_map<std::string, int64_t> topic_vid;
    for (auto& [topic, repo_vids] : sort_temp) {
        records.emplace_back();
        auto eid = --virtual_id;
        auto dst_id = --virtual_id;
        topic_vid[topic] = dst_id;
        {
            nlohmann::json v_node;
            v_node["id"] = developer_vid;
            v_node["type"] = "github_user";
            v_node["properties"]["name"] = developer_name;
            std::get<0>(records.back()) = v_node.dump();
        }
        {
            nlohmann::json v_repl;
            v_repl["id"] = eid;
            v_repl["src"] = developer_vid;
            v_repl["dst"] = dst_id;
            v_repl["type"] = "repo";
            v_repl["properties"]["count"] = repo_vids.size();
            std::get<1>(records.back()) = v_repl.dump();
        }
        {
            nlohmann::json v_node;
            v_node["id"] = dst_id;
            v_node["type"] = "topic";
            v_node["properties"]["name"] = topic;
            std::get<2>(records.back()) = v_node.dump();
        }
    }
    std::vector<std::pair<int64_t, int64_t>> sort_temp1(repos.begin(), repos.end());
    std::sort(sort_temp1.begin(), sort_temp1.end(),[](const auto& a, const auto& b) {
        return a.second > b.second;
    });
    if (sort_temp1.size() > repo_topn) {
        sort_temp1.resize(repo_topn);
    }
    for (auto& [repo_vid, score] : sort_temp1) {
        for (auto& [topic, repo_vids] : sort_temp) {
            if (repo_vids.count(repo_vid)) {
                records.emplace_back();
                auto eid = --virtual_id;
                {
                    nlohmann::json v_node;
                    v_node["id"] = repo_vid;
                    v_node["type"] = "github_repo";
                    auto repo_iter = txn.GetVertexIterator(repo_vid);
                    v_node["properties"]["name"] = repo_iter.GetField("name").AsString();
                    std::get<0>(records.back()) = v_node.dump();
                }
                {
                    nlohmann::json v_repl;
                    v_repl["id"] = eid;
                    v_repl["src"] = repo_vid;
                    v_repl["dst"] = topic_vid.at(topic);
                    v_repl["type"] = "belong_to";
                    v_repl["properties"] = nlohmann::json::object();
                    std::get<1>(records.back()) = v_repl.dump();
                }
                {
                    nlohmann::json v_node;
                    v_node["id"] = topic_vid.at(topic);
                    v_node["type"] = "topic";
                    v_node["properties"]["name"] = topic;
                    std::get<2>(records.back()) = v_node.dump();
                }
            }
        }
    }

    return records;
}

bool export_es_data(lgraph_api::GraphDB &db, const std::string &request, std::string &response) {
    using namespace std;
    try {
        /*{
            ofstream github_user;
            github_user.open("github_user.txt");
            auto txn = db.CreateReadTxn();
            auto iiter = txn.GetVertexIndexIterator("github_user", "id", "", "");
            for (; iiter.IsValid(); iiter.Next()) {
                auto viter = txn.GetVertexIterator(iiter.GetVid());
                nlohmann::json line;
                auto all_fds = viter.GetAllFields();
                line["id"] = all_fds.at("id").AsInt32();
                line["name"] = all_fds.at("name").AsString();
                github_user << line.dump() << "\n";
            }
            github_user.close();
        }*/
        {
            ofstream github_repo;
            github_repo.open("github_repo.txt");
            auto txn = db.CreateReadTxn();
            EdgeUid star_euid;
            star_euid.lid = txn.GetEdgeLabelId("star");
            auto iiter = txn.GetVertexIndexIterator("github_repo", "id", "", "");
            for (; iiter.IsValid(); iiter.Next()) {
                auto viter = txn.GetVertexIterator(iiter.GetVid());
                int32_t count = 0;
                for (auto eit = viter.GetInEdgeIterator(star_euid, true); eit.IsValid(); eit.Next()) {
                    if (eit.GetLabelId() != star_euid.lid) {
                        break;
                    }
                    count++;
                }
                nlohmann::json line;
                auto all_fds = viter.GetAllFields();
                line["id"] = all_fds.at("id").AsInt32();
                line["name"] = all_fds.at("name").AsString();
                line["star"] = count;
                github_repo << line.dump() << "\n";
            }
            github_repo.close();
        }
        return true;
    } catch (const exception &e) {
        auto err = e.what();
        LOG_WARN() << "export_es_data, exception: " << err;
        nlohmann::json output;
        output["ok"] = false;
        output["response"] = std::string("Error on export_es_data: ") + err;
        response = output.dump();
        return false;
    }
}

}