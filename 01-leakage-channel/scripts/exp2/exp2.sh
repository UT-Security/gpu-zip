#!/usr/bin/env bash

# Load MSR module
sudo modprobe msr


# Setup
samples=400000 # 400 seconds
date=`date +"%m%d-%H%M"`


############ monitor GPU and IMC perf events ###############

sudo rm -rf out
mkdir out
sudo rm -rf input.txt


#### read-only from texture of four different pattern: Black, Random, Gradient, Skew
#### Increase workload complexity

echo ../../../poc/gpu-create/bin/texture 0.0 1 3000 0 10000000 >> input.txt 
echo ../../../poc/gpu-create/bin/texture 1.0 1 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 100.0 1 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 101.0 1 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 0.0 4 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 1.0 4 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 100.0 4 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 101.0 4 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 0.0 8 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 1.0 8 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 100.0 8 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 101.0 8 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 0.0 12 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 1.0 12 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 100.0 12 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 101.0 12 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 0.0 16 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 1.0 16 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 100.0 16 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 101.0 16 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 0.0 20 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 1.0 20 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 100.0 20 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 101.0 20 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 0.0 24 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 1.0 24 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 100.0 24 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 101.0 24 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 0.0 28 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 1.0 28 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 100.0 28 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 101.0 28 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 0.0 32 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 1.0 32 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 100.0 32 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 101.0 32 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 0.0 36 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 1.0 36 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 100.0 36 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 101.0 36 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 0.0 40 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 1.0 40 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 100.0 40 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 101.0 40 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 0.0 44 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 1.0 44 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 100.0 44 3000 0 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 101.0 44 3000 0 10000000 >> input.txt

#### write-only to texture of four different pattern: Black, Random, Gradient, Skew
#### Increase workload complexity

echo ../../../poc/gpu-create/bin/texture 0.0 1 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 1.0 1 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 100.0 1 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 101.0 1 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 0.0 4 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 1.0 4 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 100.0 4 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 101.0 4 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 0.0 8 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 1.0 8 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 100.0 8 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 101.0 8 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 0.0 12 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 1.0 12 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 100.0 12 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 101.0 12 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 0.0 16 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 1.0 16 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 100.0 16 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 101.0 16 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 0.0 20 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 1.0 20 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 100.0 20 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 101.0 20 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 0.0 24 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 1.0 24 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 100.0 24 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 101.0 24 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 0.0 28 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 1.0 28 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 100.0 28 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 101.0 28 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 0.0 32 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 1.0 32 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 100.0 32 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 101.0 32 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 0.0 36 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 1.0 36 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 100.0 36 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 101.0 36 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 0.0 40 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 1.0 40 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 100.0 40 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 101.0 40 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 0.0 44 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 1.0 44 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 100.0 44 3000 1 10000000 >> input.txt
echo ../../../poc/gpu-create/bin/texture 101.0 44 3000 1 10000000 >> input.txt

sudo ../../bin/driver 1 1 ${samples} 1
sudo mkdir time-exp2-${date}
sudo mv time_* ./time-exp2-${date}

sudo mv ./out ../../data/exp2-${date}
sudo mv ./time-exp2-${date} ../../data/time-exp2-${date}

# Unload MSR module
sudo modprobe -r msr
