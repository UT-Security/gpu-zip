import warnings
warnings.simplefilter(action='ignore', category=FutureWarning)

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import os
import numpy as np
import glob
import argparse
from distutils.dir_util import remove_tree


def parse_file(fn):
    with open(fn, 'r') as in_file: 
        in_data = in_file.read().splitlines()

    readings = np.array([float(x)/1000 for x in in_data])
    samples_filtered = []
    samples_low = np.percentile(readings, 5)
    samples_high = np.percentile(readings, 95)
    for sample in readings:
        if ((sample >= samples_low) and (sample <= samples_high)):
            samples_filtered.append(sample)
    return np.mean(samples_filtered),np.std(samples_filtered)


def plot(myDict):	
    labels = []
    time_0 = []
    time_1 = []
    time_0_std = []
    time_1_std = []
    for x in myDict:
        labels.append(float(x*x*4/1024/1024))
        time_0.append(myDict[x][0][0])
        time_1.append(myDict[x][1][0])
        time_0_std.append(myDict[x][0][1])
        time_1_std.append(myDict[x][1][1])
    
    fig, a1 = plt.subplots(1, 1, figsize=(3, 2))
    plt.xlabel('Texture size (MiB)', fontsize = 8)
    plt.ylabel('LLC walk time (ms)', fontsize = 8)

    plt.scatter(labels, time_0, s=2, c = "navy", label="Compressible")
    markers, caps, bars = plt.errorbar(labels, time_0, c = "navy",  yerr=time_0_std, fmt=".")
    [bar.set_alpha(0.5) for bar in bars]
    [cap.set_alpha(0.5) for cap in caps]

    plt.scatter(labels, time_1, s=2, c = "darkorange", label="Non-Compressible")
    markers, caps, bars = plt.errorbar(labels, time_1, c = "darkorange", yerr=time_1_std, fmt=".")
    [bar.set_alpha(0.5) for bar in bars]
    [cap.set_alpha(0.5) for cap in caps]
    
    plt.gca().xaxis.set_major_locator(ticker.MultipleLocator(4))

    plt.legend(loc='upper center', bbox_to_anchor=(0.42, 1.35), ncol=2, markerscale=4, fontsize=8)
    plt.subplots_adjust(left=0.16,top=0.80,right=0.98,bottom=0.2)

    plt.savefig("./plot/llc_size.pdf", dpi=300)


def main():
    # Prepare output directory
    try:
        remove_tree('plot')
    except:
        pass
    out_dir = 'plot'
    try:
        os.makedirs(out_dir)
    except:
        pass

    parser = argparse.ArgumentParser()
    parser.add_argument('folder')

    args = parser.parse_args()
    data_folder = args.folder
    files = sorted(glob.glob(data_folder + "/*"), reverse=True)

    size_dict = {}
    for f in files:
        size = int(f.split(".txt")[0].split("_")[-1])
        bw = int(f.split("w")[1].split("_")[0])
        time, time_std = parse_file(f)
        if(size not in size_dict):
            size_dict[size] = {}
            size_dict[size][bw] = (time, time_std)
        else:
            size_dict[size][bw] = (time, time_std)

    # Plot LLC walk time vs texture size for compressible and non-compressible textures.
    plot(size_dict)

if __name__ == "__main__":
    main()
