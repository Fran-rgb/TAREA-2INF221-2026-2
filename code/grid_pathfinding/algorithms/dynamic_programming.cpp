/*
Fuente/Referencia: 
Algoritmo: Programación Dinámica
*/

#include <vector>
#include <algorithm>
#include <climits>
#include "path_result.h"

PathResult solveDP(int n, int m, int v, int p,
                   const std::vector<std::vector<int>>& types,
                   const std::vector<std::vector<long long>>& values)
{
    const long long NEG = LLONG_MIN / 4;       // "inalcanzable"

    // dp[i][j][k]: mejor puntaje en (i,j) con k celdas de daño pisadas
    std::vector<std::vector<std::vector<long long>>> dp(
        n, std::vector<std::vector<long long>>(m, std::vector<long long>(v, NEG)));
    // par[i][j][k]: de dónde vinimos ('D' = desde arriba, 'R' = desde la izquierda)
    std::vector<std::vector<std::vector<char>>> par(
        n, std::vector<std::vector<char>>(m, std::vector<char>(v, '?')));

    dp[0][0][0] = 0;

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            if (i == 0 && j == 0) continue;

            int t = types[i][j];
            if (t != 1 && t != -1) continue;   // muro

            int cost = (t == -1) ? 1 : 0;      // vida que cuesta esta celda
            long long gain = (t == 1) ? values[i][j] : -static_cast<long long>(p);

            for (int k = cost; k < v; k++) {
                int pk = k - cost;             // daño acumulado antes de pisar (i,j)
                long long best = NEG;
                char from = '?';

                if (i > 0 && dp[i-1][j][pk] > best) {
                    best = dp[i-1][j][pk];
                    from = 'D';
                }
                if (j > 0 && dp[i][j-1][pk] > best) {
                    best = dp[i][j-1][pk];
                    from = 'R';
                }
                if (from == '?') continue;     // no se puede llegar así

                dp[i][j][k] = best + gain;
                par[i][j][k] = from;
            }
        }
    }

    // Elegir el mejor k en el destino
    int bk = -1;
    for (int k = 0; k < v; k++) {
        if (dp[n-1][m-1][k] > NEG && (bk == -1 || dp[n-1][m-1][k] > dp[n-1][m-1][bk])) {
            bk = k;
        }
    }
    if (bk == -1) return {false, 0, "", v};

    // Reconstruir el camino hacia atrás
    std::string path;
    int ci = n - 1, cj = m - 1, k = bk;
    while (ci != 0 || cj != 0) {
        char ch = par[ci][cj][k];
        path.push_back(ch);
        if (types[ci][cj] == -1) k--;
        if (ch == 'D') ci--; else cj--;
    }
    std::reverse(path.begin(), path.end());

    return {true, dp[n-1][m-1][bk], path, v - bk};
}