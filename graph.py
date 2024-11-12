import json
import os

import matplotlib.pyplot as plt

MEASUREMENTS_JSON = "measurements.json"
RESULT_DIRECTORY = "result"
GRAPH_DIRECTORY = "graph"
RUNTIME_DIRECTORY = "runtime"
SPEEDUP_DIRECTORY = "speedup"


def find_measurements(result_directory):
    found_files = []

    for root, _, files in os.walk(result_directory):
        if MEASUREMENTS_JSON in files:
            found_files.append(os.path.join(root, MEASUREMENTS_JSON))

    return found_files


def plot_runtime(result, dataset):
    plt.figure(figsize=(10, 6))
    plt.plot(
        range(1, len(result) + 1),
        [result[str(thread)] for thread in range(1, len(result) + 1)],
        marker="o",
        linestyle="-",
        linewidth=1,
    )

    plt.title(f"SpMV - CPP Atomic Add Runtime for {dataset}")
    plt.xlabel("Number of Threads")
    plt.ylabel(f"Runtime (in nanoseconds)")

    plt.savefig(
        os.path.join(
            GRAPH_DIRECTORY, RUNTIME_DIRECTORY, f"{dataset.replace('/', '-')}.png"
        )
    )


def plot_speedup(result, dataset):
    plt.figure(figsize=(10, 6))
    plt.plot(
        range(1, len(result) + 1),
        [result["1"] / result[str(thread)] for thread in range(1, len(result) + 1)],
        marker="o",
        linestyle="-",
        linewidth=1,
    )

    plt.title(
        f"SpMV - CPP Atomic Add Speedup for {dataset} (with respect to one thread)"
    )
    plt.xlabel("Number of Threads")
    plt.ylabel(f"Speedup")

    plt.savefig(
        os.path.join(
            GRAPH_DIRECTORY, SPEEDUP_DIRECTORY, f"{dataset.replace('/', '-')}.png"
        )
    )


def plot_result(result, dataset):
    plot_runtime(result, dataset)
    plot_speedup(result, dataset)


if __name__ == "__main__":
    measurement_files = find_measurements(RESULT_DIRECTORY)

    for measurement_file in measurement_files:
        dataset = os.path.join(
            *os.path.normpath(measurement_file).split(os.path.sep)[1:-1]
        )

        result = json.load(open(measurement_file, "r"))["time"]

        plot_result(result, dataset)
