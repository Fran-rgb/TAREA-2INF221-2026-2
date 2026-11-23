/*
 * Referencias y Fuentes:
 * - Medición de tiempo: <chrono> de C++11.
 * - Medición de memoria: sys/resource.h (getrusage - POSIX/Linux).
 * - Aislamiento de mediciones: fork/pipe/waitpid (POSIX/Linux).
 * - Algoritmos incluidos desde la carpeta 'algorithms/'.
 *
 * Cada medición (algoritmo x instancia) corre en un proceso hijo. Así el pico
 * de memoria (ru_maxrss) parte limpio en cada medición y no se arrastra desde
 * algoritmos o instancias anteriores. El padre solo coordina, escribe los
 * resultados y verifica que Fuerza Bruta y DP coincidan.
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <csignal>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

// algoritmos
#include "algorithms/path_result.h"
#include "algorithms/brute_force.cpp"
#include "algorithms/dynamic_programming.cpp"
#include "algorithms/greedy.cpp"

// Firmas de los algoritmos definidos en algorithms/
PathResult solveBruteForce(int n, int m, int v, int p,
                          const std::vector<std::vector<int>>& types,
                          const std::vector<std::vector<long long>>& values);

PathResult solveDP(int n, int m, int v, int p,
                   const std::vector<std::vector<int>>& types,
                   const std::vector<std::vector<long long>>& values);

PathResult solveGreedy(int n, int m, int v, int p,
                       const std::vector<std::vector<int>>& types,
                       const std::vector<std::vector<long long>>& values);

using Types    = std::vector<std::vector<int>>;
using Values   = std::vector<std::vector<long long>>;
using SolverFn = PathResult (*)(int, int, int, int, const Types&, const Values&);

static const int V = 3;
static const int MAX_BF_SUM = 32;          // Fuerza bruta solo si N + M <= 32
static const unsigned BF_TIMEOUT_S = 600;  // límite por instancia para fuerza bruta

// Obtener el uso máximo de memoria en KB (Peak RSS)
long getPeakMemoryUsageKB() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss;
}

// Función auxiliar para escribir la salida en un archivo txt
void write_To_File(const std::string& filepath, const PathResult& result) {
    std::ofstream archivo(filepath);
    archivo << result.found << "\n"
            << result.score << "\n"
            << result.path << "\n"
            << result.remaining_lives << "\n";
}

// Función auxiliar para definir una grilla a partir de un archivo txt.
// Retorna false si el archivo no existe o está incompleto.
bool read_File(const std::string& filepath, int n, int m,
               Types& types, Values& values) {
    std::ifstream archivo(filepath);
    if (!archivo.is_open()) {
        return false;
    }

    std::string basura;
    for (int i = 0; i < 4; ++i) {
        if (!(archivo >> basura)) return false;
    }

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            if (!(archivo >> types[i][j])) return false;
        }
    }

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            if (!(archivo >> values[i][j])) return false;
        }
    }
    return true;
}

// Reproduce el camino y recalcula puntaje y vidas. Detecta caminos mal
// reconstruidos aunque el puntaje coincida.
bool validatePath(const PathResult& r, int n, int m, int v, int p,
                  const Types& types, const Values& values) {
    if (!r.found) return true;  // nada que validar
    int i = 0, j = 0, lives = v;
    long long score = 0;
    for (char c : r.path) {
        if (c == 'D') i++;
        else if (c == 'R') j++;
        else return false;
        if (i >= n || j >= m) return false;
        int t = types[i][j];
        if (t == 1) score += values[i][j];
        else if (t == -1) { score -= p; lives--; }
        else return false;      // pisó un muro
        if (lives <= 0) return false;
    }
    return i == n - 1 && j == m - 1 && score == r.score && lives == r.remaining_lives;
}

// Resultado de una medición (POD, se envía del hijo al padre por un pipe)
struct Measurement {
    bool ok;            // la medición terminó correctamente
    double time_ms;
    long mem_kb;        // pico de memoria adicional respecto a la grilla ya cargada
    bool found;
    long long score;
    bool path_ok;       // el camino devuelto se pudo reproducir
};

// Ejecuta UN algoritmo sobre UNA instancia en un proceso hijo y devuelve la medición.
Measurement measureIsolated(SolverFn solver, bool is_bruteforce,
                            int n, int m, int p,
                            const std::string& in_path,
                            const std::string& out_path) {
    Measurement res{};
    res.ok = false;

    int fd[2];
    if (pipe(fd) != 0) return res;

    pid_t pid = fork();
    if (pid < 0) {
        close(fd[0]);
        close(fd[1]);
        return res;
    }

    if (pid == 0) {
        // ----- HIJO -----
        close(fd[0]);
        if (is_bruteforce) alarm(BF_TIMEOUT_S);   // SIGALRM mata al hijo si se pasa

        Measurement r{};
        r.ok = false;
        {
            Types types(n, std::vector<int>(m, 0));
            Values values(n, std::vector<long long>(m, 0));

            if (read_File(in_path, n, m, types, values)) {
                // La grilla ya está cargada: se mide solo el costo del algoritmo
                long mem_before = getPeakMemoryUsageKB();
                auto start = std::chrono::high_resolution_clock::now();

                PathResult sol = solver(n, m, V, p, types, values);

                auto end = std::chrono::high_resolution_clock::now();
                long mem_after = getPeakMemoryUsageKB();

                r.ok = true;
                r.time_ms = std::chrono::duration<double, std::milli>(end - start).count();
                r.mem_kb = mem_after - mem_before;
                r.found = sol.found;
                r.score = sol.score;
                r.path_ok = validatePath(sol, n, m, V, p, types, values);

                write_To_File(out_path, sol);
            } else {
                std::cerr << "Advertencia: Archivo no encontrado o incompleto: " << in_path << std::endl;
            }
        }
        ssize_t w = write(fd[1], &r, sizeof(r));
        (void)w;
        close(fd[1]);
        _exit(r.ok ? 0 : 1);
    }

    // ----- PADRE -----
    close(fd[1]);
    Measurement r{};
    ssize_t got = read(fd[0], &r, sizeof(r));
    close(fd[0]);

    int status = 0;
    waitpid(pid, &status, 0);

    if (WIFSIGNALED(status) && WTERMSIG(status) == SIGALRM) {
        std::cerr << "Timeout (" << BF_TIMEOUT_S << " s): " << in_path << std::endl;
        return res;
    }
    if (got == (ssize_t)sizeof(r) && WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        return r;
    }
    return res;
}

// Escribe una línea de resultados
void writeLine(std::ofstream& output, const std::string& alg, int n, int m,
               const std::string& t, const std::string& d, const std::string& s,
               const Measurement& r) {
    output << alg << "," << n << "," << m << "," << t << "," << d << "," << s << ","
           << r.time_ms << "," << r.mem_kb << ","
           << r.found << "," << r.score << "," << r.path_ok << "\n";
}

int main() {
    // --- Parámetros de los casos de prueba ---
    std::vector<int> Ns = {1 << 2, 1 << 3, 1 << 4, 1 << 6, 1 << 8, 1 << 10};
    std::vector<int> FACTORES_M = {1, 2};
    std::vector<std::string> Ts = {"libre", "murallas", "minas", "mixta"};
    std::vector<std::string> Ds = {"D10", "D1000"};
    std::vector<std::string> Ss = {"a", "b", "c"};

    std::ofstream output("data/measurements/resultados.txt");
    if (!output.is_open()) {
        std::cerr << "Error: No se pudo crear el archivo de mediciones." << std::endl;
        return 1;
    }
    output << "algoritmo,n,m,t,d,s,time_ms,mem_kb,found,score,path_ok\n";

    std::cout << "Iniciando mediciones de algoritmos de busqueda del optimo..." << std::endl;

    int errores = 0;

    for (int n : Ns) {
        std::string n_str = std::to_string(n);
        for (int m_factor : FACTORES_M) {
            int m = n * m_factor;
            std::string m_str = std::to_string(m);

            for (const auto& t : Ts) {
                for (const auto& d : Ds) {
                    for (const auto& s : Ss) {

                        std::string name = n_str + "_" + m_str + "_" + t + "_" + d + "_" + s;
                        std::string in_path = "data/grid_input/" + name + ".txt";
                        std::string out_base = "data/grid_output/" + name;
                        int p = std::stoi(d.substr(1));

                        // --- 1. Fuerza Bruta (solo si N + M <= 32) ---
                        Measurement bf{};
                        bool bf_ran = (n + m <= MAX_BF_SUM);
                        if (bf_ran) {
                            bf = measureIsolated(solveBruteForce, true, n, m, p,
                                                 in_path, out_base + "_bruteforce_out.txt");
                            if (bf.ok) writeLine(output, "Bruteforce", n, m, t, d, s, bf);
                        }

                        // --- 2. Programación Dinámica ---
                        Measurement dp = measureIsolated(solveDP, false, n, m, p,
                                                         in_path, out_base + "_dp_out.txt");
                        if (dp.ok) writeLine(output, "Dp", n, m, t, d, s, dp);

                        // --- 3. Greedy ---
                        Measurement gr = measureIsolated(solveGreedy, false, n, m, p,
                                                         in_path, out_base + "_greedy_out.txt");
                        if (gr.ok) writeLine(output, "Greedy", n, m, t, d, s, gr);

                        // --- Verificación ---
                        if (dp.ok && !dp.path_ok) {
                            std::cerr << "[ERROR] camino DP invalido: " << name << std::endl;
                            errores++;
                        }
                        if (gr.ok && !gr.path_ok) {
                            std::cerr << "[ERROR] camino greedy invalido: " << name << std::endl;
                            errores++;
                        }
                        if (bf_ran && bf.ok && !bf.path_ok) {
                            std::cerr << "[ERROR] camino fuerza bruta invalido: " << name << std::endl;
                            errores++;
                        }
                        if (bf_ran && bf.ok && dp.ok) {
                            if (bf.found != dp.found || (bf.found && bf.score != dp.score)) {
                                std::cerr << "[ERROR] Fuerza bruta y DP discrepan en " << name
                                          << ": bf=(" << bf.found << "," << bf.score << ")"
                                          << " dp=(" << dp.found << "," << dp.score << ")" << std::endl;
                                errores++;
                            }
                        }
                        if (dp.ok && gr.ok && gr.found) {
                            if (!dp.found || gr.score > dp.score) {
                                std::cerr << "[ERROR] greedy supera al optimo (DP) en " << name << std::endl;
                                errores++;
                            }
                        }
                    }
                }
            }
        }
        output.flush();
        std::cout << "Mediciones finalizadas para n = " << n << std::endl;
    }

    output.close();
    std::cout << "Proceso finalizado. Errores de verificacion: " << errores << std::endl;
    return errores == 0 ? 0 : 1;
}