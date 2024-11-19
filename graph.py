import json
import os
from collections import defaultdict

import matplotlib.pyplot as plt

RESULT_JULIA_DIRECTORY = "result_julia"

NTHREADS = [i + 1 for i in range(12)]

DEFAULT_METHOD = "serial_default_implementation"
METHODS = [
    DEFAULT_METHOD,
    "intrinsics_atomic_add",
    "atomix_atomic_add",
]

DATA_CLASS = {
    "1024-0.1": "uniform",
    "8192-0.1": "uniform",
    "1048576-3000000": "uniform",
    "FEMLAB-poisson3Da": "FEMLAB",
    "FEMLAB-poisson3Db": "FEMLAB",
}

COLORS_JULIA = ["gray", "cadetblue", "saddlebrown"]

MEASUREMENTS_JSON = "measurements.json"
RESULT_DIRECTORY = "result"
GRAPH_DIRECTORY = "graph"
RUNTIME_DIRECTORY = "runtime"
SPEEDUP_DIRECTORY = "speedup"

COLORS_CPP = ["navy", "black", "orange"]


def load_json():
    combine_results = defaultdict(lambda: defaultdict(lambda: defaultdict(lambda: {})))
    for n_thread in NTHREADS:
        results_json = json.load(
            open(f"{RESULT_JULIA_DIRECTORY}/spmv_{n_thread}_threads.json", "r")
        )
        for result in results_json:

            matrix = (
                result["matrix"].replace("/", "-")
                if result["dataset"] != "uniform"
                else f"{result['matrix']['size']}-{result['matrix']['sparsity']}"
            )
            combine_results[result["dataset"]][matrix][result["method"]][
                result["n_threads"]
            ] = result["time"]

    return combine_results


def find_measurements(result_directory):
    found_files = []

    for root, _, files in os.walk(result_directory):
        if MEASUREMENTS_JSON in files:
            found_files.append(os.path.join(root, MEASUREMENTS_JSON))

    return found_files


def plot_runtime(result, result_julia, dataset):
    dataset_index = dataset.replace("/", "-")
    plt.figure(figsize=(10, 6))
    for (method, method_result), color in zip(result.items(), COLORS_CPP):
        plt.plot(
            range(1, len(method_result) + 1),
            [method_result[str(thread)] for thread in range(1, len(method_result) + 1)],
            label=f"CPP: {method}",
            color=color,
            marker="o",
            linestyle="-",
            linewidth=1,
        )

    for method, color in zip(METHODS, COLORS_JULIA):
        plt.plot(
            NTHREADS,
            [
                1e9
                * result_julia[DATA_CLASS[dataset_index]][dataset_index][method][
                    n_thread
                ]
                for n_thread in NTHREADS
            ],
            label=f"Julia: {method}",
            color=color,
            marker="o",
            linestyle="-",
            linewidth=1,
        )

    plt.title(f"SpMV - Runtime for {dataset}")
    plt.legend()
    plt.xlabel("Number of Threads")
    plt.ylabel(f"Runtime (in nanoseconds)")

    plt.savefig(
        os.path.join(GRAPH_DIRECTORY, RUNTIME_DIRECTORY, f"{dataset_index}.png")
    )


def plot_speedup(result, result_julia, dataset):
    dataset_index = dataset.replace("/", "-")
    plt.figure(figsize=(10, 6))
    for (method, method_result), color in zip(result.items(), COLORS_CPP):
        plt.plot(
            range(1, len(method_result) + 1),
            [
                result[DEFAULT_METHOD][str(thread)] / method_result[str(thread)]
                for thread in range(1, len(method_result) + 1)
            ],
            label=f"CPP: {method}",
            color=color,
            marker="o",
            linestyle="-",
            linewidth=1,
        )

    for method, color in zip(METHODS, COLORS_JULIA):
        plt.plot(
            NTHREADS,
            [
                result_julia[DATA_CLASS[dataset_index]][dataset_index][DEFAULT_METHOD][
                    n_thread
                ]
                / result_julia[DATA_CLASS[dataset_index]][dataset_index][method][
                    n_thread
                ]
                for n_thread in NTHREADS
            ],
            label=f"Julia: {method}",
            color=color,
            marker="o",
            linestyle="-",
            linewidth=1,
        )

    plt.title(f"SpMV - Speedup for {dataset} (with respect to {DEFAULT_METHOD})")
    plt.legend()
    plt.xlabel("Number of Threads")
    plt.ylabel(f"Speedup")

    plt.savefig(
        os.path.join(GRAPH_DIRECTORY, SPEEDUP_DIRECTORY, f"{dataset_index}.png")
    )


def plot_result(result, result_julia, dataset):
    plot_runtime(result, result_julia, dataset)
    plot_speedup(result, result_julia, dataset)


if __name__ == "__main__":
    measurement_files = find_measurements(RESULT_DIRECTORY)
    result_julia = load_json()

    for measurement_file in measurement_files:
        dataset = os.path.join(
            *os.path.normpath(measurement_file).split(os.path.sep)[1:-1]
        )

        result = json.load(open(measurement_file, "r"))

        plot_result(result, result_julia, dataset)
