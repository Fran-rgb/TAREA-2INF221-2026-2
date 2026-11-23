Autor: Franco Antonio Guerrero Gómez

Rol: 202473628-0

# Documentación

Este directorio contiene la implementación en C++ de los algoritmos de la Tarea 2, el programa principal que genera las mediciones y valida los caminos, y los scripts de Python para generar instancias y figuras comparativas.

## Entrega

La entrega se realiza vía **aula.usm.cl** en formato `.zip`.

## Grid pathfinding

Problema implementado: recorrer una grilla desde `(0,0)` hasta `(n-1,m-1)`
usando solo movimientos `D` (abajo) y `R` (derecha). Los movimientos sobre
celdas normales suman su valor; pisar una mina resta la penalidad `P` y consume
una vida; las murallas no pueden recorrerse. Se comienza con `V = 3` vidas.

Algoritmos implementados:

- Fuerza Bruta con búsqueda exhaustiva y backtracking.
- Programación Dinámica con estado `(fila, columna, daños acumulados)`.
- Greedy, que elige entre `D` y `R` la celda inmediata con mayor ganancia.

### Programa Principal 

Ubicado en `grid_pathfinding/` el archivo `grid_pathfinding.cpp` se encarga de leer las instancias desde `grid_pathfinding/data/grid_input/` que estan en formato `.txt` para cargarlas como instancias que sirven de entrada para los algoritmos, los ejecuta en procesos aislados, cabe mencionar que se ejecuta Fuerza Bruta solo cuando `N + M <= 32` y se aplica un límite de 600 segundos a cada medición de Fuerza Bruta;

Por cada ejecución de un algoritmo, se escribe cada camino encontrado en `data/grid_output/`, se mide tiempo con `std::chrono` en milisegundos, se mide el incremento de memoria pico desde el inicio de la medición con `getrusage(RUSAGE_SELF, ...)`, en KB, se verifica que los caminos sean reproducibles y que DP coincida con Fuerza Bruta y por ultimo advierte si Greedy retorna un puntaje superior al óptimo encontrado por DP.

Los resultados de las mediciones se escriben en `data/measurements/resultados.txt` con columnas:

```text
algoritmo,n,m,t,d,s,time_ms,mem_kb,found,score,path_ok
```

`found` indica si se alcanzó la meta, `score` es el puntaje obtenido y
`path_ok` indica si el camino pudo reproducirse correctamente.

#### Instalación y compilación

El programa requiere un entorno POSIX compatible con las llamadas `fork`, `pipe`, `waitpid`, `getrusage` y `alarm`, por lo que fue diseñado para ejecutarse en Linux.

Desde `code/grid_pathfinding`, se puede usar el Makefile:

```bash
make run
```

Para recompilar y ejecutar desde cero:

```bash
make clean
make run
```

### Scripts

#### Generación de casos de prueba

El script necesita `numpy` como dependencia de Python.

Desde `code/grid_pathfinding`, ejecutar:

```bash
python3 scripts/grid_generator.py
```

Las instancias tienen las siguientes características:

- Filas `N`: `{4, 8, 16, 64, 256, 1024}`.
- Columnas `M`: `{N, 2N}`.
- Tipos: `libre`, `murallas`, `minas` y `mixta`.
- Dominios: `D10` y `D1000`.
- Tres muestras por combinación: `a`, `b` y `c`.
- Un máximo de `floor(sqrt(N*M))` murallas o minas según el tipo.

Cada archivo usa el formato:

```text
N M V P
M valores de tipo por fila
M valores numéricos por fila
```

Los identificadores son `1` para celda normal, `0` para muralla y `-1` para mina.
Las grillas reservan al menos un camino válido aleatorio que evita murallas y minas.

Los nombres tienen la forma:

```text
{N}_{M}_{tipo}_{D}_{muestra}.txt
```

#### Generación de gráficos

El script necesita `numpy` y `matplotlib` como dependencias de Python.

Desde `code/grid_pathfinding`, ejecutar:

```bash
python3 scripts/plot_generator.py
```

El script lee `data/measurements/resultados.txt` y guarda las siguientes figuras
en `data/plots/`:

- `tiempo_ejecucion.png`: tiempo promedio según tamaño y tipo de grilla.
- `uso_memoria.png`: memoria promedio según tamaño y tipo de grilla.
- `brecha_greedy.png`: diferencia `óptimo DP - puntaje greedy`, separada por
  dominio y tipo de grilla.
- `tasa_falla_greedy.png`: porcentaje de instancias con solución en las que
  Greedy no alcanza la meta.

Los promedios de tiempo y memoria incluyen las tres muestras y ambos dominios.
Las métricas de calidad excluyen instancias sin solución según DP.