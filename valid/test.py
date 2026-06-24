import sys
import os
import glob
from subprocess import Popen, PIPE


def generate_args(binary, *params):
    arguments = [binary]
    arguments.extend(list(params))
    return arguments


def execute_binary(args):
    process = Popen(" ".join(args), shell=True, stdout=PIPE, stderr=PIPE)
    (std_output, std_error) = process.communicate()
    process.wait()
    rc = process.returncode
    return rc, std_output, std_error


def check_correctness(
    name, binary_path, data_graph_path, query_folder_path, expected_results
):
    # find all query graphs.
    query_graph_path_list = glob.glob("{0}/*.graph".format(query_folder_path))
    d2 = "-d"
    q2 = "-q"
    query_graph_path_list = sorted(query_graph_path_list)

    # check the correctness
    for query_graph_path in query_graph_path_list:
        execution_args = generate_args(
            binary_path[0],
            d2,
            data_graph_path,
            q2,
            query_graph_path,
            *binary_path[1:]
        )
        # print(execution_args)
        (rc, std_output, std_error) = execute_binary(execution_args)
        query_graph_name = os.path.splitext(os.path.basename(query_graph_path))[0]
        expected_embedding_num = expected_results[query_graph_name]

        if rc == 0:
            embedding_num = 0
            std_output_list = std_output.split(b"\n")
            # print(std_output_list)
            for line in std_output_list[::-1]:
                if b"#Embeddings" in line or b"Number of Matches" in line:
                    embedding_num = int(line.split(b":")[1].strip())
                    break
            if embedding_num != expected_embedding_num:
                print("EXPLORE engine {0} is wrong. Expected {1}, Output {2}".format(
                    query_graph_name, expected_embedding_num, embedding_num
                ))
                continue
        else:
            print("{} error.".format(binary_path[0]))
            return
    print(f"{binary_path[0]} pass the correctness check.")

if __name__ == "__main__":
    binary_path_with_config={
        "VSMatch_with_VSSim": ["../build/matching/main.out", "-filter VSSim -order NULL -engine VSMatch -num MAX"],
        "VSMatch_with_CFL": ["../build/matching/main.out", "-filter CFL -order NULL -engine VSMatch -num MAX -equivalent 1"],
        "General": ["../build/matching/main.out", "-filter CFL -order GQL -engine EXPLORE -num MAX"],
    }

    # load expected results.
    input_expected_results = {}
    input_expected_results_file = "expected_output.res"
    with open(input_expected_results_file, "r") as f:
        for line in f:
            if line:
                result_item = line.split(":")
                input_expected_results[result_item[0].strip()] = int(
                    result_item[1].strip()
                )

    for name, binary_path in binary_path_with_config.items():
 
        input_data_graph_path = "./data_graph/HPRD.graph"
        input_query_graph_folder_path = "./query_graph/"
        print("="*48)
        print("Checking correctness for : {}".format(name))
        print("Binary path: {}".format(binary_path[0]))
        print("Binary config: {}".format(binary_path[1:]))
        check_correctness(
            name,
            binary_path,
            input_data_graph_path,
            input_query_graph_folder_path,
            input_expected_results,
        )
        print("="*48)
