# Overview

This repository contains the code to reproduce experiments in [the paper][paper]:

* GPU.zip: On the Side-Channel Implications of Hardware-Based Graphical Data Lossless Compression

## Preliminaries

* Required tool: perf, stress-ng
* Required shared libraries: OpenGL library (-lgl, core profile 3.3), GLFW library (-lglfw).
* Python virtual environment and package installation:

```bash
virtualenv -p python3 venv
source venv/bin/activate
pip install numpy
pip install matplotlib
pip install py-cpuinfo
```

## High level summary

1. `01-leakage-channel` reproduces Table 2, and Figure 2 and 3 in the paper.
2. `02-memory-stressor` reproduces Figure 4 in the paper.
3. `03-llc` reproduces Figure 5 in the paper.
5. `04-chrome-poc` contains the two PoCs in Section 5 of the paper. Reproducing Figure 14 and 15 can be done by uprobing Chrome and then run the PoCs with scripts and driver code similar to the ones in `01-leakage-channel`.
6. `poc` contains a OpenGL workload that attempts to trigger iGPU graphical data compression.
7. `uprobe-chromium` attaches a uprobe to the `Math.sqrt()` function in Chrome.

## Tested machine

1. `01-leakage-channel` is tested on an Intel i7-8700 (Ubuntu 22.04), i5-1135G7 (Debian 11.6), i7-12700K (Ubuntu 22.04), and AMD Ryzen 7 4800U (Ubuntu 20.04).
2. `02-memory-stressor` and `03-llc` are tested on an Intel i7-8700 (Ubuntu 22.04).
3. `04-chrome-poc` is tested on machines in Table 3 of the paper.

All tested machines are bare-metal machines. Running the code on other CPUs will likely require modification to the code (e.g., hardware measurement methodology).

## Citation

If you make any use of this code for academic purposes, please cite [the paper][paper]:

```tex
@inproceedings{wang2024gpuzip,
    author = {Yingchen Wang and Riccardo Paccagnella and Zhao Gang and Willy R. Vasquez and David Kohlbrenner and Hovav Shacham and Christopher W. Fletcher},
    title = {{GPU.zip}: On the Side-Channel Implications of Hardware-Based Graphical Data Compression},
    booktitle = {Proceedings of the IEEE Symposium on Security and Privacy (S&P)},
    year = {2024}
}
```

[paper]: https://www.hertzbleed.com/gpu.zip/GPU-zip.pdf