#include "../src/benchmark.hpp"
#include <omp.h>
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <sys/stat.h>
#include <vector>

namespace fs = std::filesystem;

template <typename T, typename I>
void experiment_spmv_csr(benchmark_params_t params);

int main(int argc, char **argv) {
  auto params = parse(argc, argv);
  auto A_desc = json::parse(std::ifstream(fs::path(params.input) / "A.bspnpy" /
                                          "binsparse.json"))["binsparse"];
  auto x_desc = json::parse(std::ifstream(fs::path(params.input) / "x.bspnpy" /
                                          "binsparse.json"))["binsparse"];

  // print format
  if (A_desc["format"] != "CSR") {
    throw std::runtime_error("Only CSR format for A is supported");
  }
  if (x_desc["format"] != "DVEC") {
    throw std::runtime_error("Only dense format for x is supported");
  }
  if (A_desc["data_types"]["pointers_to_1"] == "int32" &&
      A_desc["data_types"]["values"] == "float64") {
    experiment_spmv_csr<double, int32_t>(params);
  } else if (A_desc["data_types"]["pointers_to_1"] == "int64" &&
             A_desc["data_types"]["values"] == "float64") {
    experiment_spmv_csr<double, int64_t>(params);
  } else {
    std::cout << "pointers_to_1_type: " << A_desc["data_types"]["pointers_to_1"]
              << std::endl;
    std::cout << "values_type: " << A_desc["data_types"]["values"] << std::endl;
    throw std::runtime_error("Unsupported data types");
  }

  return 0;
}

template <typename T, typename I>
void remove_zeros_from_csr(int m, const std::vector<I> &A_ptr,
                           const std::vector<I> &A_idx,
                           const std::vector<T> &A_val,
                           std::vector<I> &new_A_ptr, std::vector<I> &new_A_idx,
                           std::vector<T> &new_A_val) {
  new_A_ptr.resize(m + 1, 0);
  for (int i = 0; i < m; i++) {
    for (int p = A_ptr[i]; p < A_ptr[i + 1]; p++) {
      if (A_val[p] != 0) {
        new_A_ptr[i + 1]++;
      }
    }
  }

  for (int i = 0; i < m; i++) {
    new_A_ptr[i + 1] += new_A_ptr[i];
  }

  int new_nnz = new_A_ptr[m];
  new_A_idx.resize(new_nnz);
  new_A_val.resize(new_nnz);

  int pos = 0;
  for (int i = 0; i < m; i++) {
    for (int p = A_ptr[i]; p < A_ptr[i + 1]; p++) {
      if (A_val[p] != 0) {
        new_A_idx[pos] = A_idx[p];
        new_A_val[pos] = A_val[p];
        pos++;
      }
    }
  }
}

template <typename T, typename I>
void csr_to_csc(int m, int n, const std::vector<I> &A_ptr,
                const std::vector<I> &A_idx, const std::vector<T> &A_val,
                std::vector<I> &B_ptr, std::vector<I> &B_idx,
                std::vector<T> &B_val) {
  B_ptr.assign(n + 1, 0);

  for (int i = 0; i < m; i++) {
    for (int p = A_ptr[i]; p < A_ptr[i + 1]; p++) {
      int j = A_idx[p];
      B_ptr[j + 1]++;
    }
  }

  for (int j = 0; j < n; j++) {
    B_ptr[j + 1] += B_ptr[j];
  }

  B_idx.resize(A_idx.size());
  B_val.resize(A_val.size());
  std::vector<I> next(n, 0);
  for (int i = 0; i < n; i++) {
    next[i] = B_ptr[i];
  }

  for (int i = 0; i < m; i++) {
    for (int p = A_ptr[i]; p < A_ptr[i + 1]; p++) {
      int j = A_idx[p];
      T val = A_val[p];
      int dest = next[j]++;
      B_idx[dest] = i;
      B_val[dest] = val;
    }
  }
}

template <typename T, typename I>
void experiment_spmv_csr(benchmark_params_t params) {
  auto A_desc = json::parse(std::ifstream(fs::path(params.input) / "A.bspnpy" /
                                          "binsparse.json"))["binsparse"];
  auto x_desc = json::parse(std::ifstream(fs::path(params.input) / "x.bspnpy" /
                                          "binsparse.json"))["binsparse"];

  int m = A_desc["shape"][0];
  int n = A_desc["shape"][1];

  auto x_val =
      npy_load_vector<T>(fs::path(params.input) / "x.bspnpy" / "values.npy");
  auto A_ptr = npy_load_vector<I>(fs::path(params.input) / "A.bspnpy" /
                                  "pointers_to_1.npy");
  auto A_idx =
      npy_load_vector<I>(fs::path(params.input) / "A.bspnpy" / "indices_1.npy");
  auto A_val =
      npy_load_vector<T>(fs::path(params.input) / "A.bspnpy" / "values.npy");

  auto y_val = std::vector<T>(m, 0);

  auto y_val_target = npy_load_vector<T>(fs::path(params.input) /
                                         "y_ref.bspnpy" / "values.npy");

  std::vector<I> B_ptr, B_idx, C_ptr, C_idx;
  std::vector<T> B_val, C_val;
  remove_zeros_from_csr<T, I>(m, A_ptr, A_idx, A_val, B_ptr, B_idx, B_val);
  csr_to_csc<T, I>(m, n, B_ptr, B_idx, B_val, C_ptr, C_idx, C_val);

  // perform an spmv of the matrix in c++
  std::vector<long long> times;
  for (int i = 0; i < params.max_threads; i++) {
    int num_threads = i + 1;
	omp_set_num_threads(num_threads);
	std::cout << "Running with " << num_threads << " threads" << std::endl;
    auto result = benchmark([]() {},
                            [&y_val, &C_ptr, &C_val, &C_idx, &x_val, &m, &n]() {
							#pragma omp parallel for
                              for (int j = 0; j < n; j++) {
                                for (int p = C_ptr[j]; p < C_ptr[j + 1]; p++) {
                                  int i = C_idx[p];
								  auto temp = C_val[p] * x_val[j];
								#pragma omp atomic
                                  y_val[i] += temp;
                                }
                              }
                            });
	std::cout << "Runtime: " << result.first << std::endl;

    for (int i = 0; i < m; i++) {
      if ((float)(y_val[i] / (double)result.second) != (float)y_val_target[i]) {
        std::cout << "Got: " << y_val[i] / (double)result.second
                  << "; Expected: " << y_val_target[i] << std::endl;
      }
    }

	std::fill(y_val.begin(), y_val.end(), 0);

    times.push_back(result.first);
  }

  std::filesystem::create_directory(fs::path(params.output) / "y.bspnpy");
  json y_desc;
  y_desc["version"] = 0.5;
  y_desc["format"] = "DVEC";
  y_desc["shape"] = {n};
  y_desc["nnz"] = n;
  y_desc["data_types"]["values_type"] = "float64";
  std::ofstream y_desc_file(fs::path(params.output) / "y.bspnpy" /
                            "binsparse.json");
  y_desc_file << y_desc;
  y_desc_file.close();

  npy_store_vector<T>(fs::path(params.output) / "y.bspnpy" / "values.npy",
                      y_val);

  json measurements;
  for (int i = 0; i < params.max_threads; i++) {
    measurements["time"][std::to_string(i + 1)] = times[i];
  }
  measurements["memory"] = 0;
  std::ofstream measurements_file(fs::path(params.output) /
                                  "measurements.json");
  measurements_file << measurements;
  measurements_file.close();
}
