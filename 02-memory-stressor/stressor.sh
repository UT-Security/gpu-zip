#!/usr/bin/env bash

############ congestion write ###############

mkdir time

for i in {1..12}
do

	stress-ng --memcpy $i &

	sleep 30

	../poc/gpu-create/bin/texture 0.0 10 3000 1 10000
	../poc/gpu-create/bin/texture 1.0 10 3000 1 10000

	pkill -f stress-ng

	mkdir ./time/out-${i}
	sudo mv time_* ./time/out-${i}

done