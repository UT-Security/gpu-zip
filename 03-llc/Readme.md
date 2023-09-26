# Pattern-dependent last level cache utilization

This folder tests that compressible versus non-compressible textures leaves pattern-dependent *LLC walk time* (metric for contention over the last level cache. See paper S3.4).
Script `./scripts/llc.sh` generates data for reproducing Figure 5 in the paper.

## Preliminaries

- Compiler: g++ with the C++17 standard.
- Required shared libraries: OpenGL library (-lgl), GLFW library (-lglfw).
- OpenGL version: core profile 3.3. Corresponding GLAD (OpenGL Loader Generator) locates at `../poc/library/glad`.

## Tested machine

Script `./scripts/llc.sh` is tested on an Intel i7-8700 running Ubuntu 22.04.
Running the scripts on other CPUs will require modification to the code (machine-specific parameters in `pputil.c`, and texture size range in `./scripts/llc.sh`).

## How to build

```bash
make
```

## How to reproduce Figure 5 in the paper

```bash
cd scripts
./llc.sh
```

Above script measures the *LLC walk time* after the iGPU writes to a compressible or non-compressible texture of size from 3.8 MiB to 27.8 MiB.
For each experiment, we collect 2000 *LLC walk time* data points.

To plot the collected data:

```bash
python plot_llc_size.py ./data/ 
```

The plot similar to Figure 5 in the paper can be found in the plot subfolder (`./plot/llc_size.pdf`).
