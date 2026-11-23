Autor: Franco Antonio Guerrero Gómez

Rol: 202473628-0

# TAREA 2 INF221 2026-2

## Estructura del Repositorio

Este directorio contiene las implementaciones en C++ de los algoritmos de busqueda de camino desde el punto `(0,0)` hasta `(n-1,m-1)` de distintas grillas usando solo movimientos `D` (abajo) y `R` (derecha).

Los scripts para la automatización de ejecuciones y mediciones, los programas en Python para generación de instancias y gráficos, así como las fuentes bibliográficas utilizadas (en este caso ninguna).

A continuación, se describe la estructura del repositorio:

```bash
├── code
├── report
└── README.md
```

### `code`

Contiene los archivos que conforman la entrega de la tarea. En ella, se implementaron los algoritmos solicitados, la generacion de instancias para usarlos, los algoritmos que los utilizan y miden los tiempos de ejecucion y memoria respectivamente usados. También se incluye la generacion de graficos comparativos a partir de la recolección de datos.

### `report`

En esta carpeta se encuentra el informe realizado sobre la Tarea en formato latex, donde el documento "report.pdf" es generado a partir de la jerarquia de archivos .tex presente en la carpeta.

## Instrucciones para compilar

### Requisitos del sistema

Sistema Operativo: Linux / WSL.
Compilador C++: g++ con soporte para C++17.
Python: Python 3.8+ con los paquetes de:
pip install numpy pandas matplotlib seaborn

### Pasos a seguir

Ejecutar los siguientes pasos desde la raíz del repositorio:

1. Generar las instancias de prueba:

   ```bash
   python3 code/grid_pathfinding/scripts/grid_generator.py
   ```

2. Compilar y ejecutar los algoritmos para generar las mediciones:

   ```bash
   make -C code/grid_pathfinding run
   ```

   Para recompilar desde cero, ejecutar `make -C code/grid_pathfinding clean`
   antes de `make -C code/grid_pathfinding run`.

3. Generar los gráficos a partir de las mediciones:

   ```bash
   python3 code/grid_pathfinding/scripts/plot_generator.py
   ```

4. Compilar el informe desde la carpeta `report`. Ejecutar `pdflatex` dos veces
   para actualizar las referencias cruzadas:

   ```bash
   cd report
   pdflatex report.tex
   pdflatex report.tex
   ```