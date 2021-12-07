#include <chrono>
#include <random>
#include <iostream>
#include <vector>
#include <cassert>
#include "bfs.cpp"

uint32_t indexes2vertex(int i, int j, int k, uint32_t cube_sz) {
    return i + cube_sz * j + cube_sz * cube_sz * k;
}

void build_cubic_graph(std::vector<std::vector<uint32_t>> &edges, uint32_t cube_sz) {
    uint32_t sz = cube_sz * cube_sz * cube_sz;
    for (int i = 0; i < sz; ++i) {
        edges.emplace_back();
    }
    for (int i = 0; i < cube_sz; ++i) {
        for (int j = 0; j < cube_sz; ++j) {
            for (int k = 0; k < cube_sz; ++k) {
                uint32_t cur = indexes2vertex(i, j, k, cube_sz);
                uint32_t nbr;
                if (i + 1 != cube_sz) {
                    nbr = indexes2vertex(i + 1, j, k, cube_sz);
                    edges[cur].push_back(nbr);
                    edges[nbr].push_back(cur);
                }
                if (j + 1 != cube_sz) {
                    nbr = indexes2vertex(i, j + 1, k, cube_sz);
                    edges[cur].push_back(nbr);
                    edges[nbr].push_back(cur);
                }
                if (k + 1 != cube_sz) {
                    nbr = indexes2vertex(i, j, k + 1, cube_sz);
                    edges[cur].push_back(nbr);
                    edges[nbr].push_back(cur);
                }
            }
        }
    }
}

int main() {
    uint32_t reps = 5;
    uint32_t cube_sz = 500;
    uint32_t graph_sz = cube_sz * cube_sz * cube_sz;
    std::vector<std::vector<uint32_t>> edges;
    build_cubic_graph(edges, cube_sz);

    uint64_t sum1 = 0;
    std::vector<int> vs1;
    for (int i = 0; i < reps; ++i) {
        vs1.clear();
        vs1.resize(graph_sz, -1);
        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        bfs_sequential(vs1, edges);
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        sum1 += std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    }
    std::cout << "Sequential: " << sum1 / reps << " milliseconds" << std::endl;

    uint64_t sum2 = 0;
    std::vector<std::atomic_int> vs2(graph_sz);
    for (int i = 0; i < reps; ++i) {
        for (int j = 0; j < graph_sz; ++j) {
            vs2[j] = -1;
        }

        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        bfs_parallel(vs2, edges, 32);
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        sum2 += std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    }
    std::cout << "Parallel: " << sum2 / reps << " milliseconds" << std::endl;

    for (int i = 0; i < cube_sz; ++i) {
        for (int j = 0; j < cube_sz; ++j) {
            for (int k = 0; k < cube_sz; ++k) {
                uint32_t cur = indexes2vertex(i, j, k, cube_sz);
                assert(vs1[cur] == vs2[cur].load());
                assert(vs1[cur] == i + j + k);
            }
        }
    }

    std::cout << "Coefficient " << 1.0 * sum1 / sum2 << std::endl;
    return 0;
}