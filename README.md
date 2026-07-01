# SubgraphMatching
## Introduction

Source code of `Compressing Search Spaces for Fast Subgraph Matching via Vertex-Set Simulation`


## Execution
Execute the following command to build the program.
```
mkdir build & cd build
cmake ..
make
```
Execute the following command to run the program.
```
./build/matching/main.out -d data/data.txt -q data/query.txt -filter VSSim -order NULL -engine VSMatch -num MAX
```
If using other filter, add `-equivalent 1`
```
./build/matching/main.out -d data/data.txt -q data/query.txt -filter CFL -order NULL -engine VSMatch -num MAX -equivalent 1

```

## Input
Both the input query graph and data graph are vertex-labeled.
Each graph starts with 't N M' where N is the number of vertices and M is the number of edges. A vertex and an edge are formatted
as 'v VertexID LabelId Degree' and 'e VertexId VertexId' respectively. Note that we require that the vertex
id is started from 0 and the range is [0,N - 1] where V is the vertex set. The following
is an input sample.

Example:

```bash
t 5 6
v 0 0 2
v 1 1 3
v 2 2 3
v 3 1 2
v 4 2 2
e 0 1
e 0 2
e 1 2
e 1 3
e 2 4
e 3 4
```
## Datasets
Datasets can be found at: [dataset](https://drive.google.com/drive/folders/1Fg3YLUByaghfspaWZeVN96_pKJhkELBV?usp=drive_link).

## References
[1] Zhijie Zhang, Yujie Lu, Weiguo Zheng, Xuemin Lin: A Comprehensive Survey and Experimental Study of Subgraph Matching: Trends, Unbiasedness, and Interaction. Proc. ACM Manag. Data 2(1): 60:1-60:29 (2024)

[2] Shixuan Sun, Qiong Luo: In-Memory Subgraph Matching: An In-depth Study. SIGMOD Conference 2020: 1083-1098
