import warnings
warnings.simplefilter(action='ignore', category=FutureWarning)
import cpuinfo
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import os
import numpy as np
import glob
import argparse
from distutils.dir_util import remove_tree


def parse_files(imc, time, gpu, mem, CPUFreq):
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
    read_sum = 0
    write_sum = 0
    read_total = []
    write_total = []
    total_band = []
    time_total = []
    prev_time = 0
    with open(imc) as f:
        for line in f:
            read = float(line.strip().split(",")[0])
            write = float(line.strip().split(",")[1])
            curr_time = int(line.strip().split(",")[2])
            if(counter>=len(intervals)):
                break
            # A new frame
            if(curr_time>=intervals[counter][1]):
                read_total.append(read_sum*1.04858)   # MiB to MB
                write_total.append(write_sum*1.04858) # MiB to MB
                time_total.append(float(intervals[counter][1]-intervals[counter][0])/(1000000*CPUFreq))
                counter = counter + 1
                read_sum = 0
                write_sum = 0
            elif(curr_time>=intervals[counter][0]):
                read_sum = read_sum + read
                write_sum = write_sum + write
                total_band.append((read+write)*1.04858/(1000*(float(curr_time-prev_time)/(1000000000*CPUFreq))))
                prev_time = curr_time

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

    return read_total, write_total, total_band, time_total, gpu_total, mem_total


def plot_single(all_read, all_write, all_time, plot_name):

    read = {}
    read_std = {}
    write = {}
    write_std = {}

    minimum = 100000
    maximum = 0

    # Parse data: data read 
    for label, trace in all_read.items():
        
        # Filter time outliers (for the plot)
        samples_filtered = []
        time_low = np.percentile(all_time[label], 5)
        time_high = np.percentile(all_time[label], 95)
        for curr_time_counter in range(len(all_time[label])):
            curr_time = all_time[label][curr_time_counter]
            if ( (curr_time>=time_low) and (curr_time<=time_high)):
                samples_filtered.append(trace[curr_time_counter])

        minimum = min(min(samples_filtered), minimum)
        maximum = max(max(samples_filtered), maximum)

        # Store data for scatter
        curr_layer = int(label.split("_")[4])
        curr_size = int(label.split("_")[2])
        curr_pattern = int(float(label.split("_")[3]))
        c_nc = -1
        if(curr_pattern==1):
            c_nc = 1
        elif((curr_pattern==0) or (curr_size%curr_pattern == 0)):
            c_nc = 0
        else:
            c_nc = 1
        
        if(curr_layer not in read):
            read[curr_layer]={}
        read[curr_layer].setdefault(c_nc, []).append(np.mean(samples_filtered))
        if(curr_layer not in read_std):
            read_std[curr_layer]={}
        read_std[curr_layer].setdefault(c_nc, []).append(np.std(samples_filtered))

    # Parse data: data write
    for label, trace in all_write.items():
        
        # Filter outliers (for the plot)
        samples_filtered = []
        time_low = np.percentile(all_time[label], 5)
        time_high = np.percentile(all_time[label], 95)
        for curr_time_counter in range(len(all_time[label])):
            curr_time = all_time[label][curr_time_counter]
            if ( (curr_time>=time_low) and (curr_time<=time_high)):
                samples_filtered.append(trace[curr_time_counter])

        minimum = min(min(samples_filtered), minimum)
        maximum = max(max(samples_filtered), maximum)

        # Store data for scatter
        curr_layer = int(label.split("_")[4])
        curr_size = int(label.split("_")[2])
        curr_pattern = int(float(label.split("_")[3]))
        c_nc = -1
        if(curr_pattern==1):
            c_nc = 1
        elif((curr_pattern==0) or (curr_size%curr_pattern == 0)):
            c_nc = 0
        else:
            c_nc = 1

        if(curr_layer not in write):
            write[curr_layer]={}
        write[curr_layer].setdefault(c_nc, []).append(np.mean(samples_filtered))
        if(curr_layer not in write_std):
            write_std[curr_layer]={}
        write_std[curr_layer].setdefault(c_nc, []).append(np.std(samples_filtered))

    # Plot all data
    plt_label = []
    plt_nc = [] # non-compressible
    plt_c = [] # compressible
    plt_nc_std = []
    plt_c_std = []
    
    plt_write_label = []
    plt_write_nc = []
    plt_write_c = []
    plt_write_nc_std = []
    plt_write_c_std = []
    
    for layer in read:
        plt_label.append(layer)
        plt_nc.append(np.mean(read[layer][1]))
        plt_c.append(np.mean(read[layer][0]))
        plt_nc_std.append(np.mean(read_std[layer][1]))
        plt_c_std.append(np.mean(read_std[layer][0]))

    for layer in write:
        plt_write_label.append(layer)
        plt_write_nc.append(np.mean(write[layer][1]))
        plt_write_c.append(np.mean(write[layer][0]))
        plt_write_nc_std.append(np.mean(write_std[layer][1]))
        plt_write_c_std.append(np.mean(write_std[layer][0]))

    fig, (a1,a2) = plt.subplots(1, 2, figsize=(5.0, 2))

    a1.scatter(plt_label, plt_c, s=2, c = "navy", label="Compressible")
    a1.scatter(plt_label, plt_nc, s=2, c = "darkorange", label="Non-Compressible")
    a1.errorbar(plt_label, plt_c, c = "navy", yerr=plt_c_std, fmt=".")
    a1.errorbar(plt_label, plt_nc, c = "darkorange", yerr=plt_nc_std, fmt=".")
    a1.set_xlabel('Number of iterations', fontsize = 9)
    a1.set_ylabel('%s (%s)'%("DRAM traffic per frame", "MB"), fontsize = 9)
    a1.yaxis.set_label_coords(-0.27,0.6)
    a1.set_title('DRAM read', fontsize=10)
    a1.set_ylim([0, 1800])
    a1.set_yticks(np.arange(0, 2000, 500))
    #a1.tick_params(axis="both", labelsize=8)

    a2.scatter(plt_write_label, plt_write_c, s=2, c = "navy", label="Compressible")
    a2.scatter(plt_write_label, plt_write_nc, s=2, c = "darkorange", label="Non-Compressible")
    a2.errorbar(plt_write_label, plt_write_c, c = "navy", yerr=plt_write_c_std, fmt=".")
    a2.errorbar(plt_write_label, plt_write_nc, c = "darkorange", yerr=plt_write_nc_std, fmt=".")
    a2.set_xlabel('Number of iterations', fontsize = 9)
    a2.set_title('DRAM write', fontsize=10)
    a2.set_ylim([0, 1800])
    a2.set_yticks(np.arange(0, 2000, 500))
    #a2.tick_params(axis='both', labelsize=8)

    handles, labels = a2.get_legend_handles_labels()
    fig.legend(handles, labels, bbox_to_anchor=(0.85, 1.0), ncol=2, markerscale=4, fontsize=10)
    plt.subplots_adjust(wspace=0.4,left=0.13,top=0.70,right=0.97,bottom=0.2)
    plt.savefig("./plot/%s.pdf" % plot_name, dpi=300)


def plot_gpu_mem(all_gpu, all_mem, plot_name):

    gpu = {}
    gpu_stds = {}
    mem = {}
    mem_stds = {}

    # Parse data: data read 
    for label, trace in all_gpu.items():
        
        # Filter gpu outliers 
        gpu_filtered = []
        gpu_mean = np.mean(trace)
        gpu_std = np.std(trace)
        for curr_gpu in trace:
            if ( abs(curr_gpu-gpu_mean) <= 4*gpu_std):
                gpu_filtered.append(curr_gpu)

        # Filter mem outliers 
        mem_filtered = []
        mem_mean = np.mean(all_mem[label])
        mem_std = np.std(all_mem[label])
        for curr_mem in all_mem[label]:
            if ( abs(curr_mem-mem_mean) <= 4*mem_std):
                mem_filtered.append(curr_mem)

        # Store data for scatter
        curr_layer = int(label.split("_")[4])
        curr_size = int(label.split("_")[2])
        curr_pattern = int(float(label.split("_")[3]))
        c_nc = -1
        if(curr_pattern==1):
            c_nc = 1
        elif((curr_pattern==0) or (curr_size%curr_pattern == 0)):
            c_nc = 0
        else:
            c_nc = 1

        if(curr_layer not in gpu):
            gpu[curr_layer]={}
        gpu[curr_layer].setdefault(c_nc, []).append(np.mean(gpu_filtered))
        if(curr_layer not in gpu_stds):
            gpu_stds[curr_layer]={}
        gpu_stds[curr_layer].setdefault(c_nc, []).append(np.std(gpu_filtered))

        if(curr_layer not in mem):
            mem[curr_layer]={}
        mem[curr_layer].setdefault(c_nc, []).append(np.mean(mem_filtered))
        if(curr_layer not in mem_stds):
            mem_stds[curr_layer]={}
        mem_stds[curr_layer].setdefault(c_nc, []).append(np.std(mem_filtered))

    
    # Plot all data
    plt_label = []
    plt_gpu_nc = [] # non-compressible
    plt_gpu_c = [] # compressible
    plt_gpu_nc_std = []
    plt_gpu_c_std = []
    
    plt_mem_label = []
    plt_mem_nc = []
    plt_mem_c = []
    plt_mem_nc_std = []
    plt_mem_c_std = []
    
    for layer in gpu:
        plt_label.append(layer)
        plt_gpu_nc.append(np.mean(gpu[layer][1]))
        plt_gpu_c.append(np.mean(gpu[layer][0]))
        plt_gpu_nc_std.append(np.mean(gpu_stds[layer][1]))
        plt_gpu_c_std.append(np.mean(gpu_stds[layer][0]))

    for layer in mem:
        plt_mem_label.append(layer)
        plt_mem_nc.append(np.mean(mem[layer][1]))
        plt_mem_c.append(np.mean(mem[layer][0]))
        plt_mem_nc_std.append(np.mean(mem_stds[layer][1]))
        plt_mem_c_std.append(np.mean(mem_stds[layer][0]))

    fig, (a1,a2) = plt.subplots(1, 2, figsize=(5.0, 2))

    a1.scatter(plt_label, plt_gpu_c, s=2, c = "navy", label="Compressible")
    a1.scatter(plt_label, plt_gpu_nc, s=2, c = "darkorange", label="Non-Compressible")
    a1.errorbar(plt_label, plt_gpu_c, c = "navy", yerr=plt_gpu_c_std, fmt=".")
    a1.errorbar(plt_label, plt_gpu_nc, c = "darkorange", yerr=plt_gpu_nc_std, fmt=".")
    a1.set_xlabel('Number of iterations', fontsize = 9)
    a1.set_ylabel('%s (%s)'%("GPU frequency", "MHz"), fontsize = 9)
    a1.yaxis.set_label_coords(-0.27,0.6)
    a1.set_title('GPU frequency', fontsize=10)
    #a1.tick_params(axis="both", labelsize=8)

    a2.scatter(plt_mem_label, plt_mem_c, s=2, c = "navy", label="Compressible")
    a2.scatter(plt_mem_label, plt_mem_nc, s=2, c = "darkorange", label="Non-Compressible")
    a2.errorbar(plt_mem_label, plt_mem_c, c = "navy", yerr=plt_mem_c_std, fmt=".")
    a2.errorbar(plt_mem_label, plt_mem_nc, c = "darkorange", yerr=plt_mem_nc_std, fmt=".")
    a2.set_xlabel('Number of iterations', fontsize = 9)
    a2.set_ylabel('%s (%s)'%("Memory Utilization", "KiB"), fontsize = 9)
    a2.set_title('Memory Utilization', fontsize=10)
    #a2.tick_params(axis='both', labelsize=8)

    handles, labels = a2.get_legend_handles_labels()
    fig.legend(handles, labels, bbox_to_anchor=(0.85, 1.0), ncol=2, markerscale=4, fontsize=10)
    plt.subplots_adjust(wspace=0.4,left=0.13,top=0.70,right=0.97,bottom=0.2)
    plt.savefig("./plot/%s.pdf" % plot_name, dpi=300)


def plot_bandwidth(all_read, all_write, all_time_read, all_time_write, plot_name):

    total_r = {}
    total_r_std = {}
    times_r = {}
    times_r_std = {}

    total_w = {}
    total_w_std = {}
    times_w = {}
    times_w_std = {}
    
    # Parse data for read-only workload
    for label, trace in all_read.items():
        
        # Filter outliers (for the plot)
        samples_filtered = []
        samples_mean = np.mean(trace)
        samples_std = np.std(trace)
        for sample in trace:
            if ( abs(sample-samples_mean) <= 4*samples_std):
                samples_filtered.append(sample)

        # Filter time outliers 
        time_filtered = []
        time_mean = np.mean(all_time_read[label])
        time_std = np.std(all_time_read[label])
        for curr_time in all_time_read[label]:
            if ( abs(curr_time-time_mean) <= 4*time_std):
                time_filtered.append(curr_time)

    
        # Store data for scatter
        curr_layer = int(label.split("_")[4])
        curr_size = int(label.split("_")[2])
        curr_pattern = int(float(label.split("_")[3]))
        c_nc = -1
        if(curr_pattern==1):
            c_nc = 1
        elif((curr_pattern==0) or (curr_size%curr_pattern == 0)):
            c_nc = 0
        else:
            c_nc = 1

        if(curr_layer not in total_r):
            total_r[curr_layer]={}
            times_r[curr_layer]={}
        total_r[curr_layer].setdefault(c_nc, []).append(np.mean(samples_filtered))
        times_r[curr_layer].setdefault(c_nc, []).append(np.mean(time_filtered))

        if(curr_layer not in total_r_std):
            total_r_std[curr_layer]={}
            times_r_std[curr_layer]={}
        total_r_std[curr_layer].setdefault(c_nc, []).append(np.std(samples_filtered))
        times_r_std[curr_layer].setdefault(c_nc, []).append(np.std(time_filtered))


    # Parse data for write-only workload
    for label, trace in all_write.items():
        
        # Filter outliers (for the plot)
        samples_filtered = []
        samples_mean = np.mean(trace)
        samples_std = np.std(trace)
        for sample in trace:
            if ( abs(sample-samples_mean) <= 4*samples_std):
                samples_filtered.append(sample)

        # Filter time outliers 
        time_filtered = []
        time_mean = np.mean(all_time_write[label])
        time_std = np.std(all_time_write[label])
        for curr_time in all_time_write[label]:
            if ( abs(curr_time-time_mean) <= 4*time_std):
                time_filtered.append(curr_time)

        # Store data for scatter
        curr_layer = int(label.split("_")[4])
        curr_size = int(label.split("_")[2])
        curr_pattern = int(float(label.split("_")[3]))
        c_nc = -1
        if(curr_pattern==1):
            c_nc = 1
        elif((curr_pattern==0) or (curr_size%curr_pattern == 0)):
            c_nc = 0
        else:
            c_nc = 1

        if(curr_layer not in total_w):
            total_w[curr_layer]={}
            times_w[curr_layer]={}
        total_w[curr_layer].setdefault(c_nc, []).append(np.mean(samples_filtered))
        times_w[curr_layer].setdefault(c_nc, []).append(np.mean(time_filtered))

        if(curr_layer not in total_w_std):
            total_w_std[curr_layer]={}
            times_w_std[curr_layer]={}
        total_w_std[curr_layer].setdefault(c_nc, []).append(np.std(samples_filtered))
        times_w_std[curr_layer].setdefault(c_nc, []).append(np.std(time_filtered))


    # Plot all data
    plt_label = []
    plt_nc = []
    plt_c = []
    plt_nc_std = []
    plt_c_std = []

    plt_time_nc = []
    plt_time_c = []
    plt_time_nc_std = []
    plt_time_c_std = []

    for layer in total_r:
        plt_label.append(layer)
        plt_nc.append(np.mean(total_r[layer][1]))
        plt_c.append(np.mean(total_r[layer][0]))
        plt_nc_std.append(np.mean(total_r_std[layer][1]))
        plt_c_std.append(np.mean(total_r_std[layer][0]))

        plt_time_nc.append(np.mean(times_r[layer][1]))
        plt_time_c.append(np.mean(times_r[layer][0]))
        plt_time_nc_std.append(np.mean(times_r_std[layer][1]))
        plt_time_c_std.append(np.mean(times_r_std[layer][0]))

    fig, (ax1,ax3) = plt.subplots(1, 2, figsize=(6.1, 3.0))
    ax1.set_title('Read workload', fontsize=10)
    ax2 = ax1.twinx()
    ax1.scatter(plt_label, plt_c, s=2, c = "navy", label="DRAM bandwidth Compressible")
    ax1.scatter(plt_label, plt_nc, s=2, c = "royalblue", label="DRAM bandwidth Non-Compressible")
    ax1.errorbar(plt_label, plt_c, c = "navy", yerr=plt_c_std, fmt=".")
    ax1.errorbar(plt_label, plt_nc, c = "royalblue", yerr=plt_nc_std, fmt=".")
    ax2.scatter(plt_label, plt_time_c, s=2, c = "darkred", marker = '*', label="Rendering time Compressible")
    ax2.scatter(plt_label, plt_time_nc, s=2, c = "lightcoral", marker = '*', label="Rendering time Non-Compressible")
    ax2.errorbar(plt_label, plt_time_c, c = "darkred", yerr=plt_time_c_std, fmt="*")
    ax2.errorbar(plt_label, plt_time_nc, c = "lightcoral", yerr=plt_time_nc_std, fmt="*")
    #ax1.tick_params(axis='both', labelsize=8)
    #ax2.tick_params(axis="y", labelsize=8)
    ax1.set_xlabel('Number of iterations', fontsize = 9)
    ax1.set_ylabel('%s (%s)'%("DRAM bandwidth", "GB/s"), fontsize = 9, color="blue")
    ax1.yaxis.set_major_formatter(ticker.FormatStrFormatter('%.0f'))
    ax1.tick_params(colors='blue')
    ax1.set_yticks(np.arange(0, 40, 5))
    ax2.tick_params(colors='red')
    ax2.set_yticks(np.arange(0, 150, 20))
    

    plt_label = []
    plt_nc = []
    plt_c = []
    plt_nc_std = []
    plt_c_std = []

    plt_time_nc = []
    plt_time_c = []
    plt_time_nc_std = []
    plt_time_c_std = []

    for layer in total_w:
        plt_label.append(layer)
        plt_nc.append(np.mean(total_w[layer][1]))
        plt_c.append(np.mean(total_w[layer][0]))
        plt_nc_std.append(np.mean(total_w_std[layer][1]))
        plt_c_std.append(np.mean(total_w_std[layer][0]))

        plt_time_nc.append(np.mean(times_w[layer][1]))
        plt_time_c.append(np.mean(times_w[layer][0]))
        plt_time_nc_std.append(np.mean(times_w_std[layer][1]))
        plt_time_c_std.append(np.mean(times_w_std[layer][0]))

    ax4 = ax3.twinx()
    ax3.set_title('Write workload', fontsize=10)
    ax3.scatter(plt_label, plt_c, s=2, c = "navy", label="DRAM bandwidth Compressible")
    ax3.scatter(plt_label, plt_nc, s=2, c = "royalblue", label="DRAM bandwidth Non-Compressible")
    ax3.errorbar(plt_label, plt_c, c = "navy", yerr=plt_c_std, fmt=".")
    ax3.errorbar(plt_label, plt_nc, c = "royalblue", yerr=plt_nc_std, fmt=".")
    ax4.scatter(plt_label, plt_time_c, s=2, c = "darkred", marker = '*', label="Rendering time Compressible")
    ax4.scatter(plt_label, plt_time_nc, s=2, c = "lightcoral", marker = '*', label="Rendering time Non-Compressible")
    ax4.errorbar(plt_label, plt_time_c, c = "darkred", yerr=plt_time_c_std, fmt="*")
    ax4.errorbar(plt_label, plt_time_nc, c = "lightcoral", yerr=plt_time_nc_std, fmt="*")
    #ax3.tick_params(axis='both', labelsize=8)
    #ax4.tick_params(axis="y", labelsize=8)
    ax3.set_xlabel('Number of iterations', fontsize = 9)
    ax3.yaxis.set_major_formatter(ticker.FormatStrFormatter('%.0f'))
    ax4.set_ylabel('%s (%s)'%("Rendering time", "ms"), fontsize = 9, color="red")
    ax3.tick_params(colors='blue')
    ax3.set_yticks(np.arange(0, 40, 5))
    ax4.tick_params(colors='red')
    ax4.set_yticks(np.arange(0, 150, 20))

    handles, labels = ax3.get_legend_handles_labels()
    handles_second, labels_second = ax4.get_legend_handles_labels()
    handles.extend(handles_second)
    labels.extend(labels_second)

    fig.legend(handles, labels, bbox_to_anchor=(1.01, 1.0), ncol=2, markerscale=4, fontsize=10)
    plt.subplots_adjust(wspace=0.3,left=0.08,top=0.75,right=0.91,bottom=0.15)
    plt.savefig("./plot/%s.pdf" % plot_name, dpi=300)



def main():

    info = cpuinfo.get_cpu_info()

    # Prepare output directory
    try:
        remove_tree('plot')
    except:
        pass
    try:
        os.makedirs('plot')
    except:
        pass

    # Parse arguments
    parser = argparse.ArgumentParser()
    parser.add_argument('folder')
    parser.add_argument('time')
    args = parser.parse_args()
    in_dir = args.folder
    time_dir = args.time
    CPUFreq = float(info["hz_advertised"][0]/1000000000)

    # Read data
    imc_files = sorted(glob.glob(in_dir + "/imc*"), reverse=True)
    imc_files.sort(key=lambda x: os.path.getmtime(x))
    # Read rendering time data
    time_files = sorted(glob.glob(time_dir + "/time*"), reverse=True)
    time_files.sort(key=lambda x: os.path.getmtime(x))
    # Read MEM data
    mem_files = sorted(glob.glob(in_dir + "/mem*"), reverse=True)
    mem_files.sort(key=lambda x: os.path.getmtime(x))
    # Read GPU data
    gpu_files = sorted(glob.glob(in_dir + "/gpu*"), reverse=True)
    gpu_files.sort(key=lambda x: os.path.getmtime(x))

    total = int(len(imc_files)/2)

    # read-only worload
    read_all = {}
    write_all = {}
    time_read_all = {}
    band_read = {}
    mem_all = {}
    gpu_all = {}
    for counter in range(total):

        curr_imc_file = imc_files[counter]
        curr_time_file = time_files[counter]
        curr_mem_file = mem_files[counter]
        curr_gpu_file = gpu_files[counter]

        curr_read, curr_write, curr_band, curr_time, curr_gpu, curr_mem = parse_files(curr_imc_file, curr_time_file, curr_gpu_file, curr_mem_file, CPUFreq)
        label_time = curr_time_file.split("/")[-1].split(".txt")[0]

        read_all.setdefault(label_time, []).extend(curr_read)
        write_all.setdefault(label_time, []).extend(curr_write)
        time_read_all.setdefault(label_time, []).extend(curr_time)
        band_read.setdefault(label_time, []).extend(curr_band)
        mem_all.setdefault(label_time, []).extend(curr_mem)
        gpu_all.setdefault(label_time, []).extend(curr_gpu)
                
    # Plot the DRAM read and write data of read-only workload (compressible and non-compressible texture) as workload complexity increases 
    plot_single(read_all, write_all, time_read_all, "GPUread")
    # Plot GPU frequency and memory utilization  (peak RSS) for sanity check
    plot_gpu_mem(gpu_all, mem_all,  "GPUread_gpu_mem")

    # write-only worload
    read_all = {}
    write_all = {}
    time_write_all = {}
    band_write = {}
    mem_all = {}
    gpu_all = {}
    for counter in range(total, total*2):

        curr_imc_file = imc_files[counter]
        curr_time_file = time_files[counter]
        curr_mem_file = mem_files[counter]
        curr_gpu_file = gpu_files[counter]

        curr_read, curr_write, curr_band, curr_time, curr_gpu, curr_mem = parse_files(curr_imc_file, curr_time_file, curr_gpu_file, curr_mem_file, CPUFreq)
        label_time = curr_time_file.split("/")[-1].split(".txt")[0]

        read_all.setdefault(label_time, []).extend(curr_read)
        write_all.setdefault(label_time, []).extend(curr_write)
        time_write_all.setdefault(label_time, []).extend(curr_time)
        band_write.setdefault(label_time, []).extend(curr_band)
        mem_all.setdefault(label_time, []).extend(curr_mem)
        gpu_all.setdefault(label_time, []).extend(curr_gpu)

    # Plot the DRAM read and write data of write-only workload (compressible and non-compressible texture) as workload complexity increases       
    plot_single(read_all, write_all, time_write_all, "GPUwrite")
    # Plot GPU frequency and memory utilization  (peak RSS) for sanity check
    plot_gpu_mem(gpu_all, mem_all,  "GPUwrite_gpu_mem")
    
    plot_bandwidth(band_read, band_write, time_read_all, time_write_all, "GPU-band-total")
    
if __name__ == "__main__":
    main()
