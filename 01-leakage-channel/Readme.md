# iGPU Graphical Data Compression Exists

This folder tests the existance of iGPU graphical data compression by measuring necessary hardware performance events.
The graphic PoC we test locates at `../poc/gpu-create` (details about the PoC are provided there).
Our PoC supports three types of workload: `read-only`, `write-only`, `write-read`.
Our PoC supports four types of graphical patterns: `Black`, `Random`, `Gradient`, and `Skew`.

We run our PoC and measure the three metrics (see paper S3.1):

1. *Rendering time per frame*: the amount of time it takes for a frame to refresh.
2. *DRAM traffic per frame*: the amount of data transmitted across the DRAM data bus during the rendering of one frame.
3. *DRAM bandwidth*: the amount of data transmitted across the DRAM data bus between consecutive performance counter readings, divided by the time between those consecutive readings.

We also measure two other metrics that are not in presented in the paper, but for sanity check to ensure that any pattern-dependent observation of *Rendering time per frame* is not due to iGPU frequency variations and memory utilization variations:

4. *GPU frequency*: the average iGPU frequency while running the PoC.
5. *Peak RSS*: peak resident set size, the maximum amount of physical memory (RAM) that our PoC has used during its execution.

Script `./scripts/exp1/exp1.sh` reproduces Table 2 in the paper.
Script `./scripts/exp2/exp2.sh` inspects the iGPU graphical data compression on an Intel i7-8700 deeply, and reproduces Figure 2 and 3 in the paper.

## Preliminaries

- Compiler: gcc.
- Required tool: perf.

## Tested machine

Script `./scripts/exp1/exp1.sh` is tested on an Intel i7-8700 (Ubuntu 22.04), i5-1135G7 (Debian 11.6), i7-12700K (Ubuntu 22.04), and AMD Ryzen 7 4800U (Ubuntu 20.04).
Script `./scripts/exp2/exp2.sh` is tested on an Intel i7-8700 running Ubuntu 22.04.
Running the scripts on other CPUs will likely require modification to the code.

## How to build

On an Intel i7-8700, or i5-1135G7

```bash
make
```

On an Intel i7-12700K

```bash
make CFLAGS+=-DALDER
```

On an AMD Ryzen 7 4800U

```bash
make CFLAGS+=-DAMD
```

To build the graphical PoC: follow the instruction in `../poc/gpu-create`.

## How to reproduce Table 2 in the paper

```bash
cd scripts/exp1
./exp1.sh
```

Above script runs the graphic PoC (`write-read`) with four patterns (`Black`, `Random`, `Gradient`, and `Skew`) as inputs.
For each pattern, the PoC runs for 400 seconds.

The hardware measurement data can be found at folders `./data/exp1-${date}` (GPU, IMC events and memory utilization) and *Rendering time per frame* can be found in `./data/time-exp1-${date}`.

To parse the data:

```bash
cd scripts/exp1
python exp1.py ../../data/exp1-${date}/ ../../data/time-exp1-${date}/
```

The output contains average and std of *Rendering time per frame* and *DRAM traffic per frame* for reproducing Table 2, and *GPU frequency* and *Peak RSS* for sanity check.

## How to reproduce Figure 2 and 3 in the paper

```bash
cd scripts/exp2
./exp2.sh
```

Above script runs the graphic PoC (`read-only`) and graphic PoC (`write-only`) with four patterns (`Black`, `Random`, `Gradient` and `Skew`) as inputs.
It increases the workload complexity from 1 to 44 with a step of 4.
For each combination of pattern, workload complexity, and workload type, the PoC runs for 400 seconds.

The hardware measurement data can be found in folder `./data/exp2-${date}` (GPU, IMC events and memory utilization) and *Rendering time per frame* can be found in folder `./data/time-exp2-${date}`.
On Intel i7-8700, `Black` and `Gradient` are compressible, and `Random` and `Skew` are non-compressible.
When plotting the data, we group `Black` and `Gradient` as compressible, and `Random` and `Skew` as non-compressible.

To plot Figure 2 and 3 out of the data:

```bash
cd scripts/exp2
python exp2.py ../../data/exp2-${date}/ ../../data/time-exp2-${date}/
```

The plots similar to the Figure 2 and 3 in the paper can be found in the plot subfolder.

1. `./plot/GPUread.pdf` plots read and write data of *DRAM traffic per frame* vs workload complexity for compressible and non-compressible textures of the `read-only` worload (Figure 2a).
2. `./plot/GPUwrite.pdf` plots read and write data of *DRAM traffic per frame* vs workload complexity for compressible and non-compressible textures of the `write-only` worload (Figure 2b).
3. `./plot/GPU-band-total.pdf` plots *DRAM bandwidth* and *Rendering time per frame* vs workload complexity for compressible and non-compressible textures of both `read-only` and `write-only` worloads (Figure 3).

We provide two other figures for sanity check:

4. `./plot/GPUread_gpu_mem.pdf` plots iGPU frequency and memory utilization vs workload complexity for compressible and non-compressible textures of the `read-only` worload.
5. `./plot/GPUwrite_gpu_mem.pdf` plots iGPU frequency and memory utilization vs workload complexity for compressible and non-compressible textures of the `write-only` worload.
