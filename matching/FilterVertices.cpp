#include "FilterVertices.h"
#include "GenerateFilteringPlan.h"
#include <memory.h>
#include <utility/graphoperations.h>
#include "primitive/scan.h"
#include "primitive/nlf_filter.h"
#include "primitive/semi_join.h"
#include "primitive/projection.h"
#include "bi_matching/bigraphMatching.h"
#include <vector>
#include <bitset>
#include <algorithm>
#include <chrono>
#include <set>
#define INVALID_VERTEX_ID 4294967295
#define NANOSECTOMILLISEC(elapsed_time) ((elapsed_time)/(double)1000000)

bool 
FilterVertices::LDFFilter(const Graph *data_graph, const Graph *query_graph, ui **&candidates, ui *&candidates_count) {
    allocateBuffer(data_graph, query_graph, candidates, candidates_count);

    for (ui i = 0; i < query_graph->getVerticesCount(); ++i) {
        LabelID label = query_graph->getVertexLabel(i);
        ui degree = query_graph->getVertexDegree(i);

        ui data_vertex_num;
        const ui* data_vertices = data_graph->getVerticesByLabel(label, data_vertex_num);

        for (ui j = 0; j < data_vertex_num; ++j) {
            ui data_vertex = data_vertices[j];
            if (data_graph->getVertexDegree(data_vertex) >= degree) {
                candidates[i][candidates_count[i]++] = data_vertex;
            }
        }

        if (candidates_count[i] == 0) {
            return false;
        }
    }

    return true;
}

bool 
FilterVertices::NLFFilter(const Graph *data_graph, const Graph *query_graph, ui **&candidates, ui *&candidates_count) {
    allocateBuffer(data_graph, query_graph, candidates, candidates_count);

    for (ui i = 0; i < query_graph->getVerticesCount(); ++i) {
        VertexID query_vertex = i;
        computeCandidateWithNLF(data_graph, query_graph, query_vertex, candidates_count[query_vertex], candidates[query_vertex]);

        if (candidates_count[query_vertex] == 0) {
            return false;
        }
    }

    return true;
}


bool
FilterVertices::GQLFilter(const Graph *data_graph, const Graph *query_graph, ui **&candidates, ui *&candidates_count) {
    
    if (!NLFFilter(data_graph, query_graph, candidates, candidates_count))
        return false;

    
    ui query_vertices_num = query_graph->getVerticesCount();
    ui data_vertex_num = data_graph->getVerticesCount();

    bool** valid_candidates = new bool*[query_vertices_num];
    for (ui i = 0; i < query_vertices_num; ++i) {
        valid_candidates[i] = new bool[data_vertex_num];
        memset(valid_candidates[i], 0, sizeof(bool) * data_vertex_num);
    }

    ui query_graph_max_degree = query_graph->getGraphMaxDegree();
    ui data_graph_max_degree = data_graph->getGraphMaxDegree();

    int* left_to_right_offset = new int[query_graph_max_degree + 1];
    int* left_to_right_edges = new int[query_graph_max_degree * data_graph_max_degree];
    int* left_to_right_match = new int[query_graph_max_degree];
    int* right_to_left_match = new int[data_graph_max_degree];
    int* match_visited = new int[data_graph_max_degree + 1];
    int* match_queue = new int[query_vertices_num];
    int* match_previous = new int[data_graph_max_degree + 1];

    
    for (ui i = 0; i < query_vertices_num; ++i) {
        VertexID query_vertex = i;
        for (ui j = 0; j < candidates_count[query_vertex]; ++j) {
            VertexID data_vertex = candidates[query_vertex][j];
            valid_candidates[query_vertex][data_vertex] = true;
        }
    }

    
    for (ui l = 0; l < 2; ++l) {
        for (ui i = 0; i < query_vertices_num; ++i) {
            VertexID query_vertex = i;
            for (ui j = 0; j < candidates_count[query_vertex]; ++j) {
                VertexID data_vertex = candidates[query_vertex][j];

                if (data_vertex == INVALID_VERTEX_ID)
                    continue;

                if (!verifyExactTwigIso(data_graph, query_graph, data_vertex, query_vertex, valid_candidates,
                                        left_to_right_offset, left_to_right_edges, left_to_right_match,
                                        right_to_left_match, match_visited, match_queue, match_previous)) {
                    candidates[query_vertex][j] = INVALID_VERTEX_ID;
                    valid_candidates[query_vertex][data_vertex] = false;
                }
            }
        }
    }
    
    compactCandidates(candidates, candidates_count, query_vertices_num);
    
    for (ui i = 0; i < query_vertices_num; ++i) {
        delete[] valid_candidates[i];
    }
    delete[] valid_candidates;
    delete[] left_to_right_offset;
    delete[] left_to_right_edges;
    delete[] left_to_right_match;
    delete[] right_to_left_match;
    delete[] match_visited;
    delete[] match_queue;
    delete[] match_previous;

    return isCandidateSetValid(candidates, candidates_count, query_vertices_num);
}

bool
FilterVertices::CFLFilter(const Graph *data_graph, const Graph *query_graph, ui **&candidates, ui *&candidates_count,
                          ui *&order, TreeNode *&tree) {
    allocateBuffer(data_graph, query_graph, candidates, candidates_count);
    int level_count;
    ui* level_offset;
    GenerateFilteringPlan::generateCFLFilterPlan(data_graph, query_graph, tree, order, level_count, level_offset);

    VertexID start_vertex = order[0];
    computeCandidateWithNLF(data_graph, query_graph, start_vertex, candidates_count[start_vertex], candidates[start_vertex]);

    ui* updated_flag = new ui[data_graph->getVerticesCount()];
    ui* flag = new ui[data_graph->getVerticesCount()];
    std::fill(flag, flag + data_graph->getVerticesCount(), 0);

    
    for (int i = 1; i < level_count; ++i) {
        
        for (ui j = level_offset[i]; j < level_offset[i + 1]; ++j) {
            VertexID query_vertex = order[j];
            TreeNode& node = tree[query_vertex];
            
            
            generateCandidates(data_graph, query_graph, query_vertex, node.bn_, node.bn_count_, candidates, candidates_count, flag, updated_flag);
        }

        
        for (int j = level_offset[i + 1] - 1; j >= level_offset[i]; --j) {
            VertexID query_vertex = order[j];
            TreeNode& node = tree[query_vertex];

            if (node.fn_count_ > 0) {
                pruneCandidates(data_graph, query_graph, query_vertex, node.fn_, node.fn_count_, candidates, candidates_count, flag, updated_flag);
            }
        }
    }

    
    for (int i = level_count - 2; i >= 0; --i) {
        for (ui j = level_offset[i]; j < level_offset[i + 1]; ++j) {
            VertexID query_vertex = order[j];
            TreeNode& node = tree[query_vertex];

            if (node.under_level_count_ > 0) {
                pruneCandidates(data_graph, query_graph, query_vertex, node.under_level_, node.under_level_count_, candidates, candidates_count, flag, updated_flag);
            }
        }
    }


    compactCandidates(candidates, candidates_count, query_graph->getVerticesCount());

    delete[] updated_flag;
    delete[] flag;
    return isCandidateSetValid(candidates, candidates_count, query_graph->getVerticesCount());
}

bool
FilterVertices::DPisoFilter(const Graph *data_graph, const Graph *query_graph, ui **&candidates, ui *&candidates_count,
                            ui *&order, TreeNode *&tree) {
    if (!LDFFilter(data_graph, query_graph, candidates, candidates_count))
        return false;

    GenerateFilteringPlan::generateDPisoFilterPlan(data_graph, query_graph, tree, order);

    ui query_vertices_num = query_graph->getVerticesCount();
    ui* updated_flag = new ui[data_graph->getVerticesCount()];
    ui* flag = new ui[data_graph->getVerticesCount()];
    std::fill(flag, flag + data_graph->getVerticesCount(), 0);

    for (ui k = 0; k < 3; ++k) {
        if (k % 2 == 0) {
            for (ui i = 1; i < query_vertices_num; ++i) {
                VertexID query_vertex = order[i];
                TreeNode& node = tree[query_vertex];
                pruneCandidates(data_graph, query_graph, query_vertex, node.bn_, node.bn_count_, candidates, candidates_count, flag, updated_flag);
            }
        }
        else {
            for (int i = query_vertices_num - 2; i >= 0; --i) {
                VertexID query_vertex = order[i];
                TreeNode& node = tree[query_vertex];
                pruneCandidates(data_graph, query_graph, query_vertex, node.fn_, node.fn_count_, candidates, candidates_count, flag, updated_flag);
            }
        }
    }

    compactCandidates(candidates, candidates_count, query_graph->getVerticesCount());

    delete[] updated_flag;
    delete[] flag;
    return isCandidateSetValid(candidates, candidates_count, query_graph->getVerticesCount());
}

struct VectorHash {
    size_t operator()(const std::vector<VertexID>& v) const {
        size_t seed = v.size();
        for(auto& i : v) {
            seed ^= i + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};

void 
FilterVertices::compute_equal_set_base(const Graph *data_graph, VertexID query_vertex, ui **candidates, 
                        ui *candidates_count, ui **eqv_classes, ui **eqv_offsets, ui **eqv_counts, boost::dynamic_bitset<> &neighbor_valid_candiate_flag) {    
    
    std::unordered_map<std::vector<VertexID>, std::vector<VertexID>, VectorHash> vnbr2eqv;
    for (ui i=0; i<candidates_count[query_vertex]; ++i){
        VertexID data_vertex = candidates[query_vertex][i];
        ui data_neighbors_num;
        const VertexID* data_vertex_neighbors = data_graph->getVertexNeighbors(data_vertex, data_neighbors_num);
        std::vector<VertexID> vnbr;
        vnbr.reserve(data_neighbors_num);         
        for (ui j=0; j<data_neighbors_num; ++j){
            VertexID data_neighbor = data_vertex_neighbors[j];
            if (neighbor_valid_candiate_flag.test(data_neighbor)){
                vnbr.emplace_back(data_neighbor);
            }
        }        
        vnbr2eqv[vnbr].emplace_back(data_vertex);
    }
    
    ui candidate_count = 0; 
    ui eqv_count = 0; 
    ui eqv_offset = 0; 
    eqv_offsets[query_vertex][0] = 0; 
    
    std::vector<const std::vector<VertexID>*> value_ptrs;
    value_ptrs.reserve(vnbr2eqv.size());
    for (const auto& pair : vnbr2eqv) {
        value_ptrs.push_back(&(pair.second));
    }
    
    std::sort(value_ptrs.begin(), value_ptrs.end(), 
        [](const std::vector<VertexID>* a, const std::vector<VertexID>* b) {
            return a->size() > b->size(); 
        }
    );
    
    for (const auto* val_ptr : value_ptrs) {
        eqv_offset += val_ptr->size();
        for (const VertexID v : *val_ptr) {
            eqv_classes[query_vertex][eqv_count++] = v;
        }
        eqv_counts[query_vertex][candidate_count] = val_ptr->size(); 
        candidates[query_vertex][candidate_count] = (*val_ptr)[0]; 
        eqv_offsets[query_vertex][++candidate_count] = eqv_offset; 
    }
    candidates_count[query_vertex] = vnbr2eqv.size();
}

void
FilterVertices::get_equivanlence_base(const Graph *data_graph, const Graph *query_graph, ui **candidates, ui *candidates_count,
                                ui **&eqv_classes, ui **&eqv_offsets, ui **&eqv_counts){
    ui query_vertices_num = query_graph->getVerticesCount();
    ui data_vertices_num = data_graph->getVerticesCount();
    std::vector<boost::dynamic_bitset<>> valid_candidate_flag(
        query_graph->getVerticesCount(), 
        boost::dynamic_bitset<>(data_vertices_num) // 为每个位图分配精确的堆内存
    );
    boost::dynamic_bitset<> neighbor_valid_candiate_flag(data_vertices_num);
    eqv_classes = new ui*[query_vertices_num];
    eqv_offsets = new ui*[query_vertices_num];
    eqv_counts = new ui*[query_vertices_num];
    for (ui i = 0; i < query_vertices_num; ++i) {
        eqv_classes[i] = new ui[candidates_count[i]];
        eqv_offsets[i] = new ui[candidates_count[i] + 1];
        eqv_counts[i] = new ui[candidates_count[i]];
        for (ui j = 0; j < candidates_count[i]; ++j) {
            valid_candidate_flag[i].set(candidates[i][j]);
        }
    }
    for (ui i = 0; i < query_vertices_num; ++i) {
        VertexID query_vertex = i;
        ui query_neighbors_num;
        const VertexID* query_vertex_neighbors = query_graph->getVertexNeighbors(query_vertex, query_neighbors_num);
        neighbor_valid_candiate_flag.reset();
        for (ui j = 0; j < query_neighbors_num; ++j) {
            VertexID query_neighbor = query_vertex_neighbors[j];
            neighbor_valid_candiate_flag |= valid_candidate_flag[query_neighbor];
        }
        FilterVertices::compute_equal_set_base(data_graph, query_vertex , candidates, candidates_count, eqv_classes, eqv_offsets, eqv_counts, neighbor_valid_candiate_flag);
    }
}

bool FilterVertices::MergeEquivalenceClasses(const Graph *data_graph, const Graph *query_graph, 
                                             ui **candidates, ui *candidates_count,
                                             ui **eqv_classes, ui **eqv_offsets, ui **eqv_counts, 
                                             bool **valid_candidates) {
    ui query_vertices_num = query_graph->getVerticesCount();
    ui max_eqv_size = 0, max_cand_size = 0;
    for (ui i = 0; i < query_vertices_num; ++i) {
        if (candidates_count[i] == 0) return false;
        max_eqv_size = max(max_eqv_size, eqv_offsets[i][candidates_count[i] - 1] + eqv_counts[i][candidates_count[i] - 1]);
        max_cand_size = max(max_cand_size, candidates_count[i]);
    }
    ui *new_eqv_classes = new ui[max_eqv_size];
    ui *new_eqv_offsets = new ui[max_cand_size];
    ui *new_eqv_counts = new ui[max_cand_size];
    ui *new_candidates = new ui[max_cand_size];
    for (ui i = 0; i < query_vertices_num; ++i) {
        VertexID u = i;        
        ui unbr_cnt;
        const VertexID* unbrs = query_graph->getVertexNeighbors(u, unbr_cnt);
        using MergedEQV = pair<ui, std::vector<VertexID>>;
        std::unordered_map<std::vector<VertexID>, MergedEQV, VectorHash> sig2reps;
        for (ui j = 0; j < candidates_count[u]; ++j) {
            VertexID rep_v = candidates[u][j];
            std::vector<VertexID> signature;            
            ui vnbr_cnt;
            const VertexID* vnbrs = data_graph->getVertexNeighbors(rep_v, vnbr_cnt);
            for (ui l = 0; l < vnbr_cnt; ++l) {
                VertexID vnbr = vnbrs[l];
                for (ui k = 0; k < unbr_cnt; ++k) {
                    VertexID unbr = unbrs[k];
                    if (valid_candidates[unbr][vnbr]) {
                        signature.push_back(vnbr);
                        break;
                    }
                }
            }
            sig2reps[signature].first += eqv_counts[u][j];
            sig2reps[signature].second.push_back(j);
        }
        std::vector<const MergedEQV*> value_ptrs;
        for (const auto& pair : sig2reps) {
            value_ptrs.push_back(&(pair.second));
        }
        std::sort(value_ptrs.begin(), value_ptrs.end(), 
            [](const MergedEQV* a, const MergedEQV* b) {
                return a->first > b->first; 
            }
        );
        ui new_cand_count = 0;
        ui current_total_offset = 0;
        for (const auto* val_ptr : value_ptrs) {
            const std::vector<ui>& old_rep_indices = val_ptr->second;
            ui first_old_idx = old_rep_indices[0];
            new_candidates[new_cand_count] = candidates[u][first_old_idx];
            new_eqv_offsets[new_cand_count] = current_total_offset;
            ui merged_count = 0;
            for (ui old_idx : old_rep_indices) {
                ui old_offset = eqv_offsets[u][old_idx];
                ui old_count = eqv_counts[u][old_idx];
                std::copy(eqv_classes[u] + old_offset, eqv_classes[u] + old_offset + old_count, new_eqv_classes + current_total_offset);
                current_total_offset += old_count;
                merged_count += old_count;
            }            
            new_eqv_counts[new_cand_count] = merged_count;
            new_cand_count++;
        }
        candidates_count[u] = new_cand_count;
        eqv_offsets[u][new_cand_count] = current_total_offset;
        std::copy(new_candidates, new_candidates + new_cand_count, candidates[u]);
        std::copy(new_eqv_offsets, new_eqv_offsets + new_cand_count , eqv_offsets[u]);
        std::copy(new_eqv_classes, new_eqv_classes + current_total_offset, eqv_classes[u]);
        std::copy(new_eqv_counts, new_eqv_counts + new_cand_count, eqv_counts[u]);
    }
    delete[] new_eqv_classes;
    delete[] new_eqv_offsets;
    delete[] new_eqv_counts;
    delete[] new_candidates;
    return true;
}

bool 
FilterVertices::VSSimFilterInitial(const Graph *data_graph, const Graph *query_graph, ui **&candidates, ui *&candidates_count,
                                  ui *order, TreeNode *tree) {
    allocateBuffer(data_graph, query_graph, candidates, candidates_count);

    ui query_vertices_num = query_graph->getVerticesCount();
    ui data_vertices_num = data_graph->getVerticesCount();

    VertexID root = order[0];
    LabelID label = query_graph->getVertexLabel(root);
    ui degree = query_graph->getVertexDegree(root);
    ui *root_cands = candidates[root];
    #ifdef VSSIM_NLF
    auto root_nlf = query_graph->getVertexNLF(root);
    #endif
    ui label_cans_num;
    const ui* label_cans = data_graph->getVerticesByLabel(label, label_cans_num);
    ui count = 0;
    for (ui j = 0; j < label_cans_num; ++j) {
        ui data_vertex = label_cans[j];
        if (data_graph->getVertexDegree(data_vertex) < degree)break;
        #ifndef VSSIM_NLF
        root_cands[count++] = data_vertex;
        #else
        auto data_vertex_nlf = data_graph->getVertexNLF(data_vertex);
        if (data_vertex_nlf->size() >= root_nlf->size()) {
            bool is_valid = true;
            for (auto element : *root_nlf) {
                auto iter = data_vertex_nlf->find(element.first);
                if (iter == data_vertex_nlf->end() || iter->second < element.second) {
                    is_valid = false;
                    break;
                }
            }
            if (is_valid) root_cands[count++] = data_vertex;
        }
        #endif
    }
    candidates_count[root] = count;
    if (candidates_count[root] == 0) return false;
    std::cout << "Initial candidate size for root vertex " << root << ": " << candidates_count[root] << std::endl;

    ui* flag = new ui[data_vertices_num]();
    ui* updated_flag = new ui[data_vertices_num]();

    for (ui i = 1; i < query_vertices_num; ++i) {
        VertexID u = order[i];

        LabelID curr_label = query_graph->getVertexLabel(u);
        ui curr_degree = query_graph->getVertexDegree(u);

        TreeNode& node = tree[u];
        ui check_val = 0;
        ui updated_flag_count = 0;
        #ifdef VSSIM_NLF
        auto u_nlf = query_graph->getVertexNLF(u);
        #endif

        for (ui x = 0; x < node.bn_count_; ++x) {
            VertexID parent = node.bn_[x];

            for (ui y = 0; y < candidates_count[parent]; ++y) {
                VertexID parent_cand = candidates[parent][y];
                ui nbrs_cnt;
                auto nbrs = data_graph->getVertexNeighbors(parent_cand, nbrs_cnt);

                for (ui z = 0; z < nbrs_cnt; ++z) {
                    VertexID cand = nbrs[z];
                    if (data_graph->getVertexLabel(cand) != curr_label) continue;
                    if (flag[cand] == check_val) {
                        flag[cand] ++;
                        if (check_val == 0) updated_flag[updated_flag_count++] = cand;
                    }
                }
            }
            check_val ++;
        }

        ui cand_index = 0;
        for (ui check_index = 0; check_index < updated_flag_count; ++check_index) {
            VertexID cand = updated_flag[check_index];
            if (flag[cand] == check_val) {
                if (data_graph->getVertexDegree(cand) < curr_degree) continue;
                #ifndef VSSIM_NLF
                candidates[u][cand_index++] = cand;
                #else
                auto data_vertex_nlf = data_graph->getVertexNLF(cand);
                if (data_vertex_nlf->size() >= u_nlf->size()) {
                    bool is_valid = true;
                    for (auto element : *u_nlf) {
                        auto iter = data_vertex_nlf->find(element.first);
                        if (iter == data_vertex_nlf->end() || iter->second < element.second) {
                            is_valid = false;
                            break;
                        }
                    }
                    if (is_valid) candidates[u][cand_index++] = cand;
                }
                #endif
            }
        }
        candidates_count[u] = cand_index;
        for (ui check_index = 0; check_index < updated_flag_count; ++check_index) {
            flag[updated_flag[check_index]] = 0;
        }
        if (candidates_count[u] == 0) {
            delete[] flag;
            delete[] updated_flag;
            return false;
        }
    }
    delete[] flag;
    delete[] updated_flag;
    return true;
}


bool
FilterVertices::VSSimFilterHelper(const Graph *data_graph, const Graph *query_graph, ui **candidates, ui *candidates_count,
                            ui **eqv_classes, ui **eqv_offsets, ui **eqv_counts, 
                            ui *order, TreeNode *tree, ui *updated_flag, ui *flag, bool **valid_candidates, bool reverse, bool using_triangle = true) {
    ui query_vertices_num = query_graph->getVerticesCount();

    int order_start = reverse ? query_vertices_num - 2 : 1;
    int order_end = reverse ? -1 : query_vertices_num;
    int order_add = reverse ? -1 : 1;
    for (int i = order_start; i != order_end; i += order_add) {
        VertexID cur_q = order[i];
        LabelID cur_q_label = query_graph->getVertexLabel(cur_q);

        const VertexID* target_ngbs = nullptr;
        ui target_ngbs_cnt = 0;
        if (reverse) {
            target_ngbs = tree[cur_q].fn_;
            target_ngbs_cnt = tree[cur_q].fn_count_;
        } else{
            target_ngbs = tree[cur_q].bn_;
            target_ngbs_cnt = tree[cur_q].bn_count_;
        }

        ui visited_neg = 0;
        ui updated_flag_count = 0;
        for (ui j = 0; j < target_ngbs_cnt; j++) {
            VertexID q_nbr = target_ngbs[j];

            const std::vector<VertexID>& q_triangles = query_graph->getTriangleVertices(cur_q, q_nbr);

            for (ui k = 0; k < candidates_count[q_nbr]; k++) {
                VertexID cand_ngb = candidates[q_nbr][k];
                
                ui cand_nbrs_cnt;
                auto d_nbrs = data_graph->getVertexNeighbors(cand_ngb, cand_nbrs_cnt);

                for (ui l = 0; l < cand_nbrs_cnt; l++) {
                    VertexID cand_around = d_nbrs[l];
                    if (!valid_candidates[cur_q][cand_around] || data_graph->getVertexLabel(cand_around) != cur_q_label) continue;
                    bool pass_triangle_check = true;
                    if (using_triangle && !q_triangles.empty()) {
                        for (VertexID q_nbr2 : q_triangles) {
                            bool found_valid_triangle_node = false;
                            LabelID q_nbr2_label = query_graph->getVertexLabel(q_nbr2);
                            ui n1_cnt;
                            auto n1 = data_graph->getVertexNeighbors(cand_around, n1_cnt);
                            ui n2_cnt;
                            auto n2 = data_graph->getVertexNeighbors(cand_ngb, n2_cnt);
                            const VertexID* smaller_nbrs;
                            ui smaller_cnt;
                            VertexID other_node;
                            if (n1_cnt < n2_cnt) {
                                smaller_nbrs = n1;
                                smaller_cnt = n1_cnt;
                                other_node = cand_ngb;
                            } else {
                                smaller_nbrs = n2;
                                smaller_cnt = n2_cnt;
                                other_node = cand_around;
                            }
                            for (ui idx = 0; idx < smaller_cnt; ++idx) {
                                VertexID v_dp = smaller_nbrs[idx];
                                if (data_graph->getVertexLabel(v_dp) != q_nbr2_label) continue;
                                
                                if (!valid_candidates[q_nbr2][v_dp]) continue;
                                
                                if (data_graph->checkEdgeExistence(other_node, v_dp)) {
                                    found_valid_triangle_node = true;
                                    break;
                                }
                            }
                            
                            if (!found_valid_triangle_node) {
                                pass_triangle_check = false;
                                break;
                            }
                        }
                    }

                    if (pass_triangle_check) {
                        if (flag[cand_around] == visited_neg) {
                            flag[cand_around] ++;
                            if (visited_neg == 0) updated_flag[updated_flag_count++] = cand_around; 
                        }
                    }
                }
            }
            visited_neg++;
        }

        ui new_size = 0;
        for (ui j = 0; j < candidates_count[cur_q]; j++) {
            VertexID cand_cur = candidates[cur_q][j];
            if (flag[cand_cur] == visited_neg) {
                candidates[cur_q][new_size] = cand_cur;
                eqv_offsets[cur_q][new_size] = eqv_offsets[cur_q][j];
                eqv_counts[cur_q][new_size] = eqv_counts[cur_q][j];
                new_size++;
            }else{
                #ifndef VSSIM_USE_ALL_EQV
                valid_candidates[cur_q][cand_cur] = false;
                #else
                ui offset = eqv_offsets[cur_q][j];
                ui count = eqv_counts[cur_q][j];
                for (ui k = 0; k < count; ++k) {
                    VertexID member = eqv_classes[cur_q][offset + k];
                    valid_candidates[cur_q][member] = false; 
                }
                #endif
            }
        }
        
        candidates_count[cur_q] = new_size;
        if (new_size == 0) {
            return false;
        }
        for (ui i = 0; i < updated_flag_count; ++i) {
            ui v = updated_flag[i];
            flag[v] = 0;
        }
    }
    return true;
}

bool
FilterVertices::VSSimFilter(const Graph *data_graph, const Graph *query_graph, ui **&candidates, ui *&candidates_count,  ui **&eqv_classes, ui **&eqv_offsets, ui **&eqv_counts){
    ui *order;
    TreeNode *tree;
    ui query_vertices_num = query_graph->getVerticesCount();
    ui data_vertices_num = data_graph->getVerticesCount();
    ui* updated_flag = new ui[data_vertices_num]();
    ui* flag = new ui[data_vertices_num]();
    GenerateFilteringPlan::generateVSSimFilterPlan(data_graph, query_graph, tree, order);
    if (!VSSimFilterInitial(data_graph, query_graph, candidates, candidates_count, order, tree)) {
        delete[] updated_flag;
        delete[] flag;
        return false;
    }

    get_equivanlence_base(data_graph, query_graph, candidates, candidates_count, eqv_classes, eqv_offsets, eqv_counts);
    #ifndef VSSIM_USE_ALL_EQV
    bool** valid_candidates = new bool*[query_vertices_num];
    for (ui i = 0; i < query_vertices_num; ++i) {
        valid_candidates[i] = new bool[data_vertices_num]();
        for (ui j = 0; j < candidates_count[i]; ++j) {
            valid_candidates[i][candidates[i][j]] = true;
        }
    }
    #else
    bool** valid_candidates = new bool*[query_vertices_num];
    for (ui i = 0; i < query_vertices_num; ++i) {
        valid_candidates[i] = new bool[data_vertices_num]();
        for (ui j = 0; j < candidates_count[i]; ++j) {
            ui offset = eqv_offsets[i][j];
            ui count = eqv_counts[i][j];
            for (ui k = 0; k < count; ++k) {
                VertexID member = eqv_classes[i][offset + k];
                valid_candidates[i][member] = true;
            }
        }
    }
    #endif
    if (!VSSimFilterHelper(data_graph, query_graph, candidates, candidates_count, eqv_classes, eqv_offsets, eqv_counts, order, tree, updated_flag, flag, valid_candidates, true)) {
        for (ui i = 0; i < query_vertices_num; ++i) delete[] valid_candidates[i];
        delete[] valid_candidates;
        delete[] updated_flag;
        delete[] flag;
        return false;
    }


    if (!VSSimFilterHelper(data_graph, query_graph, candidates, candidates_count, eqv_classes, eqv_offsets, eqv_counts, order, tree, updated_flag, flag, valid_candidates, false)) {
        for (ui i = 0; i < query_vertices_num; ++i) delete[] valid_candidates[i];
        delete[] valid_candidates;
        delete[] updated_flag;
        delete[] flag;
        return false;
    }

    #ifndef VSSIM_MERGE
    for (ui i = 0; i < query_vertices_num; ++i) {
        ui current_offset = 0;
        for (ui j = 0; j < candidates_count[i]; ++j) {
            ui old_offset = eqv_offsets[i][j];
            ui count = eqv_counts[i][j];
            eqv_offsets[i][j] = current_offset;
            for (ui k = 0; k < count; ++k) {
                eqv_classes[i][current_offset + k] = eqv_classes[i][old_offset + k];
            }
            current_offset += count;
        }
        eqv_offsets[i][candidates_count[i]] = current_offset;
    }
    #else
    if (!MergeEquivalenceClasses(data_graph, query_graph, candidates, candidates_count, eqv_classes, eqv_offsets, eqv_counts, valid_candidates)) {
        for (ui i = 0; i < query_vertices_num; ++i) delete[] valid_candidates[i];
        delete[] valid_candidates;
        delete[] updated_flag;
        delete[] flag;
        return false;
    }
    #endif
    for (ui i = 0; i < query_vertices_num; ++i) {
        delete[] valid_candidates[i];
    }
    delete[] valid_candidates;
    delete[] updated_flag;
    delete[] flag;
    return true;
}

bool FilterVertices::VEQFilter(const Graph *data_graph, const Graph *query_graph, ui **&candidates, ui *&candidates_count,
                               ui *&order, TreeNode *&tree) {

    
    if (!LDFFilter(data_graph, query_graph, candidates, candidates_count))
        return false;
    GenerateFilteringPlan::generateDPisoFilterPlan(data_graph, query_graph, tree, order);

    ui query_vertices_num = query_graph->getVerticesCount();
    
    auto start = std::chrono::high_resolution_clock::now();    
    
    bool** flag = new bool*[query_graph->getVerticesCount()];
    for (ui i = 0; i < query_graph->getVerticesCount(); i++) {
        flag[i] = new bool[data_graph->getVerticesCount()];
        memset(flag[i], 0, sizeof(bool)*data_graph->getVerticesCount());
        for (ui j = 0; j < candidates_count[i]; j++) {
            flag[i][candidates[i][j]] = true;
        }
    }
    for (ui u = 0; u < query_vertices_num; u++) {
        ui unbrs_count;
        const VertexID* unbrs = query_graph->getVertexNeighbors(u, unbrs_count);
        for (ui i = 0; i < candidates_count[u]; ++i) {
            auto can = candidates[u][i];
            ui cnbrs_count;
            const ui* cnbrs = data_graph->getVertexNeighbors(can, cnbrs_count);
            bool f_del = true;
            for (ui j = 0; j < unbrs_count; j++) {
                auto unbr = unbrs[j];
                f_del = true;
                for (ui k = 0; k < cnbrs_count; k++) {
                    auto cnbr = cnbrs[k];
                    if (flag[unbr][cnbr] == true) {
                        
                        f_del = false;
                        break;
                    }
                }
                if (f_del == true) break;
            }
            if (f_del == true) {                
                if (candidates_count[u] == 1) {
                    return false;
                }
                flag[u][can] = false;
                candidates[u][i] = candidates[u][--candidates_count[u]];
                i--;
            }
        }
        
    }
    auto end = std::chrono::high_resolution_clock::now();
    double edgeCheckTime = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    std::cout << "edgeCheckTime:" << NANOSECTOMILLISEC(edgeCheckTime)<< std::endl;
    

    std::vector<std::set<VertexID>> m_candidates;
    m_candidates.resize(query_vertices_num);
    for (ui i = 0; i < query_vertices_num; i++) {
        for (ui j = 0; j < candidates_count[i]; j++) {
            m_candidates[i].insert(candidates[i][j]);
        }
    }
    auto pruneStart = std::chrono::high_resolution_clock::now();
    for (ui k = 0; k < 3; ++k) {
        if (k % 2 == 1) {
            for (ui i = 0; i < query_vertices_num; i++) { 
                VertexID query_vertex = order[i];
                TreeNode& node = tree[query_vertex];
                VEQPruneCandidates(data_graph, query_graph, query_vertex, node.bn_, node.bn_count_,
                                   candidates, candidates_count, m_candidates);
            }
        }
        else {
            for (ui i = query_vertices_num - 1; i != (ui)-1; i--) { 
                VertexID query_vertex = order[i];
                TreeNode& node = tree[query_vertex];
                VEQPruneCandidates(data_graph, query_graph, query_vertex, node.fn_, node.fn_count_,
                                   candidates, candidates_count, m_candidates);
            }
        }
    }
    auto pruneEnd = std::chrono::high_resolution_clock::now();
    double VEQPruneTime = std::chrono::duration_cast<std::chrono::nanoseconds>(pruneEnd - pruneStart).count();
    std::cout << "VEQPruneTime:" << NANOSECTOMILLISEC(VEQPruneTime) << std::endl;

    start = std::chrono::high_resolution_clock::now();
    VEQCompactCandidates(candidates, candidates_count, query_vertices_num, m_candidates);
    end = std::chrono::high_resolution_clock::now();
    double VEQCompactTime = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    std::cout << "VEQCompactTime:" << NANOSECTOMILLISEC(VEQCompactTime)<< std::endl;


    return true;
}

void FilterVertices::VEQPruneCandidates(const Graph *data_graph, const Graph *query_graph, VertexID u,
                                    VertexID *child_vertices, ui child_vertices_count, VertexID **candidates,
                                    ui *candidates_count, std::vector<std::set<VertexID>>& D) {
    
    const std::unordered_map<LabelID, ui>* u_nlf = query_graph->getVertexNLF(u);
    ui unbrs_num, vnbrs_num;
    const VertexID* unbrs = query_graph->getVertexNeighbors(u, unbrs_num);
    const VertexID* vnbrs;
    for (ui i = 0; i < candidates_count[u]; i++) {
        VertexID v = candidates[u][i];
        auto v_iter = D[u].find(v);
        if (v_iter == D[u].end()) continue;
        bool h_value = true;
        vnbrs = data_graph->getVertexNeighbors(v, vnbrs_num);
        std::unordered_map<LabelID, ui> v_nlf;
        for (ui j = 0; j < vnbrs_num; j++) { 
            VertexID vc = vnbrs[j];
            for (ui k = 0; k < unbrs_num; k++) {
                VertexID unbr = unbrs[k];
                auto iter = D[unbr].find(vc);
                if (iter != D[unbr].end()) {
                    ui vc_label = data_graph->getVertexLabel(vc);
                    if (v_nlf.find(vc_label) != v_nlf.end()) {
                        v_nlf[vc_label] += 1;
                    } else {
                        v_nlf[vc_label] = 1;
                    }
                    break;
                }
            }
        }
        
        if (v_nlf.size() < u_nlf->size()) {
            D[u].erase(v_iter);
            continue;
        }
        
        for (auto u_label : *u_nlf) {
            auto v_label = v_nlf.find(u_label.first);
            if (v_label == v_nlf.end() || v_label->second < u_label.second) {
                h_value = false;
                break;
            }
            
        }
        
        if (h_value == false) {
            D[u].erase(v_iter);
            continue;
        }

        for (ui j = 0; j < child_vertices_count; j++) {
            VertexID uc = child_vertices[j];
            bool uc_flag = false; 
            for (ui k = 0; k < vnbrs_num; k++) {
                VertexID vc = vnbrs[k];
                auto iter = D[uc].find(vc);
                if (iter != D[uc].end()) {
                    uc_flag = true;
                    break;
                }
            }
            if (uc_flag == false) {
                D[u].erase(v_iter);
                break;
            }
        }
    }
    
    
    
    return;
}

void FilterVertices::VEQCompactCandidates(ui** &candidates, ui* &candidates_count, ui query_vertex_num, 
                                          std::vector<std::set<VertexID>>& D) {
    for (ui i = 0; i < query_vertex_num; i++) {
        for (ui j = 0; j < candidates_count[i]; j++) {
            if (D[i].find(candidates[i][j]) == D[i].end()) {
                candidates[i][j] = candidates[i][--candidates_count[i]];
                if(candidates_count[i] == 0) return;
                j--;
            }
        }
    }
}

void FilterVertices::allocateBuffer(const Graph *data_graph, const Graph *query_graph, ui **&candidates,
                                    ui *&candidates_count) {
    ui query_vertices_num = query_graph->getVerticesCount();
    ui candidates_max_num = data_graph->getGraphMaxLabelFrequency();

    candidates_count = new ui[query_vertices_num];
    memset(candidates_count, 0, sizeof(ui) * query_vertices_num);

    candidates = new ui*[query_vertices_num];

    for (ui i = 0; i < query_vertices_num; ++i) {
        candidates[i] = new ui[candidates_max_num];
    }
}

bool
FilterVertices::verifyExactTwigIso(const Graph *data_graph, const Graph *query_graph, ui data_vertex, ui query_vertex,
                                   bool **valid_candidates, int *left_to_right_offset, int *left_to_right_edges,
                                   int *left_to_right_match, int *right_to_left_match, int* match_visited,
                                   int* match_queue, int* match_previous){
    ui left_partition_size;
    ui right_partition_size;
    const VertexID* query_vertex_neighbors = query_graph->getVertexNeighbors(query_vertex, left_partition_size);
    const VertexID* data_vertex_neighbors = data_graph->getVertexNeighbors(data_vertex, right_partition_size);
    ui edge_count = 0;
    for (int i = 0; i < left_partition_size; ++i) {
        VertexID query_vertex_neighbor = query_vertex_neighbors[i];
        left_to_right_offset[i] = edge_count;

        for (int j = 0; j < right_partition_size; ++j) {
            VertexID data_vertex_neighbor = data_vertex_neighbors[j];
            
            if (valid_candidates[query_vertex_neighbor][data_vertex_neighbor]) {
                left_to_right_edges[edge_count++] = j;
            }
        }
    }
    left_to_right_offset[left_partition_size] = edge_count;    

    memset(left_to_right_match, -1, left_partition_size * sizeof(int));
    memset(right_to_left_match, -1, right_partition_size * sizeof(int));    
    
    GraphOperations::match_bfs(left_to_right_offset, left_to_right_edges, left_to_right_match, right_to_left_match,
                               match_visited, match_queue, match_previous, left_partition_size, right_partition_size);
    for (int i = 0; i < left_partition_size; ++i) {
        if (left_to_right_match[i] == -1)
            return false;
    }

    return true;
}

void FilterVertices::compactCandidates(ui **&candidates, ui *&candidates_count, ui query_vertices_num) {
    for (ui i = 0; i < query_vertices_num; ++i) {
        VertexID query_vertex = i;
        ui next_position = 0;
        for (ui j = 0; j < candidates_count[query_vertex]; ++j) {
            VertexID data_vertex = candidates[query_vertex][j];

            if (data_vertex != INVALID_VERTEX_ID) {
                candidates[query_vertex][next_position++] = data_vertex;
            }
        }

        candidates_count[query_vertex] = next_position;
    }
}

bool FilterVertices::isCandidateSetValid(ui **&candidates, ui *&candidates_count, ui query_vertices_num) {
    for (ui i = 0; i < query_vertices_num; ++i) {
        if (candidates_count[i] == 0)
            return false;
    }
    return true;
}

void
FilterVertices::computeCandidateWithNLF(const Graph *data_graph, const Graph *query_graph, VertexID query_vertex,
                                               ui &count, ui *buffer) {
    LabelID label = query_graph->getVertexLabel(query_vertex);
    ui degree = query_graph->getVertexDegree(query_vertex);
#if OPTIMIZED_LABELED_GRAPH == 1
    const std::unordered_map<LabelID, ui>* query_vertex_nlf = query_graph->getVertexNLF(query_vertex);
#endif
    ui data_vertex_num;
    const ui* data_vertices = data_graph->getVerticesByLabel(label, data_vertex_num);
    count = 0;
    for (ui j = 0; j < data_vertex_num; ++j) {
        ui data_vertex = data_vertices[j];
        if (data_graph->getVertexDegree(data_vertex) >= degree) {
#if OPTIMIZED_LABELED_GRAPH == 1
            const std::unordered_map<LabelID, ui>* data_vertex_nlf = data_graph->getVertexNLF(data_vertex);
            if (data_vertex_nlf->size() >= query_vertex_nlf->size()) {
                bool is_valid = true;
                for (auto element : *query_vertex_nlf) {
                    auto iter = data_vertex_nlf->find(element.first);
                    if (iter == data_vertex_nlf->end() || iter->second < element.second) {
                        is_valid = false;
                        break;
                    }
                }
                if (is_valid) {
                    if (buffer != NULL) {
                        buffer[count] = data_vertex;
                    }
                    count += 1;
                }
            }
#else
            if (buffer != NULL) {
                buffer[count] = data_vertex;
            }
            count += 1;
#endif
        }
    }

}

void FilterVertices::computeCandidateWithLDF(const Graph *data_graph, const Graph *query_graph, VertexID query_vertex,
                                             ui &count, ui *buffer) {
    LabelID label = query_graph->getVertexLabel(query_vertex);
    ui degree = query_graph->getVertexDegree(query_vertex);
    count = 0;
    ui data_vertex_num;
    const ui* data_vertices = data_graph->getVerticesByLabel(label, data_vertex_num);

    if (buffer == NULL) {
        for (ui i = 0; i < data_vertex_num; ++i) {
            VertexID v = data_vertices[i];
            if (data_graph->getVertexDegree(v) >= degree) {
                count += 1;
            }
        }
    }
    else {
        for (ui i = 0; i < data_vertex_num; ++i) {
            VertexID v = data_vertices[i];
            if (data_graph->getVertexDegree(v) >= degree) {
                buffer[count++] = v;
            }
        }
    }
}

void FilterVertices::generateCandidates(const Graph *data_graph, const Graph *query_graph, VertexID query_vertex,
                                       VertexID *pivot_vertices, ui pivot_vertices_count, VertexID **candidates,
                                       ui *candidates_count, ui *flag, ui *updated_flag) {
    LabelID query_vertex_label = query_graph->getVertexLabel(query_vertex);
    ui query_vertex_degree = query_graph->getVertexDegree(query_vertex);

#if OPTIMIZED_LABELED_GRAPH == 1
    const std::unordered_map<LabelID , ui>* query_vertex_nlf = query_graph->getVertexNLF(query_vertex);
#endif

    ui count = 0;
    ui updated_flag_count = 0;
    for (ui i = 0; i < pivot_vertices_count; ++i) {
        VertexID pivot_vertex = pivot_vertices[i];
        for (ui j = 0; j < candidates_count[pivot_vertex]; ++j) {
            VertexID v = candidates[pivot_vertex][j];

            if (v == INVALID_VERTEX_ID)
                continue;

            ui v_nbrs_count;
            const VertexID* v_nbrs = data_graph->getVertexNeighbors(v, v_nbrs_count);
            for (ui k = 0; k < v_nbrs_count; ++k) {
                VertexID v_nbr = v_nbrs[k];
                LabelID v_nbr_label = data_graph->getVertexLabel(v_nbr);
                ui v_nbr_degree = data_graph->getVertexDegree(v_nbr);

                if (flag[v_nbr] == count && v_nbr_label == query_vertex_label && v_nbr_degree >= query_vertex_degree) {
                    flag[v_nbr] += 1;

                    if (count == 0) {
                        updated_flag[updated_flag_count++] = v_nbr;
                    }
                }
            }
        }
        count += 1;
    }

    for (ui i = 0; i < updated_flag_count; ++i) {
        VertexID v = updated_flag[i];
        if (flag[v] == count) {
            
#if OPTIMIZED_LABELED_GRAPH == 1
            const std::unordered_map<LabelID, ui>* data_vertex_nlf = data_graph->getVertexNLF(v);

            if (data_vertex_nlf->size() >= query_vertex_nlf->size()) {
                bool is_valid = true;

                for (auto element : *query_vertex_nlf) {
                    auto iter = data_vertex_nlf->find(element.first);
                    if (iter == data_vertex_nlf->end() || iter->second < element.second) {
                        is_valid = false;
                        break;
                    }
                }

                if (is_valid) {
                    candidates[query_vertex][candidates_count[query_vertex]++] = v;
                }
            }
#else
            candidates[query_vertex][candidates_count[query_vertex]++] = v;
#endif
        }
    }

    for (ui i = 0; i < updated_flag_count; ++i) {
        ui v = updated_flag[i];
        flag[v] = 0;
    }
}

void FilterVertices::pruneCandidates(const Graph *data_graph, const Graph *query_graph, VertexID query_vertex,
                                    VertexID *pivot_vertices, ui pivot_vertices_count, VertexID **candidates,
                                    ui *candidates_count, ui *flag, ui *updated_flag) {
    LabelID query_vertex_label = query_graph->getVertexLabel(query_vertex);
    ui query_vertex_degree = query_graph->getVertexDegree(query_vertex);

    ui count = 0;
    ui updated_flag_count = 0;
    for (ui i = 0; i < pivot_vertices_count; ++i) {
        VertexID pivot_vertex = pivot_vertices[i];
        for (ui j = 0; j < candidates_count[pivot_vertex]; ++j) {
            VertexID v = candidates[pivot_vertex][j];

            if (v == INVALID_VERTEX_ID)
                continue;

            ui v_nbrs_count;
            const VertexID* v_nbrs = data_graph->getVertexNeighbors(v, v_nbrs_count);
            for (ui k = 0; k < v_nbrs_count; ++k) {
                VertexID v_nbr = v_nbrs[k];
                LabelID v_nbr_label = data_graph->getVertexLabel(v_nbr);
                ui v_nbr_degree = data_graph->getVertexDegree(v_nbr);

                if (flag[v_nbr] == count && v_nbr_label == query_vertex_label && v_nbr_degree >= query_vertex_degree) {
                    flag[v_nbr] += 1;

                    if (count == 0) {
                        updated_flag[updated_flag_count++] = v_nbr;
                    }
                }
            }
        }
        count += 1;
    }
    for (ui i = 0; i < candidates_count[query_vertex]; ++i) {
        ui v = candidates[query_vertex][i];
        if (v == INVALID_VERTEX_ID)
            continue;

        if (flag[v] != count) {
            candidates[query_vertex][i] = INVALID_VERTEX_ID;
        }
    }
    for (ui i = 0; i < updated_flag_count; ++i) {
        ui v = updated_flag[i];
        flag[v] = 0;
    }
}


void FilterVertices::sortCandidates(ui **candidates, ui *candidates_count, ui num) {
    for (ui i = 0; i < num; ++i) {
        std::sort(candidates[i], candidates[i] + candidates_count[i]);
    }
}
void FilterVertices::sortCandidates(ui** candidates, ui* candidates_count, ui num, ui **eqv_counts){
    for (ui i = 0; i < num; ++i) {
        std::sort(candidates[i], candidates[i] + candidates_count[i], [eqv_counts, i](ui a, ui b) {
            return eqv_counts[i][a] > eqv_counts[i][b];
        });
    }
}
