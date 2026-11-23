/*
Fuente/Referencia: 
Algoritmo: Fuerza Bruta
*/

#include <vector>
#include "path_result.h"

void dfs(int i, int j, int v, long long score, std::string& path,
         int n, int m,
         const std::vector<std::vector<int>>& types,
         const std::vector<std::vector<long long>>& values,
         int p, PathResult& best)
{
    // Caso base: llegamos al destino con vida 
    if (i == n - 1 && j == m - 1) {
        if (!best.found || score > best.score) {
            best = {true, score, path, v};
        }
        return;
    }

    const int di[2] = {1, 0};
    const char dc[2] = {'D', 'R'};
    const int dj[2] = {0, 1};

    for (int d = 0; d < 2; d++) {
        int ni = i + di[d];
        int nj = j + dj[d];

        if (ni >= n || nj >= m) continue;      // fuera de la grilla

        int t = types[ni][nj];
        long long nscore = score;
        int nv = v;

        if (t == 1) {
            nscore += values[ni][nj];
        } else if (t == -1) {
            nscore -= p;
            nv--;
            if (nv <= 0) continue;             // poda: sin vida, camino inválido
        } else {
            continue;                          // muro
        }

        path.push_back(dc[d]);                 // elegir
        dfs(ni, nj, nv, nscore, path, n, m, types, values, p, best);
        path.pop_back();                       // deshacer (backtracking)
    }
}

PathResult solveBruteForce(int n, int m, int v, int p,
                           const std::vector<std::vector<int>>& types,
                           const std::vector<std::vector<long long>>& values)
{
    PathResult best = {false, 0, "", v};

    std::string path;
    dfs(0, 0, v, 0, path, n, m, types, values, p, best);
    return best;
}