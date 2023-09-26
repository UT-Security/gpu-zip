#!/usr/bin/env bash


# Parse args
if [ $# -eq 3 ]; then
    iter=$1
    size=$2 
    type=$3
else
	size=3000 
    iter=100
    type=2
fi


../bin/texture 0.0 ${iter} ${size} ${type} 500 
../bin/texture 1.0 ${iter} ${size} ${type} 500

python plot_time.py time_${type}_${size}_0.0_${iter}.txt ./time_${type}_${size}_1.0_${iter}.txt

