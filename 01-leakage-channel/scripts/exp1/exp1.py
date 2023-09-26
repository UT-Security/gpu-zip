import warnings
warnings.simplefilter(action='ignore', category=FutureWarning)

import numpy as np
import os
import glob
import argparse
import cpuinfo

# Parse a pair of IMC and time trace. 
# Identify the block of IMC data that belongs to the same frame
def parse_files(imc, mem, time, gpu, CPUFreq):
    intervals = []
    with open(time) as f:
        for line in f:
            begin = int(line)
            end = 0
            try:
                end = next(f)
            except StopIteration:    
                break
            if(len(end) > 1):
                intervals.append((begin, int(end)))

    counter = 0
    imc_sum = 0
    time_total = []
    imc_total = []
    with open(imc) as f:
        for line in f:
            curr_imc = float(line.strip().split(",")[0])
            curr_time = int(line.strip().split(",")[1])
            if(counter>=len(intervals)):
                break
            # A new frame
            if(curr_time>=intervals[counter][1]):
                imc_total.append(imc_sum*1.04858) # MiB to MB
                time_total.append(float(intervals[counter][1]-intervals[counter][0])/(1000000*CPUFreq))
                counter = counter + 1
                imc_sum = 0
            elif(curr_time>=intervals[counter][0]):
                imc_sum = imc_sum + curr_imc

    mem_total = []
    # We only parse the Peak resident set size
    with open(mem) as f:
        for line in f:
            curr_peakRSS = int(line.strip().split(", ")[1])
            mem_total.append(curr_peakRSS)

    # Parse GPU frequency
    gpu_total = []
    with open(gpu) as f:
        for line in f:
            curr_gpu = int(line.strip().split(", ")[0])
            gpu_total.append(curr_gpu)

    return time_total, imc_total, mem_total, gpu_total



def parse_result(all_imc, all_mem, all_time, all_gpu):

    # Setting up texture selector -> texture name
    selector={}
    selector[0] = "Black"
    selector[1] = "Random"
    selector[100] = "Gradient"
    selector[101] = "Skew"

    # Parse data
    for label, trace in all_imc.items():
        
        # Filter time outliers (for the plot)
        time_filtered = []
        imc_filtered = []
        
        time_low = np.percentile(all_time[label], 5)
        time_high = np.percentile(all_time[label], 95)

        for curr_time_counter in range(len(all_time[label])):
            curr_time = all_time[label][curr_time_counter]
            if ( (curr_time>=time_low) and (curr_time<=time_high)):
                time_filtered.append(curr_time)
                imc_filtered.append(trace[curr_time_counter])
        
        # Filter mem outliers 
        mem_filtered = []
        mem_mean = np.mean(all_mem[label])
        mem_std = np.std(all_mem[label])
        for curr_mem in all_mem[label]:
            if ( abs(curr_mem-mem_mean) <= 4*mem_std):
                mem_filtered.append(curr_mem)

        # Filter gpu outliers 
        gpu_filtered = []
        gpu_mean = np.mean(all_gpu[label])
        gpu_std = np.std(all_gpu[label])
        for curr_gpu in all_gpu[label]:
            if ( abs(curr_gpu-gpu_mean) <= 4*gpu_std):
                gpu_filtered.append(curr_gpu)

        print("%s: \n\tDRAM traffic per frame (MB): %2f +- %5f \n\tRendering time per frame (ms): %2f +- %5f \n\tPeak RSS (KiB): %2f +- %5f \n\tGPU frequency (MHz): %2f +- %5f" % (selector[label], np.mean(imc_filtered), np.std(imc_filtered), np.mean(time_filtered), np.std(time_filtered), np.mean(mem_filtered), np.std(mem_filtered), np.mean(gpu_filtered), np.std(gpu_filtered) ))


def main():
    info = cpuinfo.get_cpu_info()
    
    # Parse arguments
    parser = argparse.ArgumentParser()
    parser.add_argument('folder')
    parser.add_argument('time')
    args = parser.parse_args()
    in_dir = args.folder
    time_dir = args.time
    CPUFreq = float(info["hz_advertised"][0]/1000000000)

    # Read IMC data
    imc_files = sorted(glob.glob(in_dir + "/imc*"), reverse=True)
    imc_files.sort(key=lambda x: os.path.getmtime(x))
    # Read TIME data
    time_files = sorted(glob.glob(time_dir + "/time*"), reverse=True)
    time_files.sort(key=lambda x: os.path.getmtime(x))
    # Read MEM data
    mem_files = sorted(glob.glob(in_dir + "/mem*"), reverse=True)
    mem_files.sort(key=lambda x: os.path.getmtime(x))
    # Read GPU data
    gpu_files = sorted(glob.glob(in_dir + "/gpu*"), reverse=True)
    gpu_files.sort(key=lambda x: os.path.getmtime(x))

    total = len(imc_files)

    time_all = {}
    imc_all = {}
    mem_all = {}
    gpu_all = {}
    for counter in range(total):
        curr_imc_file = imc_files[counter]
        curr_mem_file = mem_files[counter]
        curr_time_file = time_files[counter]
        curr_gpu_file = gpu_files[counter]

        curr_time, curr_imc, curr_mem, curr_gpu = parse_files(curr_imc_file, curr_mem_file, curr_time_file, curr_gpu_file, CPUFreq)

        label = curr_time_file.split("/")[-1].split(".txt")[0]
        selector = int(float(label.split("_")[3]))
        time_all.setdefault(selector, []).extend(curr_time)
        imc_all.setdefault(selector, []).extend(curr_imc)
        mem_all.setdefault(selector, []).extend(curr_mem)
        gpu_all.setdefault(selector, []).extend(curr_gpu)

    parse_result(imc_all, mem_all, time_all, gpu_all)

    
if __name__ == "__main__":
    main()
