# OpenGL PoC for iGPU graphical data lossless compression

This OpenGL PoC creates textures with 4 distinct patterns: `Black`, `Random`, `Gradient` and `Skew` (Figure 1 in the paper).
We use this PoC to demonstrate the existence of iGPU graphical data lossless compression.

## Preliminaries

- Compiler: g++ with the C++17 standard.
- Required shared libraries: OpenGL library (-lgl), GLFW library (-lglfw).
- OpenGL version: core profile 3.3. Corresponding GLAD (OpenGL Loader Generator) locates at `./library/glad`.

## How to build

```bash
make
```

## PoC detail

The PoC takes in 5 parameters:

1. `color` specifies the pattern.
    1. `color` = `0` generates pattern `Black`.
    2. `color` = `1` generates pattern `Random`.
    3. `color` &mid; `SCR_WIDTH` such that `color` divides `SCR_WIDTH` (see below) generates pattern `Gradient`.
    4. `color` &nmid; `SCR_WIDTH` such that `color` does not divide `SCR_WIDTH` (see below) generates pattern `Skew`.
2. `iterations` is the workload complexity (See Algorithm 1 in the paper).
3. `SCR_WIDTH` defines the width of created texture. By default, the height of the created texture `SCR_HEIGHT` equals to `SCR_WIDTH`.
4. `read_write` determines the workload type.
    1. If `read_write` = `0`, the workload is `read-only`. During each frame update, it renders the gpu-created texture with specified pattern to the screen for `iterations` times.
    2. If `read_write` = `1`, the workload is `write-only`. During each frame update, it asks the gpu to create a texture with specified pattern for `iterations` times, and renders nothing.
    3. If `read_write` = `2`, the workload is `write-read`. During each frame update, it first asks the gpu to create a texture with specified pattern and then renders the gpu-created texture to the screen, and performs the two operations back-to-back for `iterations` times.
5. `print_time` logs *Rendering time per frame*.
    1. `print_time` = `0` logs nothing and the workload never terminates.
    2. `print_time` &ne; `0` logs the *Rendering time per frame* for `print_time` frame updates and terminates the workload. The logging result is saved into a file formatted as `time_${read_write}_${SCR_WIDTH}_${color}_${iterations}.txt`

## How to run

For example, the command:

```bash
./bin/texture 0.0 10 3000 0 500
```

renders a `Black` texture of size `3000x3000` to the screen for `10` iterations during each frame update, and collects the rendering time of `500` frame updates.

## Testing script

On most machines we tested, `Black` (`color` = `0`) is always compressible, and `Random` (`color` = `1`) is always non-compressible.
We provide a script that logs 500 *Rendering time per frame* data points for `Black` and `Random` textures with `iterations` ,`SCR_WIDTH`, and `read_write` as inputs.
To run the script:

```bash
cd scripts
./time.sh iterations SCR_WIDTH read_write
```

By default, `print_time` = `500`.

Above `./time.sh` uses `plot_time.py` to parse the two logging files of `Black` and `Random` textures.
It prints the average, std, min and max *Rendering time per frame* (CPU cycles) of each pattern.
It plots the distribution of the two *Rendering time per frame* traces in `plot` folder.

You can change the script to test the other patterns or the amount of data that the script logs.
