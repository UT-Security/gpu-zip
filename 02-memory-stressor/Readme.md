# Memory bus contention

The `write-only` workload in our graphical PoC (See `../poc/gpu-create` for details about the PoC) features pattern-dependent *DRAM traffic per frame* but not pattern-dependent *Rendering time per frame* because it cannot saturate the iGPU memory subsystem.
This folder tests that adding extra memory contention alongside `write-only` to saturate the IMC can manifest rendering time differences (Figure 4 in the paper).

## Preliminaries

- Required tool: stress-ng.

## Tested machine

Script `./stressor.sh` is tested on an Intel i7-8700 running Ubuntu 22.04.
Running the scripts on other CPUs will likely require modification to the code (e.g., more memory stressors).

## How to build

To build the graphical PoC: follow the instruction in `../poc/gpu-create`.

## How to reproduce Figure 4 in the paper

```bash
./stressor.sh
```

Above script runs the graphic PoC (`write-only`) with patterns `Black` and `Random`, workload complexity `iterations` = `10`, and `SCR_WIDTH` = `3000`.
We add extra memory contention by increasing the number of `memcpy` workers from 1 to 12.
For each experiment, we collect 10000 *Rendering time per frame* data points, which can be found in `./time`.

**Note**: On machines with many cores, we recommend increasing the maximum number of `memcpy` workers to fully saturate the IMC.

To plot the collected data:

```bash
python stressor.py ./time/
```

The plot similar to Figure 4 in the paper can be found in the plot subfolder (`./plot/memory-stressor.pdf`).
