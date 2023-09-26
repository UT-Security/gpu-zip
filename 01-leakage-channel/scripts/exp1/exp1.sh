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

#### write-read four patterns: black, random, gradient, skew 

echo ../../../poc/gpu-create/bin/texture 0.0   20 3000 2 10000000 >> input.txt # black
echo ../../../poc/gpu-create/bin/texture 1.0   20 3000 2 10000000 >> input.txt # random
echo ../../../poc/gpu-create/bin/texture 100.0 20 3000 2 10000000 >> input.txt # gradient
echo ../../../poc/gpu-create/bin/texture 101.0 20 3000 2 10000000 >> input.txt # skew

sudo ../../bin/driver 1 2 ${samples} 1

sudo mkdir time-exp1-${date}
sudo mv time_* ./time-exp1-${date}

sudo mv ./out ../../data/exp1-${date}
sudo mv ./time-exp1-${date} ../../data/time-exp1-${date}

# Unload MSR module
sudo modprobe -r msr