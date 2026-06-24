#ifndef SUBGRAPHMATCHING_CONFIG_H
#define SUBGRAPHMATCHING_CONFIG_H

/**
 * Set the maximum size of a query graph. By default, we set the value as 64.
 */
#define MAXIMUM_QUERY_GRAPH_SIZE 64
#define MAXIMUM_DATA_GRAPH_SIZE 4*1024*1024
#define INVALID_VERTEX_ID 4294967295
#define HASH_TABLE_RATIO 1.2

#define VSSIM_NLF
#define VSSIM_MERGE
#define VSSIM_USE_ALL_EQV

/**
 * Setting the value as 1 is to (1) enable the neighbor label frequency filter (i.e., NLF filter); and (2) enable
 * to check the existence of an edge with the label information. The cost is to (1) build an unordered_map for each
 * vertex to store the frequency of the labels of its neighbor; and (2) build the label neighbor offset.
 * If the memory can hold the extra memory cost, then enable this feature to boost the performance. Otherwise, disable
 * it by setting this value as 0.
 */
#define OPTIMIZED_LABELED_GRAPH 1

/**
 * Set intersection method.
 * 0: Hybrid method; 1: Merge based set intersections.
 */
#define HYBRID 0

/**
 * Accelerate set intersection with SIMD instructions.
 * 0: AVX2; 1: AVX512; 2: Basic;
 */
#define SI 0

/**
 * Define ENABLE_QFLITER to enable QFliter set intersection method.
 */
// #define ENABLE_QFLITER 1

#define PRINT_SEPARATOR "------------------------------"

#endif 
