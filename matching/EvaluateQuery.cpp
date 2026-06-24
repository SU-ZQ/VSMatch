#include "EvaluateQuery.h"
#include <vector>
#include <cstring>
#include <set>
#include <algorithm>

void EvaluateQuery::generateBN(const Graph *query_graph, ui *order, ui *pivot, ui **&bn, ui *&bn_count) {
    ui query_vertices_num = query_graph->getVerticesCount();
    bn_count = new ui[query_vertices_num]();
    bn = new ui *[query_vertices_num];
    for (ui i = 0; i < query_vertices_num; ++i) {
        bn[i] = new ui[query_vertices_num];
    }

    std::vector<bool> visited_vertices(query_vertices_num, false);
    visited_vertices[order[0]] = true;
    for (ui i = 1; i < query_vertices_num; ++i) {
        VertexID vertex = order[i];
        ui nbrs_cnt;
        const ui *nbrs = query_graph->getVertexNeighbors(vertex, nbrs_cnt);
        for (ui j = 0; j < nbrs_cnt; ++j) {
            VertexID nbr = nbrs[j];
            if (visited_vertices[nbr] && nbr != pivot[i]) {
                bn[i][bn_count[i]++] = nbr;
            }
        }
        visited_vertices[vertex] = true;
    }
}

size_t
EvaluateQuery::update_label_cnt_isolate_dfs_base(const Graph *data_graph, const Graph *query_graph, ui *idx_embedding, 
                        LabelID label_id, ui *vertex2depth, ui **valid_candidate_idx, ui *idx_count, ui **eqv_classes, ui **eqv_offsets, bool *visited_vertices_for_cnt, bool *pass_non_pos, bool *is_isolate, ui *idx_for_cnt, ui *part_emb_for_cnt, ui *idx_start_for_cnt, ui *idx_end_for_cnt, ui *non_pos_for_cnt, ui *valid_v_for_cnt, uint8_t *vertex_count_for_cnt, size_t *depth_embedding_cnt){
    ui label_vertex_cnt;
    auto label_vertices_tmp = query_graph->getVerticesByLabel(label_id, label_vertex_cnt);
    if (label_vertex_cnt == 1){
        size_t label_embedding_cnt = 0;
        ui u = label_vertices_tmp[0];
        if (!is_isolate[u]){
            ui eqv_class_idx = idx_embedding[u];
            ui start = eqv_offsets[u][eqv_class_idx];
            ui end = eqv_offsets[u][eqv_class_idx + 1];
            label_embedding_cnt += end - start;
        } else {
            for (ui idx = 0; idx < idx_count[vertex2depth[u]]; ++idx) {
                ui eqv_class_idx = valid_candidate_idx[vertex2depth[u]][idx];
                ui start = eqv_offsets[u][eqv_class_idx];
                ui end = eqv_offsets[u][eqv_class_idx + 1];
                label_embedding_cnt += end - start;
            }
        }
        return label_embedding_cnt;
    }
    std::vector<ui> label_vertices(label_vertices_tmp, label_vertices_tmp + label_vertex_cnt);
    ui start, end;
    for (ui d = 0; d < label_vertex_cnt; ++d) {
        ui u = label_vertices[d];
        part_emb_for_cnt[u] = 0;
        if (!is_isolate[u]){
            ui eqv_class_idx = idx_embedding[u];
            start = eqv_offsets[u][eqv_class_idx];
            end = eqv_offsets[u][eqv_class_idx + 1];
            part_emb_for_cnt[u] += end - start;
            for (ui i = start; i < end; ++i) {
                vertex_count_for_cnt[eqv_classes[u][i]]++;
            }
        } else {
            for (ui idx = 0; idx < idx_count[vertex2depth[u]]; ++idx) {
                ui eqv_class_idx = valid_candidate_idx[vertex2depth[u]][idx];
                start = eqv_offsets[u][eqv_class_idx];
                end = eqv_offsets[u][eqv_class_idx + 1];
                part_emb_for_cnt[u] += end - start;
                for (ui i = start; i < end; ++i) {
                    vertex_count_for_cnt[eqv_classes[u][i]]++;
                }
            }
        }
    }
    for (ui d = 0; d < label_vertex_cnt; ++d) {
        ui u = label_vertices[d];
        non_pos_for_cnt[u] = 0;
        if (!is_isolate[u]){
            ui eqv_class_idx = idx_embedding[u];
            start = eqv_offsets[u][eqv_class_idx];
            end = eqv_offsets[u][eqv_class_idx + 1];
            for (ui i = start; i < end; ++i) {
                if (vertex_count_for_cnt[eqv_classes[u][i]] > 1){
                    non_pos_for_cnt[u]++;
                }
            }
        } else {
            for (ui idx = 0; idx < idx_count[vertex2depth[u]]; ++idx) {
                ui eqv_class_idx = valid_candidate_idx[vertex2depth[u]][idx];
                start = eqv_offsets[u][eqv_class_idx];
                end = eqv_offsets[u][eqv_class_idx + 1];
                for (ui i = start; i < end; ++i) {
                    if (vertex_count_for_cnt[eqv_classes[u][i]] > 1){
                        non_pos_for_cnt[u]++;
                    }
                }
            }
        }
    }
    sort(label_vertices.begin(), label_vertices.end(), [&](ui a, ui b){
        return non_pos_for_cnt[a] < non_pos_for_cnt[b];
    });
    ui total_valid_v_cnt = 0;
    for (ui d = 0; d < label_vertex_cnt; ++d) {
        ui u = label_vertices[d];
        idx_start_for_cnt[d] = total_valid_v_cnt;
        ui non_pos = idx_end_for_cnt[d] = total_valid_v_cnt + part_emb_for_cnt[u];
        if (!is_isolate[u]){
            ui eqv_class_idx = idx_embedding[u];
            start = eqv_offsets[u][eqv_class_idx];
            end = eqv_offsets[u][eqv_class_idx + 1];
            for (ui i = start; i < end; ++i) {
                VertexID v = eqv_classes[u][i];
                if (vertex_count_for_cnt[v]-- == 1){
                    valid_v_for_cnt[--non_pos] = v;
                }else{
                    valid_v_for_cnt[total_valid_v_cnt++] = v;
                }
            }
        } else {
            for (ui idx = 0; idx < idx_count[vertex2depth[u]]; ++idx) {
                ui eqv_class_idx = valid_candidate_idx[vertex2depth[u]][idx];
                start = eqv_offsets[u][eqv_class_idx];
                end = eqv_offsets[u][eqv_class_idx + 1];            
                for (ui i = start; i < end; ++i) {
                    VertexID v = eqv_classes[u][i];
                    if (vertex_count_for_cnt[v]-- == 1){
                        valid_v_for_cnt[--non_pos] = v;
                    }else{
                        valid_v_for_cnt[total_valid_v_cnt++] = v;
                    }
                }
            }
        }
        non_pos_for_cnt[d] = non_pos;
        total_valid_v_cnt = idx_end_for_cnt[d];
    }
    struct FastDfs {
        ui* idx_start;
        ui* non_pos;
        ui* idx_end;
        ui* valid_v;
        bool* visited;
        ui max_d;
        size_t run(ui depth) {
            ui start = idx_start[depth];
            ui np = non_pos[depth];
            ui end = idx_end[depth];
            if (depth == max_d) {
                size_t cnt = 0;
                for (ui i = start; i < end; ++i) {
                    if (!visited[valid_v[i]]) {
                        cnt++;
                    }
                }
                return cnt;
            }
            size_t total = 0;
            for (ui i = start; i < np; ++i) {
                ui v = valid_v[i];
                if (!visited[v]) {
                    visited[v] = true;
                    total += run(depth + 1);
                    visited[v] = false;
                }
            }
            if (np < end) {
                size_t nc_count = 0;
                for (ui i = np; i < end; ++i) {
                    if (!visited[valid_v[i]]) {
                        nc_count++;
                    }
                }
                if (nc_count > 0) {
                    total += nc_count * run(depth + 1);
                }
            }
            return total;
        }
    };
    FastDfs dfs_runner {
        idx_start_for_cnt,
        non_pos_for_cnt,
        idx_end_for_cnt,
        valid_v_for_cnt,
        visited_vertices_for_cnt,
        label_vertex_cnt - 1
    };
    size_t result = dfs_runner.run(0);
    return result;
}

inline size_t bitwise_mrv_ultra_kernel_64(uint64_t avail_u, uint64_t state_v, const uint64_t* adj) {
    if (!avail_u) return 1;

    int best_u = -1;
    int min_cand_count = 9999;
    uint64_t best_cand_mask = 0;

    uint64_t iter_u = avail_u;
    while (iter_u) {
        int i = __builtin_ctzll(iter_u);
        iter_u &= iter_u - 1;

        uint64_t cand = adj[i] & ~state_v;
        int c = __builtin_popcountll(cand);
        if (c == 0) return 0;
        if (c < min_cand_count) {
            min_cand_count = c;
            best_u = i;
            best_cand_mask = cand;
            if (c == 1) break; 
        }
    }

    uint64_t future_adj = 0;
    uint64_t future_u = avail_u ^ (1ULL << best_u); 
    uint64_t temp_u = future_u;
    while (temp_u) {
        int i = __builtin_ctzll(temp_u);
        temp_u &= temp_u - 1;
        future_adj |= adj[i];
    }

    uint64_t conflict_cands = best_cand_mask & future_adj;
    uint64_t safe_cands = best_cand_mask & ~future_adj;

    size_t count = 0;
    uint64_t next_avail_u = future_u;

    while (conflict_cands) {
        int v = __builtin_ctzll(conflict_cands);
        count += bitwise_mrv_ultra_kernel_64(next_avail_u, state_v | (1ULL << v), adj);
        conflict_cands &= conflict_cands - 1;
    }

    if (safe_cands) {
        int v = __builtin_ctzll(safe_cands); 
        size_t sub_count = bitwise_mrv_ultra_kernel_64(next_avail_u, state_v | (1ULL << v), adj);
        count += sub_count * __builtin_popcountll(safe_cands); 
    }

    return count;
}

inline size_t bitwise_mrv_bitmap_kernel(uint64_t avail_u, uint64_t* state_v, const uint64_t adj[][16], int num_blocks) {
    if (!avail_u) return 1;

    int best_u = -1;
    int min_cand_count = 99999;
    uint64_t best_cand_mask[16] = {0};

    uint64_t iter_u = avail_u;
    while (iter_u) {
        int i = __builtin_ctzll(iter_u);
        iter_u &= iter_u - 1;

        int c = 0;
        uint64_t cand[16];
        for (int b = 0; b < num_blocks; ++b) {
            cand[b] = adj[i][b] & ~state_v[b];
            c += __builtin_popcountll(cand[b]);
        }

        if (c == 0) return 0;
        if (c < min_cand_count) {
            min_cand_count = c;
            best_u = i;
            for (int b = 0; b < num_blocks; ++b) best_cand_mask[b] = cand[b];
            if (c == 1) break;
        }
    }

    uint64_t future_adj[16] = {0};
    uint64_t future_u = avail_u ^ (1ULL << best_u);
    uint64_t temp_u = future_u;
    while (temp_u) {
        int i = __builtin_ctzll(temp_u);
        temp_u &= temp_u - 1;
        for (int b = 0; b < num_blocks; ++b) {
            future_adj[b] |= adj[i][b];
        }
    }

    uint64_t conflict_cands[16];
    uint64_t safe_cands[16];
    int safe_count = 0;
    int first_safe_b = -1;
    int first_safe_v = -1;

    for (int b = 0; b < num_blocks; ++b) {
        conflict_cands[b] = best_cand_mask[b] & future_adj[b];
        safe_cands[b] = best_cand_mask[b] & ~future_adj[b];
        int s_cnt = __builtin_popcountll(safe_cands[b]);
        if (s_cnt > 0 && first_safe_b == -1) {
            first_safe_b = b;
            first_safe_v = __builtin_ctzll(safe_cands[b]);
        }
        safe_count += s_cnt;
    }

    size_t count = 0;
    uint64_t next_avail_u = future_u;

    for (int b = 0; b < num_blocks; ++b) {
        uint64_t cc = conflict_cands[b];
        while (cc) {
            int v = __builtin_ctzll(cc);
            state_v[b] |= (1ULL << v);
            count += bitwise_mrv_bitmap_kernel(next_avail_u, state_v, adj, num_blocks);
            state_v[b] &= ~(1ULL << v);
            cc &= cc - 1;
        }
    }

    if (safe_count > 0) {
        state_v[first_safe_b] |= (1ULL << first_safe_v);
        size_t sub_count = bitwise_mrv_bitmap_kernel(next_avail_u, state_v, adj, num_blocks);
        state_v[first_safe_b] &= ~(1ULL << first_safe_v);
        count += sub_count * safe_count;
    }

    return count;
}

size_t
EvaluateQuery::update_label_cnt_isolate_dfs_base_backup2(const Graph *data_graph, const Graph *query_graph, ui *idx_embedding, 
                        LabelID label_id, ui *vertex2depth, ui **valid_candidate_idx, ui *idx_count, ui **eqv_classes, ui **eqv_offsets, bool *visited_vertices_for_cnt, bool *pass_non_pos, bool *is_isolate, ui *idx_for_cnt, ui *part_emb_for_cnt, ui *idx_start_for_cnt, ui *idx_end_for_cnt, ui *non_pos_for_cnt, ui *valid_v_for_cnt, uint8_t *vertex_count_for_cnt, size_t *depth_embedding_cnt){
    ui label_vertex_cnt;
    auto label_vertices_tmp = query_graph->getVerticesByLabel(label_id, label_vertex_cnt);

    if (label_vertex_cnt == 1){
        size_t label_embedding_cnt = 0;
        ui u = label_vertices_tmp[0];
        if (!is_isolate[u]){
            ui eqv_class_idx = idx_embedding[u];
            ui start = eqv_offsets[u][eqv_class_idx];
            ui end = eqv_offsets[u][eqv_class_idx + 1];
            label_embedding_cnt += end - start;
        } else {
            for (ui idx = 0; idx < idx_count[vertex2depth[u]]; ++idx) {
                ui eqv_class_idx = valid_candidate_idx[vertex2depth[u]][idx];
                ui start = eqv_offsets[u][eqv_class_idx];
                ui end = eqv_offsets[u][eqv_class_idx + 1];
                label_embedding_cnt += end - start;
            }
        }
        return label_embedding_cnt;
    }
    if (label_vertex_cnt > 64) {
        return update_label_cnt_isolate_dfs_base(data_graph, query_graph, idx_embedding, 
                        label_id, vertex2depth, valid_candidate_idx, idx_count, eqv_classes, 
                        eqv_offsets, visited_vertices_for_cnt, pass_non_pos, is_isolate, 
                        idx_for_cnt, part_emb_for_cnt, idx_start_for_cnt, idx_end_for_cnt, 
                        non_pos_for_cnt, valid_v_for_cnt, vertex_count_for_cnt, depth_embedding_cnt);
    }
    int total = 0;
    for (ui d = 0; d < label_vertex_cnt; ++d) {
        ui u = label_vertices_tmp[d];
        if (!is_isolate[u]) {
            ui eqv_class_idx = idx_embedding[u];
            ui start = eqv_offsets[u][eqv_class_idx];
            ui end = eqv_offsets[u][eqv_class_idx + 1];
            for (ui i = start; i < end; ++i) valid_v_for_cnt[total++] = eqv_classes[u][i];
        } else {
            for (ui idx = 0; idx < idx_count[vertex2depth[u]]; ++idx) {
                ui eqv_class_idx = valid_candidate_idx[vertex2depth[u]][idx];
                ui start = eqv_offsets[u][eqv_class_idx];
                ui end = eqv_offsets[u][eqv_class_idx + 1];
                for (ui i = start; i < end; ++i) valid_v_for_cnt[total++] = eqv_classes[u][i];
            }
        }
    }

    std::sort(valid_v_for_cnt, valid_v_for_cnt + total);
    int N_unique = std::unique(valid_v_for_cnt, valid_v_for_cnt + total) - valid_v_for_cnt;

    if (N_unique > 1024) {
        return update_label_cnt_isolate_dfs_base(data_graph, query_graph, idx_embedding, 
                        label_id, vertex2depth, valid_candidate_idx, idx_count, eqv_classes, 
                        eqv_offsets, visited_vertices_for_cnt, pass_non_pos, is_isolate, 
                        idx_for_cnt, part_emb_for_cnt, idx_start_for_cnt, idx_end_for_cnt, 
                        non_pos_for_cnt, valid_v_for_cnt, vertex_count_for_cnt, depth_embedding_cnt);
    }
    for (int i = N_unique - 1; i >= 0; --i){
        valid_v_for_cnt[valid_v_for_cnt[i]] = i;
    }
    if (N_unique <= 64) {
        uint64_t adj_global_64[64] = {0}; 
        for (ui d = 0; d < label_vertex_cnt; ++d) {
            ui u = label_vertices_tmp[d];
            if (!is_isolate[u]) {
                ui eqv_class_idx = idx_embedding[u];
                ui start = eqv_offsets[u][eqv_class_idx];
                ui end = eqv_offsets[u][eqv_class_idx + 1];
                for (ui i = start; i < end; ++i) {
                    adj_global_64[d] |= (1ULL << valid_v_for_cnt[eqv_classes[u][i]]);
                }
            } else {
                for (ui idx = 0; idx < idx_count[vertex2depth[u]]; ++idx) {
                    ui eqv_class_idx = valid_candidate_idx[vertex2depth[u]][idx];
                    ui start = eqv_offsets[u][eqv_class_idx];
                    ui end = eqv_offsets[u][eqv_class_idx + 1];
                    for (ui i = start; i < end; ++i) {
                        adj_global_64[d] |= (1ULL << valid_v_for_cnt[eqv_classes[u][i]]);
                    }
                }
            }
        }
        uint64_t avail_u = (label_vertex_cnt == 64) ? ~0ULL : (1ULL << label_vertex_cnt) - 1;
        size_t count = bitwise_mrv_ultra_kernel_64(avail_u, 0, adj_global_64);
        return count;
    } else {
        int num_blocks = (N_unique + 63) / 64;
        uint64_t adj_global_bmp[64][16] = {0};
        
        for (ui d = 0; d < label_vertex_cnt; ++d) {
            ui u = label_vertices_tmp[d];
            if (!is_isolate[u]) {
                ui eqv_class_idx = idx_embedding[u];
                ui start = eqv_offsets[u][eqv_class_idx];
                ui end = eqv_offsets[u][eqv_class_idx + 1];
                for (ui i = start; i < end; ++i) {
                    ui mapped_v = valid_v_for_cnt[eqv_classes[u][i]];
                    adj_global_bmp[d][mapped_v / 64] |= (1ULL << (mapped_v % 64));
                }
            } else {
                for (ui idx = 0; idx < idx_count[vertex2depth[u]]; ++idx) {
                    ui eqv_class_idx = valid_candidate_idx[vertex2depth[u]][idx];
                    ui start = eqv_offsets[u][eqv_class_idx];
                    ui end = eqv_offsets[u][eqv_class_idx + 1];
                    for (ui i = start; i < end; ++i) {
                        ui mapped_v = valid_v_for_cnt[eqv_classes[u][i]];
                        adj_global_bmp[d][mapped_v / 64] |= (1ULL << (mapped_v % 64));
                    }
                }
            }
        }
        uint64_t avail_u = (label_vertex_cnt == 64) ? ~0ULL : (1ULL << label_vertex_cnt) - 1;
        uint64_t state_v[16] = {0};
        size_t count = bitwise_mrv_bitmap_kernel(avail_u, state_v, adj_global_bmp, num_blocks);
        return count;
    }
}

bool EvaluateQuery::check_isolate_order(const Graph *query_graph, ui *order, bool *is_isolate, ui non_isolate_count){
    ui query_vertice_num = query_graph->getVerticesCount();
    std::vector<bool> visited(query_vertice_num, false);
    std::vector<bool> extendable(query_vertice_num, false);
    std::vector<ui> neighbor_count(query_vertice_num, 0);
    bool flag = true;
    for (ui i = 0; i < query_vertice_num; ++i) {
        VertexID u = order[i];
        neighbor_count[u] = query_graph->getVertexDegree(u);
    }
    extendable[order[0]] = true;
    for (ui i = 0; i < non_isolate_count; ++i) {
        VertexID u = order[i];
        if (visited[u]){
            std::cout<<"Error: order has duplicate vertex "<<u<<std::endl;
            flag = false;
        }
        if (!extendable[u]){
            std::cout<<"Error: non-extendable vertex "<<u<<" visited."<<std::endl;
            flag = false;
        }
        if (neighbor_count[u]==0){
            std::cout<<"Error: non-isolate vertex "<<u<<" has no neighbor when visited."<<std::endl;
            flag = false;
        }
        if (is_isolate[u]){
            std::cout<<"Error: non-isolate vertex "<<u<<" marked as isolate."<<std::endl;
            flag = false;
        }
        visited[u] = true;
        ui nbr_cnt;
        const VertexID *nbrs = query_graph->getVertexNeighbors(u, nbr_cnt);
        for (ui j = 0; j < nbr_cnt; ++j) {
            VertexID nbr = nbrs[j];
            neighbor_count[nbr] --;
            extendable[nbr] = true;
        }
    }
    for (ui i = non_isolate_count; i < query_vertice_num; ++i) {
        VertexID u = order[i];
        if (visited[u]){
            std::cout<<"Error: order has duplicate vertex "<<u<<std::endl;
            flag = false;
        }
        if (!extendable[u]){
            std::cout<<"Error: non-extendable vertex "<<u<<" visited."<<std::endl;
            flag = false;
        }
        if (neighbor_count[u]>0){
            std::cout<<"Error: isolate vertex "<<u<<" has neighbor when visited."<<std::endl;
            flag = false;
        }
        if (!is_isolate[u]){
            std::cout<<"Error: isolate vertex "<<u<<" marked as non-isolate."<<std::endl;
            flag = false;
        }
        visited[u] = true;
    }
    return flag;
}

ui EvaluateQuery::split_isolate_max(const Graph *query_graph,ui *candidates_count, ui *order, ui *pivot, bool *is_isolate){

    ui query_vertice_num = query_graph->getVerticesCount();
    std::vector<bool> visited(query_vertice_num, false);
    std::vector<bool> extendable(query_vertice_num, false);
    std::vector<int> vertex_degress(query_vertice_num);

    VertexID start_vertex = 0;
    vertex_degress[start_vertex] = query_graph->getVertexDegree(start_vertex);
    auto degree_based_cmp = [&](VertexID a, VertexID b) {
        if (vertex_degress[a] != vertex_degress[b]) {
            return vertex_degress[a] > vertex_degress[b];
        }
        return candidates_count[a] < candidates_count[b];
    };
    auto cands_based_cmp = [&](VertexID a, VertexID b) {
        if (candidates_count[a] != candidates_count[b]) {
            return candidates_count[a] < candidates_count[b];
        }
        return vertex_degress[a] > vertex_degress[b];
    };
    for (ui i = 1; i < query_vertice_num; ++i){
        vertex_degress[i] = query_graph->getVertexDegree(i);
        if (cands_based_cmp(i, start_vertex)){
            start_vertex = i;
        }
    }    
    auto update_Vertices_info = [&](const Graph *query_graph, VertexID vertex){
        visited[vertex] = true;
        extendable[vertex] = false;
        vertex_degress[vertex] = -1;
        ui nbr_cnt;
        const VertexID *nbrs = query_graph->getVertexNeighbors(vertex, nbr_cnt);
        for (ui i = 0; i < nbr_cnt; ++i) {
            VertexID nbr = nbrs[i];
            if (!visited[nbr]) {
                vertex_degress[nbr] --;
                extendable[nbr] = true;
            }
        }
    };
    is_isolate[start_vertex] = false;
    order[0] = start_vertex;
    update_Vertices_info(query_graph, start_vertex);
    ui isolate_pos = ui(-1);
    for (ui i = 1; i < query_vertice_num; ++i) {
        VertexID next_vertex = i;
        for (ui j = 0; j < query_vertice_num; ++j) {
            VertexID cur_vertex = j;
            if (!visited[cur_vertex] && extendable[cur_vertex]) {
                if (!extendable[next_vertex]){
                    next_vertex = cur_vertex;
                    continue;
                }
                if (degree_based_cmp(cur_vertex, next_vertex)) {
                    next_vertex = cur_vertex;
                }
            }
        }
        if (vertex_degress[next_vertex] != 0){
            is_isolate[next_vertex] = false;
        } else {
            is_isolate[next_vertex] = true;
            if (isolate_pos == ui(-1)){
                isolate_pos = i;
            }
        }
        order[i] = next_vertex;
        update_Vertices_info(query_graph, next_vertex);
    }
    std::fill(visited.begin(), visited.end(), false);
    std::fill(extendable.begin(), extendable.end(), false);
    start_vertex = order[0];
    ui start_idx = 0;
    for (ui i = 1; i < isolate_pos; ++i){        
        VertexID next_vertex = order[i];
        if (cands_based_cmp(next_vertex, start_vertex)){
            start_vertex = next_vertex;
            start_idx = i;
        }
    }
    std::swap(order[0], order[start_idx]);
    update_Vertices_info(query_graph, start_vertex);
    for (ui i = 1; i < isolate_pos; ++i) {
        VertexID next_vertex = order[i];
        ui next_vertex_idx = i;
        for (ui j = i + 1; j < isolate_pos; ++j) {
            VertexID cur_vertex = order[j];
            if (!visited[cur_vertex] && extendable[cur_vertex]) {
                if (!extendable[next_vertex]){
                    next_vertex = cur_vertex;
                    next_vertex_idx = j;
                    continue;
                }
                if (cands_based_cmp(cur_vertex, next_vertex)) {
                    next_vertex = cur_vertex;
                    next_vertex_idx = j;
                }
            }
        }
        std::swap(order[i], order[next_vertex_idx]);
        update_Vertices_info(query_graph, next_vertex);
    }
    for (ui i = 1; i < query_vertice_num; ++i) {
        VertexID u = order[i];
        for (ui j = i - 1; j >= 0; --j) {
            VertexID cur_vertex = order[j];
            if (query_graph->checkEdgeExistence(u, cur_vertex)) {
                pivot[i] = cur_vertex;
                break;
            }
        }
    }
    return isolate_pos;
}

bool EvaluateQuery::update_Extendable_Vertex(const Graph *data_graph, const Graph *query_graph, ui cur_u, ui *embedding, ui *idx_embedding, ui *idx_count, ui **valid_candidate_idx, ui *u2depth,
                                            ui **valid_candidate_count_level, ui ***valid_candidate_idx_level, Edges ***edge_matrix,
                                            bool *visited_vertices, int *visited_bn, ui **candidates,ui **eqv_counts){
    ui nbr_cnt;
    const VertexID *nbrs = query_graph->getVertexNeighbors(cur_u, nbr_cnt);
    for (ui i = 0; i < nbr_cnt; ++i) {
        VertexID u_nbr = nbrs[i];
        if (idx_embedding[u_nbr] != -1) {
            continue;
        }
        if (visited_bn[u_nbr] == 0) {
            ui idx_id = idx_embedding[cur_u];
            Edges &edge = *edge_matrix[cur_u][u_nbr];
            ui count = edge.offset_[idx_id + 1] - edge.offset_[idx_id];
            ui *candidate_idx = edge.edge_ + edge.offset_[idx_id];
            ui valid_candidate_index_count = 0;
            ui *valid_candidate_index = valid_candidate_idx_level[u_nbr][1];
            for (ui i = 0; i < count; ++i) {
                ui temp_idx = candidate_idx[i];
                VertexID temp_v = candidates[u_nbr][temp_idx];
                if (!visited_vertices[temp_v] || eqv_counts[u_nbr][temp_idx] > 1)
                    valid_candidate_index[valid_candidate_index_count++] = temp_idx;
            }
            if (valid_candidate_index_count == 0) {
                return false;
            }
            valid_candidate_count_level[u_nbr][1] = valid_candidate_index_count;
        } else {
            ui cur_v = embedding[cur_u];
            ui old_candidate_count = valid_candidate_count_level[u_nbr][visited_bn[u_nbr]];
            ui *old_candidate_index = valid_candidate_idx_level[u_nbr][visited_bn[u_nbr]];
            ui new_candidate_count = 0;
            ui *new_candidate_index = valid_candidate_idx_level[u_nbr][visited_bn[u_nbr] + 1];
            for (ui i = 0; i < old_candidate_count; ++i) {
                ui temp_idx = old_candidate_index[i];
                VertexID temp_v = candidates[u_nbr][temp_idx];
                if (visited_vertices[temp_v] && eqv_counts[u_nbr][temp_idx] == 1)
                    continue;
                if (data_graph->checkEdgeExistence(temp_v, cur_v)) {
                    new_candidate_index[new_candidate_count++] = temp_idx;
                }
            }
            if (new_candidate_count == 0) {
                return false;
            }
            valid_candidate_count_level[u_nbr][visited_bn[u_nbr] + 1] = new_candidate_count;
        }
    }
    return true;
}

mpz_class
EvaluateQuery::VSMatch(const Graph *data_graph, const Graph *query_graph, Edges ***edge_matrix, ui **candidates,
        ui *candidates_count, ui **eqv_classes, ui **eqv_offsets, ui **eqv_counts, ui *order, ui *pivot, size_t output_limit_num, size_t &call_count, size_t time_limit, std::chrono::high_resolution_clock::time_point start_time) {

    ui max_depth = query_graph->getVerticesCount();
    ui label_cnt = query_graph->getLabelsCount();
    bool *is_isolate = new bool[max_depth];
    ui non_isolate_depth = split_isolate_max(query_graph, candidates_count, order, pivot, is_isolate);
    ui *bn_count = new ui[max_depth]();
    std::vector<std::vector<int>> label_triggers(max_depth);
    ui *trigger_pre = new ui[max_depth]();
    bool *label_embeddings_no_zero = new bool[max_depth];
    ui *label_vertex = new ui[label_cnt];
    size_t *label_embeddings = new size_t[label_cnt]();
    ui *vertex2depth = new ui[max_depth];
    for (ui i = 0; i < label_cnt; ++i) {
        label_vertex[i] = INVALID_VERTEX_ID;
    }
    ui max_eqv_set_size = 0;    
    ui max_candidates_num = 0;
    for (ui i = 0; i < max_depth; ++i){
        max_eqv_set_size = std::max(max_eqv_set_size, eqv_counts[i][0]);
        max_candidates_num = std::max(max_candidates_num, candidates_count[i]);
        VertexID u = order[i];
        vertex2depth[u] = i;
        trigger_pre[i] = 0;
        for (int j = 0; j < i; ++j){
            VertexID u2 = order[j];
            if (query_graph->checkEdgeExistence(u, u2)){
                trigger_pre[i] = j;
                bn_count[i] ++;
            }
        }
    }
    for (ui d = 0; d < non_isolate_depth; ++d) {
        VertexID u = order[d];
        ui label = query_graph->getVertexLabel(u);
        label_vertex[label] = d;
        label_embeddings_no_zero[d] = false;
    }
    for (ui d = non_isolate_depth; d < max_depth; ++d){
        VertexID u = order[d];
        ui label = query_graph->getVertexLabel(u);
        if (label_vertex[label] > max_depth){
            label_vertex[label] = trigger_pre[d];
        } else {
            label_vertex[label] = std::max(trigger_pre[d], label_vertex[label]);
        }
    }
    for (ui label_id = 0; label_id < label_cnt; ++label_id) {
        if (label_vertex[label_id] < max_depth) {
            label_triggers[label_vertex[label_id]].push_back(label_id);
        }
    }
    ui data_vertices_num = data_graph->getVerticesCount();
    ui max_label_cands_num = std::max(query_graph->getGraphMaxLabelFrequency() * max_eqv_set_size, data_vertices_num); 
    ui *idx_for_cnt = new ui[max_depth];
    ui *part_emb_for_cnt = new ui[max_depth];
    ui *idx_start_for_cnt = new ui[max_depth];
    ui *idx_end_for_cnt = new ui[max_depth];
    ui *non_pos_for_cnt = new ui[max_depth];
    size_t *depth_embedding_cnt = new size_t[max_depth];
    bool *pass_non_for_cnt = new bool[max_depth];
    bool *visited_vertices_for_cnt = new bool[data_vertices_num]();
    uint8_t *vertex_count_for_cnt = new uint8_t[data_vertices_num]();
    ui *valid_v_for_cnt = new ui[max_label_cands_num];
    ui *idx;
    ui *idx_count;
    ui *embedding;
    ui *idx_embedding;
    int *visited_bn;
    ui **valid_candidate_count_level;
    ui ***valid_candidate_idx_level;
    ui **valid_candidate_idx; 
    bool *visited_vertices;

    ui max_query_degree = query_graph->getGraphMaxDegree();
    idx = new ui[max_depth];
    idx_count = new ui[max_depth];
    embedding = new ui[max_depth];
    idx_embedding = new ui[max_depth];
    std::fill(idx_embedding, idx_embedding + max_depth, INVALID_VERTEX_ID);
    visited_vertices = new bool[data_vertices_num]();
    valid_candidate_idx = new ui *[max_depth];
    visited_bn = new int[max_depth]();
    valid_candidate_count_level = new ui*[max_depth];
    for (ui i = 0; i < max_depth; ++i) {
        valid_candidate_count_level[i] = new ui[max_query_degree + 1];
    }
    valid_candidate_idx_level = new ui **[max_depth];
    for (ui i = 0; i < max_depth; ++i) {
        valid_candidate_idx_level[i] = new ui *[max_query_degree + 1];
        for (ui j = 0; j < max_query_degree + 1; ++j){
            valid_candidate_idx_level[i][j] = new ui[max_candidates_num];
            valid_candidate_count_level[i][0] = candidates_count[i];
        }
    }
    for (ui u = 0; u < max_depth; ++u) {
        if (is_isolate[u]) {
            valid_candidate_idx[vertex2depth[u]] = valid_candidate_idx_level[u][bn_count[vertex2depth[u]]];
        }
    }
    mpz_class embedding_cnt = 0;
    mpz_class sub_embedding;
    std::chrono::high_resolution_clock::time_point current_time;
    int cur_depth = 0;
    VertexID start_vertex = order[0];
    std::set<ui> label_set;
    for (ui u = 0; u < max_depth; ++u) {
        label_set.insert(query_graph->getVertexLabel(order[u]));
    }
    idx[cur_depth] = 0;
    idx_count[cur_depth] = candidates_count[start_vertex];
    for (ui i = 0; i < idx_count[cur_depth]; ++i) {
        valid_candidate_idx_level[start_vertex][0][i] = i;
    }
    valid_candidate_idx[cur_depth] = valid_candidate_idx_level[start_vertex][0];
    while (true) {
        while (idx[cur_depth] < idx_count[cur_depth]) {
            ui valid_idx = valid_candidate_idx[cur_depth][idx[cur_depth]];
            VertexID u = order[cur_depth];
            VertexID v = candidates[u][valid_idx];

            if (visited_vertices[v] && eqv_counts[u][valid_idx]==1) {
                idx[cur_depth] += 1;
                continue;
            }
            embedding[u] = v;
            idx_embedding[u] = valid_idx;
            if (!update_Extendable_Vertex(data_graph, query_graph, u, embedding, idx_embedding, idx_count, valid_candidate_idx, vertex2depth,valid_candidate_count_level, valid_candidate_idx_level, edge_matrix, visited_vertices, visited_bn, candidates, eqv_counts)){
                idx[cur_depth] += 1;
                continue;
            }
            if (!label_triggers[cur_depth].empty()) {
                bool any_label_zero = false;
                for (ui label_id:label_triggers[cur_depth]){
                    ui label_vertex_cnt;
                    auto label_vertices = query_graph->getVerticesByLabel(label_id, label_vertex_cnt);
                    for (ui d = 0; d < label_vertex_cnt; ++d) {
                        ui u = label_vertices[d];
                        if (is_isolate[u]){
                            idx_count[vertex2depth[u]] = valid_candidate_count_level[u][bn_count[vertex2depth[u]]];
                        }
                    }
                    label_embeddings[label_id] = update_label_cnt_isolate_dfs_base(data_graph, query_graph,  idx_embedding, label_id, vertex2depth, valid_candidate_idx, idx_count, eqv_classes, eqv_offsets, visited_vertices_for_cnt, pass_non_for_cnt, is_isolate,
                                                        idx_for_cnt,
                                                        part_emb_for_cnt,
                                                        idx_start_for_cnt,
                                                        idx_end_for_cnt,
                                                        non_pos_for_cnt,
                                                        valid_v_for_cnt,
                                                        vertex_count_for_cnt,
                                                        depth_embedding_cnt);
                    current_time = std::chrono::high_resolution_clock::now();
                    if (label_embeddings[label_id] == 0) {
                        any_label_zero = true;
                        break;
                    }
                }
                if (any_label_zero){
                    idx[cur_depth] += 1;
                    continue;
                }
            }
            if (eqv_counts[u][valid_idx] == 1){
                visited_vertices[v] = true;
            }
            idx[cur_depth] += 1;

            if (cur_depth == non_isolate_depth - 1) {
                sub_embedding = 1;
                for (ui label_id:label_set) {
                    sub_embedding *= label_embeddings[label_id];
                }
                embedding_cnt += sub_embedding;
                visited_vertices[v] = false;
                current_time = std::chrono::high_resolution_clock::now();
                if (output_limit_num != std::numeric_limits<size_t>::max() && embedding_cnt >= output_limit_num) {
                    goto EXIT;
                }
                if (std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count() >= time_limit) {
                    goto EXIT;
                }
            } else {
                ui n_nbrs;
                const VertexID *nbrs = query_graph->getVertexNeighbors(u, n_nbrs);
                for (ui i = 0; i < n_nbrs; ++i) {
                    VertexID u_nbr = nbrs[i];
                    if (idx_embedding[u_nbr] == INVALID_VERTEX_ID) {
                        ++visited_bn[u_nbr];
                    }
                }
                call_count += 1;
                cur_depth += 1;
                idx[cur_depth] = 0;
                ui nxt_u = order[cur_depth];
                valid_candidate_idx[cur_depth] = valid_candidate_idx_level[nxt_u][visited_bn[nxt_u]];
                idx_count[cur_depth] = valid_candidate_count_level[nxt_u][visited_bn[nxt_u]];
                current_time = std::chrono::high_resolution_clock::now();
                if (std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count() >= time_limit) {
                    goto EXIT;
                }
            }
        }
        if (cur_depth < 1) {
            break;
        } else {
            ui cur_u = order[cur_depth];
            ui nxt_u = order[cur_depth - 1];
            --cur_depth; 
            idx_embedding[cur_u] = INVALID_VERTEX_ID;
            ui n_nbrs;
            const VertexID *nbrs = query_graph->getVertexNeighbors(nxt_u, n_nbrs);
            for (ui i = 0; i < n_nbrs; ++i) {
                VertexID u_nbr = nbrs[i];
                if (idx_embedding[u_nbr] == INVALID_VERTEX_ID) {
                    --visited_bn[u_nbr];
                }
            }
            visited_vertices[embedding[order[cur_depth]]] = false;
        }
    }
    EXIT:
    delete [] visited_bn;
    for (ui i = 0; i < max_depth; ++i) {
        delete [] valid_candidate_count_level[i];
    }
    delete [] valid_candidate_count_level;
    for (ui i = 0; i < max_depth; ++i) {
        for (int j = 0 ; j < max_query_degree + 1; ++j){
            delete [] valid_candidate_idx_level[i][j];
        }
        delete [] valid_candidate_idx_level[i];
    }
    delete [] valid_candidate_idx_level;
    delete [] visited_vertices;
    delete [] idx;
    delete [] idx_count;
    delete [] embedding;
    delete [] idx_embedding;
    delete [] label_vertex;
    delete [] label_embeddings;
    delete [] is_isolate;
    delete [] vertex2depth;
    delete [] idx_for_cnt;
    delete [] part_emb_for_cnt;
    delete [] idx_start_for_cnt;
    delete [] idx_end_for_cnt;
    delete [] non_pos_for_cnt;
    delete [] valid_v_for_cnt;
    delete [] visited_vertices_for_cnt;
    delete [] vertex_count_for_cnt;
    return embedding_cnt;
}

size_t
EvaluateQuery::exploreGraph(const Graph *data_graph, const Graph *query_graph, Edges ***edge_matrix, ui **candidates, ui *candidates_count, 
                            ui *order, ui *pivot, size_t output_limit_num, size_t &call_count, size_t time_limit, std::chrono::high_resolution_clock::time_point start_time) {
    
    ui **bn;
    ui *bn_count;
    generateBN(query_graph, order, pivot, bn, bn_count);
    
    ui *idx;
    ui *idx_count;
    ui *embedding;
    ui *idx_embedding;
    ui *temp_buffer;
    ui **valid_candidate_idx;
    bool *visited_vertices;
    allocateBuffer(data_graph, query_graph, candidates_count, idx, idx_count, embedding, idx_embedding,
                   temp_buffer, valid_candidate_idx, visited_vertices);
    
    size_t embedding_cnt = 0;
    std::chrono::high_resolution_clock::time_point current_time;
    int cur_depth = 0;
    ui max_depth = query_graph->getVerticesCount();
    VertexID start_vertex = order[0];

    idx[cur_depth] = 0;
    idx_count[cur_depth] = candidates_count[start_vertex];

    for (ui i = 0; i < idx_count[cur_depth]; ++i) {
        valid_candidate_idx[cur_depth][i] = i;
    }

    while (true) {
        while (idx[cur_depth] < idx_count[cur_depth]) {
            ui valid_idx = valid_candidate_idx[cur_depth][idx[cur_depth]];
            VertexID u = order[cur_depth];
            VertexID v = candidates[u][valid_idx];

            embedding[u] = v;
            idx_embedding[u] = valid_idx;
            visited_vertices[v] = true;
            idx[cur_depth] += 1;

            if (cur_depth == max_depth - 1) {
                embedding_cnt += 1;
                visited_vertices[v] = false;
                if (embedding_cnt >= output_limit_num) {
                    goto EXIT;
                }
                current_time = std::chrono::high_resolution_clock::now();
                if (std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count() >= time_limit) {
                    goto EXIT;
                }
            } else {
                call_count += 1;
                cur_depth += 1;
                idx[cur_depth] = 0;
                generateValidCandidateIndex(data_graph, cur_depth, embedding, idx_embedding, idx_count,
                                            valid_candidate_idx,
                                            edge_matrix, visited_vertices, bn, bn_count, order, pivot, candidates);
            }
        }
        cur_depth -= 1;
        if (cur_depth < 0)
            break;
        else
            visited_vertices[embedding[order[cur_depth]]] = false;
    }
    EXIT:
    releaseBuffer(max_depth, idx, idx_count, embedding, idx_embedding, temp_buffer, valid_candidate_idx,
                  visited_vertices,
                  bn, bn_count);
    return embedding_cnt;
}

void
EvaluateQuery::allocateBuffer(const Graph *data_graph, const Graph *query_graph, ui *candidates_count, ui *&idx,
                              ui *&idx_count, ui *&embedding, ui *&idx_embedding, ui *&temp_buffer,
                              ui **&valid_candidate_idx, bool *&visited_vertices) {
    ui query_vertices_num = query_graph->getVerticesCount();
    ui data_vertices_num = data_graph->getVerticesCount();
    ui max_candidates_num = candidates_count[0];

    for (ui i = 1; i < query_vertices_num; ++i) {
        VertexID cur_vertex = i;
        ui cur_candidate_num = candidates_count[cur_vertex];
        if (cur_candidate_num > max_candidates_num) {
            max_candidates_num = cur_candidate_num;
        }
    }

    idx = new ui[query_vertices_num];
    idx_count = new ui[query_vertices_num];
    embedding = new ui[query_vertices_num];
    idx_embedding = new ui[query_vertices_num];
    visited_vertices = new bool[data_vertices_num];
    temp_buffer = new ui[max_candidates_num];
    valid_candidate_idx = new ui *[query_vertices_num];
    for (ui i = 0; i < query_vertices_num; ++i) {
        valid_candidate_idx[i] = new ui[max_candidates_num];
    }
    std::fill(visited_vertices, visited_vertices + data_vertices_num, false);
}

void EvaluateQuery::generateValidCandidateIndex(const Graph *data_graph, ui depth, ui *embedding, ui *idx_embedding,
                                                ui *idx_count, ui **valid_candidate_index, Edges ***edge_matrix,
                                                bool *visited_vertices, ui **bn, ui *bn_cnt, ui *order, ui *pivot,
                                                ui **candidates) {
    VertexID u = order[depth];
    VertexID pivot_vertex = pivot[depth];
    ui idx_id = idx_embedding[pivot_vertex];
    Edges &edge = *edge_matrix[pivot_vertex][u];
    ui count = edge.offset_[idx_id + 1] - edge.offset_[idx_id];
    ui *candidate_idx = edge.edge_ + edge.offset_[idx_id];
    ui valid_candidate_index_count = 0;

    if (bn_cnt[depth] == 0) {
        for (ui i = 0; i < count; ++i) {
            ui temp_idx = candidate_idx[i];
            VertexID temp_v = candidates[u][temp_idx];
            if (!visited_vertices[temp_v])
                valid_candidate_index[depth][valid_candidate_index_count++] = temp_idx;
        }
    } else {
        for (ui i = 0; i < count; ++i) {
            ui temp_idx = candidate_idx[i];
            VertexID temp_v = candidates[u][temp_idx];

            if (!visited_vertices[temp_v]) {
                bool valid = true;
                for (ui j = 0; j < bn_cnt[depth]; ++j) {
                    VertexID u_bn = bn[depth][j];
                    VertexID u_bn_v = embedding[u_bn];

                    if (!data_graph->checkEdgeExistence(temp_v, u_bn_v)) {
                        valid = false;
                        break;
                    }
                }
                if (valid)
                    valid_candidate_index[depth][valid_candidate_index_count++] = temp_idx;
            }
        }
    }
    idx_count[depth] = valid_candidate_index_count;
}

void EvaluateQuery::releaseBuffer(ui query_vertices_num, ui *idx, ui *idx_count, ui *embedding, ui *idx_embedding,
                                  ui *temp_buffer, ui **valid_candidate_idx, bool *visited_vertices, ui **bn,
                                  ui *bn_count) {
    delete[] idx;
    delete[] idx_count;
    delete[] embedding;
    delete[] idx_embedding;
    delete[] visited_vertices;
    delete[] bn_count;
    delete[] temp_buffer;
    for (ui i = 0; i < query_vertices_num; ++i) {
        delete[] valid_candidate_idx[i];
        delete[] bn[i];
    }
    delete[] valid_candidate_idx;
    delete[] bn;
}