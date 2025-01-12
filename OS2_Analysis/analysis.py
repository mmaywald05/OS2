import matplotlib.pyplot as plt
import numpy as np
from collections import Counter
import scipy.stats as stats
def analyze_data(file_name, experiment_name):
    # Read the file and extract data values
    indices = []
    data = []
    with open(file_name, 'r') as file:
        for line in file:
            index, value = line.strip().split(':')
            indices.append(int(index))  # Store the indices
            data.append(float(value))  # Store the data values

    # data indicates round trip duration for ipc mechanisms
    data = [x/2 for x in data]


    data, indices = remove_outliers_iqr(data, indices)



    m, lowerBound, upperBound = mean_confidence_interval(data)

    # Count occurrences of each data value using a Counter
    data_count = Counter(data)

    # Prepare the histogram data
    x_values = sorted(data_count.keys())  # Sorted keys for the x-axis
    y_values = [data_count[x] for x in x_values]  # Counts for the y-axis

    # Plot the histogram
    plt.figure(figsize=(12, 6))
    #plt.subplot(2, 1, 1)  # First subplot for the histogram
    plt.bar(x_values, y_values, width=0.5, color='skyblue', edgecolor='black', alpha=0.7)
    plt.title(experiment_name+': Histogram of times (Filtered)')
    plt.xlabel('IPC mechanism duration (nanoseconds)')
    plt.ylabel('Occurrences')


    # Add a legend
    plt.legend()
    """
    # Plot the line diagram for the original data
    plt.subplot(2, 1, 2)  # Second subplot for the line diagram
    plt.plot(indices, data, color='blue', marker='o', markersize=2, linewidth=1, alpha=0.8, label='Original Data')
    plt.title(experiment_name+ ': IPC mechanism durations raw data (original order)')
    plt.xlabel('iteration index')
    plt.ylabel('time (nanoseconds)')
    
    # Add a legend
    plt.legend()

    # Adjust layout to prevent overlap
    plt.tight_layout()
    """
    plt.savefig(f"{experiment_name}.png", dpi=300, bbox_inches="tight")

    # Display the plot
    plt.show()
    # Print the percentiles to the console
    print("##### "+ experiment_name +" #####")
    print("Confidence Interval [" + str(lowerBound) + " ... " + str(m)  +" ... " +  str(upperBound) + "]")
    print("####################################")



def mean_confidence_interval(data, confidence=0.95):
    a = 1.0 * np.array(data)
    n = len(a)
    m, se = np.mean(a), stats.sem(a)
    h = se * stats.t.ppf((1 + confidence) / 2., n-1)
    return m, m-h, m+h

def remove_outliers_iqr(data, indices):

        """
        Removes outliers based on the IQR method, ensuring both data and indices arrays are updated.

        Args:
            data (list): List of numerical data values.
            indices (list): Corresponding indices for each data point.

        Returns:
            tuple: Filtered data and indices as two separate lists.
        """
        # Convert to NumPy arrays for easier calculations
        data = np.array(data)
        indices = np.array(indices)

        # Calculate Q1, Q3, and IQR
        Q1 = np.percentile(data, 25)
        Q3 = np.percentile(data, 75)
        IQR = Q3 - Q1

        # Calculate lower and upper bounds
        lower_bound = Q1 - 1.5 * IQR
        upper_bound = Q3 + 1.5 * IQR

        # Create a mask for valid (non-outlier) data points
        valid_mask = (data >= lower_bound) & (data <= upper_bound)

        # Filter data and indices
        filtered_data = data[valid_mask]
        filtered_indices = indices[valid_mask]

        return filtered_data.tolist(), filtered_indices.tolist()


if __name__ == '__main__':
    f_spinlock_intraproc = 'data/spinlock_intraproc.txt'  # Update this to your actual file path
    f_sem_intraproc = 'data/sem_intraproc.txt'
    f_zmq_intraproc = 'data/zmq_intraproc.txt'
    f_zmq_interproc = 'data/zmq_interproc.txt'
    f_docker = 'data/docker.txt'
    f_better_spin = 'data/better_spinlock.txt'
    analyze_data(f_spinlock_intraproc, "Spinlock Intraprocess")
    analyze_data(f_better_spin, "Better Spinlock")
    analyze_data(f_sem_intraproc, "Semaphores Intraprocess")
    analyze_data(f_zmq_intraproc, "ZMQ Intraprocess")
    analyze_data(f_zmq_interproc, "ZMQ Interprocess")
    analyze_data(f_docker, "Docker Containers")