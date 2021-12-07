#include <cilk/cilk.h>
#include <cilk/cilk_api.h>
#include <vector>
#include <queue>
#include <atomic>

void bfs_sequential(std::vector<int> &vertices, const std::vector<std::vector<uint32_t>> &edges) {
    std::queue<uint32_t> current;
    current.push(0);
    vertices[0] = 0;
    while (!current.empty()) {
        uint32_t cur = current.front();
        current.pop();
        for (uint32_t nbr : edges[cur]) {
            if (vertices[nbr] == -1) {
                vertices[nbr] = vertices[cur] + 1;
                current.push(nbr);
            }
        }
    }
}

uint32_t scan_degs(std::vector<uint32_t> &chad_degs, std::vector<uint32_t> &frontier, const std::vector<std::vector<uint32_t>> &edges, uint32_t blocks_count) {
    uint32_t sz = frontier.size();
    uint32_t block_size = sz / blocks_count + (sz % blocks_count ? 1 : 0);

    #pragma cilk grainsize 1
    cilk_for (uint32_t i = 0; i < blocks_count; ++i) {
        uint32_t left = i * block_size;
        uint32_t right = left + block_size;
        chad_degs[i] = 0;
        if (right > sz) {
            right = sz;
        }
        for (uint32_t j = left; j < right; ++j) {
            chad_degs[i] += edges[j].size();
        }
    }

    uint32_t last = chad_degs[blocks_count - 1];
    uint32_t t = chad_degs[0];
    chad_degs[0] = 0;
    for (uint32_t i = 1; i < blocks_count; ++i) {
        uint32_t next_t = chad_degs[i];
        chad_degs[i] = t + chad_degs[i - 1];
        t = next_t;
    }
    chad_degs[blocks_count] = chad_degs[blocks_count - 1] + last;

    return chad_degs[blocks_count];
}

void filter(const uint32_t *t_fr,
            std::vector<uint32_t> &chad_degs,
            std::vector<uint32_t> &act_degs,
            std::vector<uint32_t> &frontier,
            uint32_t blocks_count) {
    uint32_t last = act_degs[blocks_count - 1];
    uint32_t t = act_degs[0];
    act_degs[0] = 0;
    for (uint32_t i = 1; i < blocks_count; ++i) {
        uint32_t next_t = act_degs[i];
        act_degs[i] = t + act_degs[i - 1];
        t = next_t;
    }
    act_degs[blocks_count] = act_degs[blocks_count - 1] + last;

    frontier.resize(act_degs[blocks_count]);

    #pragma cilk grainsize 1
    cilk_for (uint32_t i = 0; i < blocks_count; ++i) {
        uint32_t left = chad_degs[i];
        uint32_t right = chad_degs[i + 1];
        for (uint32_t j = left; j < right; ++j) {
            if (t_fr[j] == -1) {
                break;
            }
            frontier[act_degs[i] + j - left] = t_fr[j];
        }
    }
}

void bfs_parallel(std::vector<std::atomic_int> &vertices, const std::vector<std::vector<uint32_t>> &edges, const uint32_t blocks_count = 1) {
    std::vector<uint32_t> frontier;
    std::vector<uint32_t> chad_degs(blocks_count + 1), act_degs(blocks_count + 1);
    uint32_t border = blocks_count << 4;
    frontier.push_back(0);
    vertices[0] = 0;
    uint32_t depth = 1;
    while (!frontier.empty()) {
        if (frontier.size() < border) {
            std::vector<uint32_t> t_frontier;
            t_frontier.reserve(frontier.size() * edges[frontier[0]].size());
            for (int i : frontier) {
                for (uint32_t nbr : edges[i]) {
                    if (vertices[nbr] == -1) {
                        vertices[nbr] = depth;
                        t_frontier.push_back(nbr);
                    }
                }
            }
            std::swap(t_frontier, frontier);
            continue;
        }
        uint32_t sz = scan_degs(chad_degs, frontier, edges, blocks_count);
        auto *t_fr = static_cast<uint32_t *>(::operator new(sizeof(uint32_t) * sz));
        uint32_t block_size = frontier.size() / blocks_count + (frontier.size() % blocks_count ? 1 : 0);

        #pragma cilk grainsize 1
        cilk_for (uint32_t i = 0; i < blocks_count; ++i) {
            uint32_t left = i * block_size;
            uint32_t right = left + block_size;
            if (right > frontier.size()) {
                right = frontier.size();
            }
            uint32_t idx = chad_degs[i];
            uint32_t act = 0;
            for (uint32_t j = left; j < right; ++j) {
                uint32_t fr = frontier[j];
                const std::vector<uint32_t> &fr_edges = edges[fr];
                for (uint32_t nbr : fr_edges) {
                    int dummy = -1;

                    if (vertices[nbr].compare_exchange_strong(dummy, depth)) {
                        t_fr[idx++] = nbr;
                        ++act;
                    }
                }
                if (idx != chad_degs[i + 1]) {
                    t_fr[idx] = -1;
                }
            }
            act_degs[i] = act;
        }

        filter(t_fr, chad_degs, act_degs, frontier, blocks_count);
        depth++;
        ::operator delete(t_fr);
    }
}

