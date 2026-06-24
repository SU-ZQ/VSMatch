



#ifndef SUBGRAPHMATCHING_FILTERVERTICES_H
#define SUBGRAPHMATCHING_FILTERVERTICES_H

#include "graph/graph.h"
#include <map>
#include <unordered_set>
#include <set>
#include <bitset>
#include <vector>
#include <boost/dynamic_bitset.hpp>

class FilterVertices {
public:
    struct pair_hash {
        template <class T1, class T2>
        std::size_t operator () (const std::pair<T1,T2> &p) const {
            auto h1 = std::hash<T1>{}(p.first);
            auto h2 = std::hash<T2>{}(p.second);
            return h1 ^ h2;
        }
    };

    struct pair_equal {
        template <class T1, class T2>
        bool operator()(const std::pair<T1, T2>& lhs, const std::pair<T1, T2>& rhs) const {
            return lhs.first == rhs.first && lhs.second == rhs.second;
        }
    };
    static void get_equivanlence_base(const Graph *data_graph, const Graph *query_graph, ui **candidates, ui *candidates_count,
                                ui **&eqv_classes, ui **&eqv_offsets, ui **&eqv_counts);
    static void compute_equal_set_base(const Graph *data_graph, VertexID query_vertex,
                                  ui **candidates, ui *candidates_count, ui **eqv_classes, ui **eqv_offsets, ui **eqv_counts,boost::dynamic_bitset<> &neighbor_valid_candiate_flag);
    static bool LDFFilter(const Graph *data_graph, const Graph *query_graph, ui **&candidates, ui *&candidates_count);
    static bool NLFFilter(const Graph* data_graph, const Graph* query_graph, ui** &candidates, ui* &candidates_count);
    static bool GQLFilter(const Graph *data_graph, const Graph *query_graph, ui **&candidates, ui *&candidates_count);   
    static bool MergeEquivalenceClasses(const Graph *data_graph, const Graph *query_graph, ui **candidates, ui *candidates_count,
                                             ui **eqv_classes, ui **eqv_offsets, ui **eqv_counts, bool **valid_candidates); 
    static bool VSSimFilterInitial(const Graph *data_graph, const Graph *query_graph, ui **&candidates, ui *&candidates_count,
                                  ui *order, TreeNode *tree);
    static bool VSSimFilterHelper(const Graph *data_graph, const Graph *query_graph, ui **candidates, ui *candidates_count,
                         ui **eqv_classes, ui **eqv_offsets, ui **eqv_counts,
                         ui *order, TreeNode *tree, ui *updated_flag, ui *flag, bool **valid_candidates, bool reverse, bool using_triangle);
    static bool VSSimFilter(const Graph *data_graph, const Graph *query_graph, ui **&candidates, ui *&candidates_count, 
                                ui **&eqv_classes, ui **&eqv_offsets, ui **&eqv_counts);
    static bool CFLFilter(const Graph *data_graph, const Graph *query_graph, ui **&candidates, ui *&candidates_count,
                              ui *&order, TreeNode *&tree);
    static bool DPisoFilter(const Graph *data_graph, const Graph *query_graph, ui **&candidates, ui *&candidates_count,
                            ui *&order, TreeNode *&tree);

    static bool VEQFilter(const Graph *data_graph, const Graph *query_graph, ui **&candidates, ui *&candidates_count,
                          ui *&order, TreeNode *&tree);


    static void computeCandidateWithNLF(const Graph *data_graph, const Graph *query_graph, VertexID query_vertex,
                                        ui &count, ui *buffer = NULL);

    static void computeCandidateWithLDF(const Graph *data_graph, const Graph *query_graph, VertexID query_vertex,
                                        ui &count, ui *buffer = NULL);

    static void generateCandidates(const Graph *data_graph, const Graph *query_graph, VertexID query_vertex,
                                      VertexID *pivot_vertices, ui pivot_vertices_count, VertexID **candidates,
                                      ui *candidates_count, ui *flag, ui *updated_flag);

    static void pruneCandidates(const Graph *data_graph, const Graph *query_graph, VertexID query_vertex,
                                   VertexID *pivot_vertices, ui pivot_vertices_count, VertexID **candidates,
                                   ui *candidates_count, ui *flag, ui *updated_flag);
    static void VEQPruneCandidates(const Graph *data_graph, const Graph *query_graph, VertexID query_vertex,
                                   VertexID *child_vertices, ui child_vertices_count, VertexID **candidates,
                                   ui *candidates_count, std::vector<std::set<VertexID>>& D);

    static void VEQCompactCandidates(ui** &candidates, ui* &candidates_count, ui query_vertex_num,
                                     std::vector<std::set<VertexID>>& D);

    static void sortCandidates(ui** candidates, ui* candidates_count, ui num);
    static void sortCandidates(ui** candidates, ui* candidates_count, ui num, ui **eqv_counts);

private:
    static void allocateBuffer(const Graph* data_graph, const Graph* query_graph, ui** &candidates, ui* &candidates_count);
    static bool verifyExactTwigIso(const Graph *data_graph, const Graph *query_graph, ui data_vertex, ui query_vertex,
                                   bool **valid_candidates, int *left_to_right_offset, int *left_to_right_edges,
                                   int *left_to_right_match, int *right_to_left_match, int* match_visited,
                                   int* match_queue, int* match_previous);
    static bool triangleCheck(const Graph* data_graph, const Graph* query_graph, ui data_vertex, ui query_vertex);
    static bool verifyExactTwigIso_with_Triangle_init(const Graph *data_graph, const Graph *query_graph, ui data_vertex, ui query_vertex,
                                   bool **valid_candidates, int *left_to_right_offset, int *left_to_right_edges,
                                   int *left_to_right_match, int *right_to_left_match, int* match_visited,
                                   int* match_queue, int* match_previous,
                                   std::unordered_map<uint64_t, std::vector<std::unordered_set<VertexID>>> &valid_candidates_map);
    static bool verifyExactTwigIso_with_Triangle(const Graph *data_graph, const Graph *query_graph, ui data_vertex, ui query_vertex,
                                   bool **valid_candidates, int *left_to_right_offset, int *left_to_right_edges,
                                   int *left_to_right_match, int *right_to_left_match, int* match_visited,
                                   int* match_queue, int* match_previous,
                                   std::unordered_map<uint64_t, std::vector<std::unordered_set<VertexID>>> &valid_candidates_map);
    static bool verifyExactTwigIso(const Graph *data_graph, const Graph *query_graph, ui data_vertex, ui query_vertex,
                                   bool **valid_candidates, int *left_to_right_offset, int *left_to_right_edges,
                                   int *left_to_right_match, int *right_to_left_match, int* match_visited,
                                   int* match_queue, int* match_previous,
                                   std::vector<std::unordered_map<VertexID, std::unordered_set<VertexID>>> &candidate_neighbors);
    static void compactCandidates(ui** &candidates, ui* &candidates_count, ui query_vertex_num);
    static bool isCandidateSetValid(ui** &candidates, ui* &candidates_count, ui query_vertex_num);
};


#endif 
