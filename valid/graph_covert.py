import glob
import os,sys
def graph2gfu(input_path, output_path ,graph_id):
    with open(input_path, 'r') as fin:
        lines = fin.readlines()
    
    header = lines[0].strip().split()
    N = int(header[1])
    M = int(header[2])
    
    vertex_labels = []
    edge_list = []
    
    for i in range(1, N + 1):
        parts = lines[i].strip().split()
        vertex_labels.append(parts[2])
    
    for i in range(N + 1, N + 1 + M):
        parts = lines[i].strip().split()
        edge_list.append((parts[1], parts[2]))
    
    with open(output_path, 'w') as fout:
        fout.write(f"#{graph_id}\n")
        fout.write(f"{N}\n")
        for label in vertex_labels:
            fout.write(f"{label}\n")
        fout.write(f"{M}\n")
        for u, v in edge_list:
            fout.write(f"{u} {v}\n")

def gfu2graph(input_path, output_path):
    with open(input_path, 'r') as fin:
        lines = fin.readlines()
    
    N = int(lines[1].strip())
    vertex_labels = [lines[i + 2].strip() for i in range(N)]
    M = int(lines[N + 2].strip())
    edge_list = [lines[N + 3 + i].strip().split() for i in range(M)]
    vertex_degrees = [0] * N
    for u, v in edge_list:
        vertex_degrees[int(u)] += 1
        vertex_degrees[int(v)] += 1

    with open(output_path, 'w') as fout:
        fout.write(f"t {N} {M}\n")
        for i in range(N):
            fout.write(f"v {i} {vertex_labels[i]} {vertex_degrees[i]}\n")
        for u, v in edge_list:
            fout.write(f"e {u} {v}\n")

if __name__ == "__main__":
    data_graph_dir="./data_graph/"
    data_graph_path_list = glob.glob(data_graph_dir+"*.graph")
    data_graph_path_list = sorted(data_graph_path_list)
    for graph_id, graph_file in enumerate(data_graph_path_list):
        gfu_file=graph_file.replace(".graph",".gfu")
        if not os.path.exists(gfu_file):
            graph2gfu(graph_file, gfu_file, graph_id)
    print("Data graph conversion done.")
    query_graph_dir="./query_graph/"
    query_graph_path_list = glob.glob(query_graph_dir+"*.graph")
    query_graph_path_list = sorted(query_graph_path_list)
    for graph_id, graph_file in enumerate(query_graph_path_list):
        gfu_file=graph_file.replace(".graph",".gfu")
        if not os.path.exists(gfu_file):
            graph2gfu(graph_file, gfu_file, graph_id)
    print("Query graph conversion done.")
