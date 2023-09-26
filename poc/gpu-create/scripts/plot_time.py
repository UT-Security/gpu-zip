import warnings
warnings.simplefilter(action='ignore', category=FutureWarning)

import numpy as np
import matplotlib.pyplot as plt
import os
import numpy as np
import argparse


# -------------------------------------------------------------------------------------------------------------------
# Parsing Functions
# -------------------------------------------------------------------------------------------------------------------

def parse_file(fn):
    readings = []
    with open(fn) as f:
        for line in f:
            begin = int(line)
            end = 0
            try:
                end = next(f)
            except StopIteration:    
                break
            if(len(end) > 1):
                readings.append(int(end)-begin)
    
    return readings



# -------------------------------------------------------------------------------------------------------------------

# for a fixed cpu method, plot different gpu workload
def plot(myDict):	
    # Prepare plot
    minimum = 10000000000
    maximum = 0
    datas = []
    labels = []
    weights = []

    # Parse data
    for label, trace in myDict.items():
        # Exclude negative samples (due to counter overflow)
        samples_positive = []
        for i, x in enumerate(trace):
            if x > 0:
                samples_positive.append(x)

        # Filter outliers (for the plot)
        samples_filtered = []
        samples_mean = np.mean(samples_positive)
        samples_std = np.std(samples_positive)
        for sample in samples_positive:
            if (abs(sample-samples_mean) < 4*samples_std):
                samples_filtered.append(sample)

        # Store data for bins
        minimum = min(round(min(samples_filtered), 1), minimum)
        maximum = max(round(max(samples_filtered), 1), maximum)

        # Store data for bars
        datas.append(samples_filtered)
        labels.append(label)
        weights.append(np.ones_like(samples_filtered)/float(len(samples_filtered)))
            
        # Print mean
        print("%15s rendering time: %2f +- %d ms (min %d max %d)" % (label, np.mean(samples_filtered), np.std(samples_filtered), min(samples_filtered), max(samples_filtered)))
	    
    # Plot all data
    plt.figure() 
    step = (maximum-minimum)/20
    bins = np.arange(minimum-2, maximum + 2, step)

    _, bins, _ = plt.hist(datas, alpha=0.5, bins=bins, weights=weights, label=labels, align="left")

	# Show grid
    plt.grid(axis='y', alpha=0.75)
	# Set labels
	# plt.gca().xaxis.get_major_locator().set_params(integer=True)
    plt.xlabel('Rendering time (CPU cycles)', fontsize=10)
    plt.ylabel('Probability', fontsize=10)

	# Save plot to file
    plt.legend(loc='upper right', fontsize=10)
    plt.tight_layout(pad=0.1)
    plt.savefig("./plot/time.pdf", dpi=300)
    plt.clf()





def main():
    global minimum
    global maximum
    # Prepare clean output directory
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
    parser.add_argument('file1')
    parser.add_argument('file2')

    args = parser.parse_args()
    file1 = args.file1
    file2 = args.file2

    # For histograms
    llc_label_dict = {}
    llc_label_dict["Compressible"] = parse_file(file1)
    llc_label_dict["Non-compressible"] = parse_file(file2)


    plot(llc_label_dict)

    

if __name__ == "__main__":
    main()
