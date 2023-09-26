#!/usr/bin/env bash
rm -r data
mkdir data

# texture size range: 1000*1000*4 -> 3.8MiB, 2700*2700*4 -> 27.8MiB
for i in {1000..2700..50}
    do
        ../bin/texture 0.0 2000 ${i} ./data/w0_${i}.txt
        sleep 20
        ../bin/texture 1.0 2000 ${i} ./data/w1_${i}.txt
        sleep 20
done

