



#ifndef SUBGRAPHMATCHING_TYPES_H
#define SUBGRAPHMATCHING_TYPES_H

#include <cstdint>
#include <stdlib.h>

typedef unsigned int ui;

typedef uint32_t VertexID;
typedef ui LabelID;

#define UINT_MAX std::numeric_limits<ui>::max()

enum MatchingIndexType {
    VertexCentric = 0,
    EdgeCentric = 1
};

struct edge {
    uint32_t vertices_[2];
    edge(uint32_t src, uint32_t dst) {
        vertices_[0] = src;
        vertices_[1] = dst;
    }
    edge() {}
    uint32_t operator[](ui index) {
        return vertices_[index];
    }
};

class TreeNode {
public:
    VertexID id_;                // 当前查询图中节点的编号
    VertexID parent_;            // 在查询树中的父节点(order中的第一个前序顶点)
    ui level_;                   // 当前节点所在的层级（距离根节点的深度）
    ui under_level_count_;       // 当前节点的“下层节点”数量，可用于快速估计子树大小，在一些 DP-based 或启发式算法中，用于估算该节点搜索空间大小，在剪枝时，用于快速跳过整棵子树。
    ui children_count_;          // 当前节点的子节点数量
    ui bn_count_;                // backward neighbors 数量
    ui fn_count_;                // forward neighbors 数量
    VertexID* under_level_;      // 指向下层节点的数组
    VertexID* children_;         // 指向子节点数组，通常结合 children_count_ 一起使用，如，当匹配完父节点后，递归地对所有子节点进行候选筛选。
    VertexID* bn_;               // backward neighbors 数组，已经在查询树中被匹配过的邻居，常用于在匹配过程中，确保当前节点与所有已匹配的邻居之间的约束得到满足。
    VertexID* fn_;               // forward neighbors 数组，尚未匹配但相邻的节点，当当前节点被匹配时，可以利用其 forward neighbors 更新后续节点的候选集（candidate filtering）。
    size_t estimated_embeddings_num_;  // 估计该节点对应的嵌入数量（候选匹配数），用于估计该节点在数据图中可能的候选数量，帮助优化匹配顺序或选择合适的过滤策略。
public:
    TreeNode() {
        id_ = 0;
        under_level_ = NULL;
        bn_ = NULL;
        fn_ = NULL;
        children_ = NULL;
        parent_ = 0;
        level_ = 0;
        under_level_count_ = 0;
        children_count_ = 0;
        bn_count_ = 0;
        fn_count_ = 0;
        estimated_embeddings_num_ = 0;
    }

    ~TreeNode() {
        delete[] under_level_;
        delete[] bn_;
        delete[] fn_;
        delete[] children_;
    }

    void initialize(const ui size) {
        under_level_ = new VertexID[size];
        bn_ = new VertexID[size];
        fn_ = new VertexID[size];
        children_ = new VertexID[size];
    }
};

// 用来记录两个查询顶点之间的边在数据图中候选点之间的邻居关系
class Edges {
public:
    ui* offset_; // 记录每个候选点的邻居在另一个候选集中的起始位置，CSR格式
    ui* edge_; // 记录每个候选点的邻居在另一个候选集中的位置(不是具体的顶点ID，而是该顶点在候选集中的下标)
    ui vertex_count_; // 查询顶点对应的候选点数量
    ui edge_count_;
    ui max_degree_;
public:
    Edges() {
        offset_ = NULL;
        edge_ = NULL;
        vertex_count_ = 0;
        edge_count_ = 0;
        max_degree_ = 0;
    }

    ~Edges() {
        delete[] offset_;
        delete[] edge_;
    }
};

#endif 
