/*
Fuente/Referencia: 
Algoritmo: Greedy
*/

#include <climits>
#include <vector>
#include "path_result.h"

PathResult solveGreedy(int n, int m, int v, int p,
                       const std::vector<std::vector<int>>& types,
                       const std::vector<std::vector<long long>>& values)
{

    long long score = 0;
    int i = 0;
    int j = 0;
    std::string path;
    while (i != n - 1 || j != m - 1) {
        int next_i = -1;
        int next_j = -1;
        long long best_gain = LLONG_MIN;

        const int di[2] = {1, 0};
        const int dj[2] = {0, 1};
        for (int d = 0; d < 2; ++d) {
            const int candidate_i = i + di[d];
            const int candidate_j = j + dj[d];
            if (candidate_i >= n || candidate_j >= m) {
                continue;
            }

            const int type = types[candidate_i][candidate_j];
            if ((type != 1 && type != -1) ||
                (type == -1 && v <= 1)) {
                continue;
            }

            const long long gain = (type == 1)
                ? values[candidate_i][candidate_j]
                : -static_cast<long long>(p);
            if (gain > best_gain) {
                best_gain = gain;
                next_i = candidate_i;
                next_j = candidate_j;
            }
        }

        if (next_i == -1) {
            return {false, score, path, v};
        }

        const int type = types[next_i][next_j];
        score += (type == 1) ? values[next_i][next_j] : -static_cast<long long>(p);
        if (type == -1) {
            --v;
        }
        path += (next_i == i + 1) ? 'D' : 'R';
        i = next_i;
        j = next_j;
    }

    return {true, score, path, v};
}