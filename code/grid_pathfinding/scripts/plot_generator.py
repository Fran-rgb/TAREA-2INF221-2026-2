"""
Genera gráficos de tiempo de ejecución, memoria y calidad de greedy desde resultados.txt.

Tiempo y memoria: las mediciones se agrupan por número de celdas y tipo de grilla;
cada punto representa el promedio entre dominios y muestras disponibles.

Calidad de greedy (usa las columnas found y score del programa principal):
  - El óptimo s(C*) se toma de Programación Dinámica, válida en todas las instancias.
  - Brecha: Δ = s(C*) - s(C_greedy), promediada sobre las instancias con solución
    en las que greedy sí llega a la meta.
  - Tasa de falla: proporción de instancias con solución (según DP) en las que
    greedy no alcanza la meta.
  - Las instancias sin solución (DP no encuentra camino) se excluyen de ambas métricas.
  - Se separa por dominio (D10 / D1000) porque la magnitud de Δ depende de p.
"""

import csv
import math
import sys
from collections import defaultdict
from pathlib import Path

import matplotlib.pyplot as plt


PROJECT_DIR = Path(__file__).resolve().parents[1]
MEASUREMENTS_FILE = PROJECT_DIR / "data" / "measurements" / "resultados.txt"
PLOTS_DIR = PROJECT_DIR / "data" / "plots"

COLUMNS = (
    "algorithm",
    "n",
    "m",
    "grid_type",
    "domain",
    "sample",
    "time_ms",
    "memory_kb",
)
EXTRA_COLUMNS = ("found", "score", "path_ok")
ALGORITHM_LABELS = {
    "Bruteforce": "Fuerza bruta",
    "Dp": "Programación dinámica",
    "Greedy": "Greedy",
}
GRID_TYPE_ORDER = ("libre", "murallas", "minas", "mixta")


def read_measurements(path):
    measurements = []
    malformed_rows = []

    with path.open("r", newline="", encoding="utf-8-sig") as file:
        for line_number, row in enumerate(csv.reader(file), start=1):
            if not row or not any(field.strip() for field in row):
                continue
            if (
                row[0].strip().lower() in ("algorithm", "algoritmo")
                and "time_ms" in (field.strip().lower() for field in row)
            ):
                continue

            if len(row) < len(COLUMNS):
                malformed_rows.append(
                    (line_number, f"se esperaban al menos {len(COLUMNS)} columnas y hay {len(row)}")
                )
                continue

            record = dict(zip(COLUMNS, (field.strip() for field in row[:len(COLUMNS)])))
            try:
                record["n"] = int(record["n"])
                record["m"] = int(record["m"])
                record["time_ms"] = float(record["time_ms"])
                record["memory_kb"] = int(record["memory_kb"])

                # Columnas de verificación (opcionales): found, score, path_ok
                if len(row) >= len(COLUMNS) + len(EXTRA_COLUMNS):
                    extra = [f.strip() for f in row[len(COLUMNS):len(COLUMNS) + len(EXTRA_COLUMNS)]]
                    record["found"] = int(extra[0]) != 0
                    record["score"] = int(extra[1])
                    record["path_ok"] = int(extra[2]) != 0
                else:
                    record["found"] = record["score"] = record["path_ok"] = None
            except ValueError as error:
                malformed_rows.append((line_number, str(error)))
                continue

            if (
                record["n"] <= 0
                or record["m"] <= 0
                or not math.isfinite(record["time_ms"])
                or record["time_ms"] < 0
                or record["memory_kb"] < 0
            ):
                malformed_rows.append((line_number, "contiene valores numéricos inválidos"))
                continue

            measurements.append(record)

    if not measurements:
        raise ValueError(f"No hay filas de medición válidas en {path}")

    return measurements, malformed_rows


def ordered_grid_types(grid_types):
    grid_types = set(grid_types)
    ordered = [name for name in GRID_TYPE_ORDER if name in grid_types]
    ordered.extend(sorted(grid_types - set(ordered)))
    return ordered


def domain_sort_key(domain):
    digits = "".join(ch for ch in domain if ch.isdigit())
    return (int(digits) if digits else math.inf, domain)


def aggregate_by_grid_size(measurements, metric):
    grouped = defaultdict(list)
    for row in measurements:
        cell_count = row["n"] * row["m"]
        grouped[(row["grid_type"], row["algorithm"], cell_count)].append(row[metric])

    return {
        key: sum(values) / len(values)
        for key, values in grouped.items()
    }


def plot_metric(measurements, metric, filename, title, y_label, use_log_y=False):
    averages = aggregate_by_grid_size(measurements, metric)
    grid_types = ordered_grid_types(grid_type for grid_type, _, _ in averages)

    columns = 2
    rows = (len(grid_types) + columns - 1) // columns
    figure, axes = plt.subplots(
        rows,
        columns,
        figsize=(14, 5 * rows),
        squeeze=False,
    )
    algorithms = list(ALGORITHM_LABELS)
    algorithms.extend(
        sorted({algorithm for _, algorithm, _ in averages} - set(algorithms))
    )

    for axis, grid_type in zip(axes.flat, grid_types):
        plotted_values = []
        for algorithm in algorithms:
            points = sorted(
                (cell_count, value)
                for (kind, name, cell_count), value in averages.items()
                if kind == grid_type and name == algorithm
            )
            if not points:
                continue

            x_values, y_values = zip(*points)
            plotted_values.extend(y_values)
            axis.plot(
                x_values,
                y_values,
                marker="o",
                linewidth=1.8,
                label=ALGORITHM_LABELS.get(algorithm, algorithm),
            )

        axis.set_title(grid_type.capitalize())
        axis.set_xlabel("Tamaño de la grilla (N × M celdas)")
        axis.set_ylabel(y_label)
        axis.grid(True, which="both", linestyle="--", alpha=0.4)
        axis.set_xscale("log")
        if use_log_y and plotted_values and all(value > 0 for value in plotted_values):
            axis.set_yscale("log")
        elif not use_log_y:
            axis.set_ylim(bottom=0)
        axis.legend()

    for axis in list(axes.flat)[len(grid_types):]:
        axis.set_visible(False)

    figure.suptitle(title, fontsize=15)
    figure.tight_layout()
    output_path = PLOTS_DIR / filename
    figure.savefig(output_path, dpi=160, bbox_inches="tight")
    plt.close(figure)
    print(f"Gráfico guardado en: {output_path}")


# ---------------------------------------------------------------------------
# Calidad de greedy: brecha Δ y tasa de falla
# ---------------------------------------------------------------------------

def group_instances(measurements):
    """Agrupa las filas por instancia: (n, m, tipo, dominio, muestra) -> {algoritmo: fila}."""
    instances = defaultdict(dict)
    for row in measurements:
        if row["found"] is None:
            continue
        key = (row["n"], row["m"], row["grid_type"], row["domain"], row["sample"])
        instances[key][row["algorithm"]] = row
    return instances


def check_consistency(instances):
    """Avisos de coherencia entre algoritmos. Devuelve la cantidad de problemas."""
    problems = 0
    for key, algs in sorted(instances.items()):
        label = "_".join(str(part) for part in key)
        for name, row in algs.items():
            if row["path_ok"] is False:
                print(f"Aviso: camino inválido de {name} en {label}.", file=sys.stderr)
                problems += 1

        dp, bf, greedy = algs.get("Dp"), algs.get("Bruteforce"), algs.get("Greedy")
        if dp and bf and (dp["found"] != bf["found"] or (dp["found"] and dp["score"] != bf["score"])):
            print(f"Aviso: fuerza bruta y DP discrepan en {label}.", file=sys.stderr)
            problems += 1
        if dp and greedy and greedy["found"] and (not dp["found"] or greedy["score"] > dp["score"]):
            print(f"Aviso: greedy supera al óptimo (DP) en {label}.", file=sys.stderr)
            problems += 1
    return problems


def compute_quality(instances):
    """
    Devuelve (brecha_promedio, tasa_falla, conteos), indexados por
    (tipo de grilla, dominio, nº de celdas).
    """
    gaps = defaultdict(list)
    fails = defaultdict(list)
    skipped = 0

    for (n, m, grid_type, domain, _), algs in instances.items():
        dp, greedy = algs.get("Dp"), algs.get("Greedy")
        if dp is None or greedy is None:
            continue
        if not dp["found"]:
            skipped += 1          # instancia sin solución: no entra en las métricas
            continue

        key = (grid_type, domain, n * m)
        if greedy["found"]:
            gaps[key].append(dp["score"] - greedy["score"])
            fails[key].append(0)
        else:
            fails[key].append(1)

    mean_gap = {key: sum(v) / len(v) for key, v in gaps.items()}
    failure_rate = {key: 100.0 * sum(v) / len(v) for key, v in fails.items()}
    return mean_gap, failure_rate, skipped


def plot_quality(values, filename, title, y_label, symlog=False, percent=False):
    grid_types = ordered_grid_types(grid_type for grid_type, _, _ in values)
    domains = sorted({domain for _, domain, _ in values}, key=domain_sort_key)

    columns = 2
    rows = (len(grid_types) + columns - 1) // columns
    figure, axes = plt.subplots(rows, columns, figsize=(14, 5 * rows), squeeze=False)

    for axis, grid_type in zip(axes.flat, grid_types):
        for domain in domains:
            points = sorted(
                (cell_count, value)
                for (kind, dom, cell_count), value in values.items()
                if kind == grid_type and dom == domain
            )
            if not points:
                continue
            x_values, y_values = zip(*points)
            axis.plot(x_values, y_values, marker="o", linewidth=1.8, label=domain)

        axis.set_title(grid_type.capitalize())
        axis.set_xlabel("Tamaño de la grilla (N × M celdas)")
        axis.set_ylabel(y_label)
        axis.grid(True, which="both", linestyle="--", alpha=0.4)
        axis.set_xscale("log")
        if percent:
            axis.set_ylim(-3, 103)
        elif symlog:
            axis.set_yscale("symlog", linthresh=1)
            axis.set_ylim(bottom=0)
        else:
            axis.set_ylim(bottom=0)
        if axis.get_legend_handles_labels()[0]:
            axis.legend(title="Dominio")

    for axis in list(axes.flat)[len(grid_types):]:
        axis.set_visible(False)

    figure.suptitle(title, fontsize=15)
    figure.tight_layout()
    output_path = PLOTS_DIR / filename
    figure.savefig(output_path, dpi=160, bbox_inches="tight")
    plt.close(figure)
    print(f"Gráfico guardado en: {output_path}")


def main():
    if not MEASUREMENTS_FILE.is_file():
        raise SystemExit(f"No se encontró el archivo de mediciones: {MEASUREMENTS_FILE}")

    measurements, malformed_rows = read_measurements(MEASUREMENTS_FILE)
    PLOTS_DIR.mkdir(parents=True, exist_ok=True)

    print(f"Filas de medición cargadas: {len(measurements)}")
    for line_number, reason in malformed_rows:
        print(
            f"Aviso: se omite la línea {line_number} de resultados.txt ({reason}).",
            file=sys.stderr,
        )

    zero_memory_rows = sum(row["memory_kb"] == 0 for row in measurements)
    if zero_memory_rows:
        print(
            "Aviso: hay mediciones de memoria iguales a 0 KB "
            f"({zero_memory_rows}/{len(measurements)}). "
            "El gráfico reflejará esos valores tal como aparecen en resultados.txt.",
            file=sys.stderr,
        )

    plot_metric(
        measurements,
        "time_ms",
        "tiempo_ejecucion.png",
        "Tiempo de ejecución vs. tamaño",
        "Tiempo promedio (ms)",
        use_log_y=True,
    )
    plot_metric(
        measurements,
        "memory_kb",
        "uso_memoria.png",
        "Uso de memoria vs. tamaño",
        "Memoria promedio (KB)",
    )

    # --- Calidad de greedy ---
    instances = group_instances(measurements)
    if not instances:
        print(
            "Aviso: resultados.txt no tiene las columnas found/score; "
            "se omiten los gráficos de brecha y tasa de falla.",
            file=sys.stderr,
        )
        return

    problems = check_consistency(instances)
    mean_gap, failure_rate, skipped = compute_quality(instances)

    solvable = sum(1 for algs in instances.values() if algs.get("Dp") and algs["Dp"]["found"])
    print(f"Instancias con solución (según DP): {solvable}; sin solución (excluidas): {skipped}")
    if problems:
        print(f"Aviso: {problems} problema(s) de coherencia detectados.", file=sys.stderr)

    if mean_gap:
        plot_quality(
            mean_gap,
            "brecha_greedy.png",
            "Brecha Δ = s(C*) − s(C_greedy) (solo donde greedy llega a la meta)",
            "Brecha promedio Δ (escala symlog)",
            symlog=True,
        )
    else:
        print("Aviso: no hay instancias donde greedy llegue a la meta; sin gráfico de brecha.", file=sys.stderr)

    if failure_rate:
        plot_quality(
            failure_rate,
            "tasa_falla_greedy.png",
            "Tasa de falla de greedy (instancias con solución)",
            "Instancias donde greedy no llega a la meta (%)",
            percent=True,
        )


if __name__ == "__main__":
    main()
