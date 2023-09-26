from curses.ascii import isdigit
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

def parse_files(time, CPUFreq):

    time_total = []
    with open(time) as f:
        for line in f:
            begin = int(line)
            end = 0
            try:
                end = next(f)
            except StopIteration:    
                break
            if(len(end) > 1):
                time_total.append(float(int(end)-begin)/(1000000*CPUFreq))

    return time_total



def plot_single(all_time_black, all_time_random, plot_name, name, unit):

    times_black = {}
    times_black_std = {}
    times_random = {}
    times_random_std = {}

    # Parse data
    for label, trace in all_time_black.items():
        
        # Filter time black outliers 
        time_black_filtered = []
        black_mean = np.mean(trace)
        black_std = np.std(trace)
        for curr_time in trace:
            if ( abs(curr_time-black_mean) <= 4*black_std):
                time_black_filtered.append(curr_time)

        # Filter time random outliers 
        time_random_filtered = []
        random_mean = np.mean(all_time_random[label])
        random_std = np.std(all_time_random[label])
        for curr_time in all_time_random[label]:
            if ( abs(curr_time-random_mean) <= 4*random_std):
                time_random_filtered.append(curr_time)

        # Store data for scatter
        curr_label = int(label)
        num_stressor = curr_label
        
        times_black[num_stressor] = np.mean(time_black_filtered)
        times_random[num_stressor] = np.mean(time_random_filtered)
        times_black_std[num_stressor] = np.std(time_black_filtered)
        times_random_std[num_stressor] = np.std(time_random_filtered)

    
    # Plot all data
    plt_label = []
    plt_time_black = []
    plt_time_black_std = []
    plt_time_random = []
    plt_time_random_std = []

    for layer in times_black:
        plt_label.append(layer)
        plt_time_black.append(times_black[layer])
        plt_time_black_std.append(times_black_std[layer])
        plt_time_random.append(times_random[layer])
        plt_time_random_std.append(times_random_std[layer])
    
    
    fig, a1 = plt.subplots(1, 1, figsize=(3, 2))

    plt.scatter(plt_label, plt_time_black, s=2, c = "navy", label="Compressible")
    markers, caps, bars = plt.errorbar(plt_label, plt_time_black, c = "navy", yerr=plt_time_black_std, fmt=".")
    [bar.set_alpha(0.5) for bar in bars]
    [cap.set_alpha(0.5) for cap in caps]

    plt.scatter(plt_label, plt_time_random, s=2, c = "darkorange", label="Non-compressible")
    markers, caps, bars = plt.errorbar(plt_label, plt_time_random, c = "darkorange", yerr=plt_time_random_std, fmt=".")
    [bar.set_alpha(0.5) for bar in bars]
    [cap.set_alpha(0.5) for cap in caps]
    
    plt.gca().xaxis.set_major_locator(ticker.MultipleLocator(2))
    plt.xlabel('Number of memory stressor', fontsize = 8)
    plt.ylabel('%s (%s)'%(name, unit), fontsize = 8)

    plt.legend(loc='upper center', bbox_to_anchor=(0.42, 1.35), ncol=2, markerscale=4, fontsize=8)
    plt.subplots_adjust(left=0.16,top=0.80,right=0.98,bottom=0.2)

    plt.savefig("./plot/%s.pdf" % plot_name, dpi=300)

    

def main():
    # Prepare output directory
    try:
        remove_tree('plot')
    except:
        pass
    try:
        os.makedirs('plot')
    except:
        pass
    
    info = cpuinfo.get_cpu_info()

    # Parse arguments
    parser = argparse.ArgumentParser()
    parser.add_argument('folder')
    args = parser.parse_args()
    in_dir = args.folder
    CPUFreq = float(info["hz_advertised"][0]/1000000000)

    # Read data
    time_files = sorted(glob.glob(in_dir + "/out*/time*"), reverse=True)
    time_files.sort(key=lambda x: os.path.getmtime(x))

    total = int(len(time_files))
    
    # parse data by num_stressor, and patter (black or random)
    time_all_black = {}
    time_all_random = {}
    for counter in range(total):

        curr_time_file = time_files[counter]
        curr_time_file_bw = int(float(curr_time_file.split("_")[3]))
        curr_time = parse_files(curr_time_file, CPUFreq)

        label = curr_time_file.split("/")[-2]
        selector = int(label.split("out-")[1])
        if(curr_time_file_bw == 0):
            time_all_black.setdefault(selector, []).extend(curr_time)
        else:
            time_all_random.setdefault(selector, []).extend(curr_time)
        
    plot_single(time_all_black, time_all_random, "memory-stressor", "Rendering time", "ms")

if __name__ == "__main__":
    main()
