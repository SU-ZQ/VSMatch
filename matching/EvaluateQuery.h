



#ifndef SUBGRAPHMATCHING_EVALUATEQUERY_H
#define SUBGRAPHMATCHING_EVALUATEQUERY_H

#include "graph/graph.h"
#include <vector>
#include <queue>
#include <unordered_set>
#include <bitset>
#include <gmpxx.h>
#include <iostream>
#include <chrono>

class EvaluateQuery {
public:
                                  
    static size_t exploreGraph(const Graph *data_graph, const Graph *query_graph, Edges ***edge_matrix, ui **candidates,ui *candidates_count, 
                                ui *order, ui *pivot, size_t output_limit_num, size_t &call_count, size_t time_limit, std::chrono::high_resolution_clock::time_point start_time);

    static size_t 
    update_label_cnt_isolate_dfs_base(const Graph *data_graph, const Graph *query_graph, ui *idx_embedding, 
                        LabelID label_id, ui *vertex2depth, ui **valid_candidate_idx, ui *idx_count, ui **eqv_classes, ui **eqv_offsets, bool *visited_vertices_for_cnt, bool *pass_non_pos, bool *is_isolate, ui *idx_for_cnt, ui *part_emb_for_cnt, ui *idx_start_for_cnt, ui *idx_end_for_cnt, ui *non_pos_for_cnt, ui *valid_v_for_cnt, uint8_t *vertex_count_for_cnt, size_t *depth_embedding_cnt);

    static size_t 
    update_label_cnt_isolate_dfs_base_backup2(const Graph *data_graph, const Graph *query_graph, ui *idx_embedding, 
                        LabelID label_id, ui *vertex2depth, ui **valid_candidate_idx, ui *idx_count, ui **eqv_classes, ui **eqv_offsets, bool *visited_vertices_for_cnt, bool *pass_non_pos, bool *is_isolate, ui *idx_for_cnt, ui *part_emb_for_cnt, ui *idx_start_for_cnt, ui *idx_end_for_cnt, ui *non_pos_for_cnt, ui *valid_v_for_cnt, uint8_t *vertex_count_for_cnt, size_t *depth_embedding_cnt);

    
    static bool check_isolate_order(const Graph *query_graph, ui *order, bool *is_isolate, ui non_isolate_count);
    static ui split_isolate_max(const Graph *query_graph, ui *candidates_count, ui *order, ui *pivot, bool *is_isolate);
    static bool
    update_Extendable_Vertex(const Graph *data_graph, const Graph *query_graph, ui cur_u, ui *embedding,
                                             ui *idx_embedding, ui *idx_count, ui ** valid_candidate_idx, ui *u2depth, ui **valid_candidate_count_level, ui ***valid_candidate_index_level, Edges ***edge_matrix, bool *visited_vertices, int *visited_bn, ui **candidates,ui **eqv_counts);

    static mpz_class
    VSMatch(const Graph *data_graph, const Graph *query_graph, Edges ***edge_matrix, ui **candidates, 
                                    ui *candidates_count, ui **eqv_classes, ui **eqv_offsets, ui **eqv_counts, ui *order, ui *pivot, size_t output_limit_num, size_t &call_count, size_t time_limit, std::chrono::high_resolution_clock::time_point start_time);
private:
    static void generateBN(const Graph *query_graph, ui *order, ui *pivot, ui **&bn, ui *&bn_count);
    static void allocateBuffer(const Graph *query_graph, const Graph *data_graph, ui *candidates_count, ui *&idx,
                                   ui *&idx_count, ui *&embedding, ui *&idx_embedding, ui *&temp_buffer,
                                   ui **&valid_candidate_idx, bool *&visited_vertices);
    static void releaseBuffer(ui query_vertices_num, ui *idx, ui *idx_count, ui *embedding, ui *idx_embedding,
                                  ui *temp_buffer, ui **valid_candidate_idx, bool *visited_vertices, ui **bn, ui *bn_count);

    static void generateValidCandidateIndex(const Graph *data_graph, ui depth, ui *embedding, ui *idx_embedding,
                                            ui *idx_count, ui **valid_candidate_index, Edges ***edge_matrix,
                                            bool *visited_vertices, ui **bn, ui *bn_cnt, ui *order, ui *pivot,
                                            ui **candidates);

    static void generateValidCandidateIndex(const Graph *data_graph, ui depth, ui *embedding, ui *idx_embedding,
                                            ui *idx_count, ui **valid_candidate_index, Edges ***edge_matrix,
                                            bool *visited_vertices, ui **bn, ui *bn_cnt, ui *order, ui *pivot,
                                            ui **candidates,ui **eqv_counts);
};


#endif 
