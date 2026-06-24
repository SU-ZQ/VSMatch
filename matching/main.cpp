#include <chrono>

#include "matchingcommand.h"
#include "graph/graph.h"
#include "FilterVertices.h"
#include "GenerateFilteringPlan.h"
#include "BuildTable.h"
#include "GenerateQueryPlan.h"
#include "EvaluateQuery.h"

#define NANOSECTOSEC(elapsed_time) ((elapsed_time)/(double)1000000000)
#define NANOSECTOMILLISEC(elapsed_time) ((elapsed_time)/(double)1000000)
#define NANOSECTOMISCROSEC(elapsed_time) ((elapsed_time)/(double)1000)

int main(int argc, char** argv) {
    MatchingCommand command(argc, argv);
    std::string input_query_graph_file = command.getQueryGraphFilePath();
    std::string input_data_graph_file = command.getDataGraphFilePath();
    std::string input_filter_type = command.getFilterType();
    std::string input_order_type = command.getOrderType();
    std::string input_engine_type = command.getEngineType();
    std::string input_max_embedding_num = command.getMaximumEmbeddingNum();
    std::string input_time_limit = command.getTimeLimit();
    std::string input_enable_equivalence = command.getEnableEquivalence();
    bool enable_equivalence = input_enable_equivalence == "" ? false : true;
    size_t output_limit = 0;
    size_t time_limit = 0;
    if (input_max_embedding_num == "MAX") {
        output_limit = std::numeric_limits<size_t>::max();
    } else {
        sscanf(input_max_embedding_num.c_str(), "%zu", &output_limit);
    }
    sscanf(input_time_limit.c_str(), "%zu", &time_limit); 


    Graph* query_graph = new Graph(true);
    query_graph->loadGraphFromFile(input_query_graph_file);
    query_graph->buildCoreTable();
    query_graph->buildTriangleInfo();

    Graph* data_graph = new Graph(true);
    data_graph->loadGraphFromFile(input_data_graph_file);

    auto start_point = std::chrono::high_resolution_clock::now();
    auto start = std::chrono::high_resolution_clock::now();
    ui** candidates = NULL;
    ui* candidates_count = NULL;
    ui** eqv_classes = NULL;
    ui** eqv_offsets = NULL;
    ui** eqv_counts = NULL;
    ui* cfl_order = NULL;
    TreeNode* cfl_tree = NULL;
    ui* dpiso_order = NULL;
    TreeNode* dpiso_tree = NULL;   
    ui* veq_order = NULL; 
    TreeNode* veq_tree = NULL;
    bool filter_flag = true;
    if (input_filter_type == "LDF") {
        filter_flag = FilterVertices::LDFFilter(data_graph, query_graph, candidates, candidates_count);
    } else if (input_filter_type == "NLF") {
        filter_flag = FilterVertices::NLFFilter(data_graph, query_graph, candidates, candidates_count);
    } else if (input_filter_type == "GQL") {
        filter_flag = FilterVertices::GQLFilter(data_graph, query_graph, candidates, candidates_count);
    } else if (input_filter_type == "CFL") {
        filter_flag = FilterVertices::CFLFilter(data_graph, query_graph, candidates, candidates_count, cfl_order, cfl_tree);
    } else if (input_filter_type == "VEQ") {
        filter_flag = FilterVertices::VEQFilter(data_graph, query_graph, candidates, candidates_count, veq_order, veq_tree);        
    } else if (input_filter_type == "DPiso") {
        filter_flag = FilterVertices::DPisoFilter(data_graph, query_graph, candidates, candidates_count, dpiso_order, dpiso_tree);
    } else if (input_filter_type == "VSSim") {
        filter_flag = FilterVertices::VSSimFilter(data_graph, query_graph, candidates, candidates_count, eqv_classes, eqv_offsets, eqv_counts);
    } else {
        std::cout << "The specified filter type '" << input_filter_type << "' is not supported." << std::endl;
        exit(-1);
    }
    if (eqv_classes == NULL){
        FilterVertices::sortCandidates(candidates, candidates_count, query_graph->getVerticesCount());
    }    
    auto end = std::chrono::high_resolution_clock::now();
    double filter_vertices_time_in_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    
    if (!filter_flag) {
        std::cout << "No candidate after filtering. Stop." << std::endl;
        std::cout << "#Embeddings: 0" << std::endl;
        return 0;
    }
    
    if (enable_equivalence && eqv_classes == NULL) {
        std::cout << "Get equivalence ..." << std::endl;
        start = std::chrono::high_resolution_clock::now();
        FilterVertices::get_equivanlence_base(data_graph, query_graph, candidates, candidates_count, eqv_classes, eqv_offsets, eqv_counts);
        end = std::chrono::high_resolution_clock::now();
        double get_equivalence_time_in_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        filter_vertices_time_in_ns += get_equivalence_time_in_ns;
    }

    start = std::chrono::high_resolution_clock::now();
    Edges ***edge_matrix = NULL;
    edge_matrix = new Edges **[query_graph->getVerticesCount()];
    for (ui i = 0; i < query_graph->getVerticesCount(); ++i) {
        edge_matrix[i] = new Edges *[query_graph->getVerticesCount()];
    }
    BuildTable::buildTables(data_graph, query_graph, candidates, candidates_count, edge_matrix);
    end = std::chrono::high_resolution_clock::now();
    double build_table_time_in_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    ui* matching_order = NULL;
    ui* pivots = NULL;
    if (input_order_type == "GQL") {
        GenerateQueryPlan::generateGQLQueryPlan(data_graph, query_graph, candidates_count, matching_order, pivots);
    } else if (input_order_type == "NULL") {
        matching_order = new ui[query_graph->getVerticesCount()];
        pivots = new ui[query_graph->getVerticesCount()];
    } else {
        std::cout << "The specified order type '" << input_order_type << "' is not supported." << std::endl;
    }

    mpz_class embedding_count = 0;
    size_t call_count = 0;
    start = std::chrono::high_resolution_clock::now();
    
    if (input_engine_type == "EXPLORE") {       
        embedding_count = EvaluateQuery::exploreGraph(data_graph, query_graph, edge_matrix, candidates,
                                                      candidates_count, matching_order, pivots, output_limit, call_count, time_limit, start_point);
    } else if (input_engine_type == "VSMatch"){
        embedding_count = EvaluateQuery::VSMatch(data_graph, query_graph, edge_matrix, candidates,
                                                    candidates_count, eqv_classes, eqv_offsets, eqv_counts, matching_order, pivots, output_limit, call_count, time_limit, start_point);
    } else {
        std::cout << "The specified engine type '" << input_engine_type << "' is not supported." << std::endl;
        exit(-1);
    }
    end = std::chrono::high_resolution_clock::now();
    double enumerate_time_in_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    double total_time_in_ns = filter_vertices_time_in_ns + build_table_time_in_ns + enumerate_time_in_ns;

    printf("Filter time (ms): %.4lf\n", NANOSECTOMILLISEC(filter_vertices_time_in_ns));
    printf("Enumerate time (ms): %.4lf\n", NANOSECTOMILLISEC(enumerate_time_in_ns));
    printf("Total time (ms): %.4lf\n", NANOSECTOMILLISEC(total_time_in_ns));
    std::cout << "#Embeddings: " << embedding_count.get_str() << std::endl;
    printf("Call Count: %zu\n", call_count);
    std::cout << "End." << std::endl;
    return 0;
}