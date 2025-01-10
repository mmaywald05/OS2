import matplotlib.pyplot as plt

def read_and_plot_histogram(filename):
    try:
        # Read the file and parse its contents
        data = {}
        with open(filename, 'r') as file:
            for line in file:
                line = line.strip()
                if ':' in line:
                    key, value = line.split(':')
                    data[float(key)] = int(value)

        # Prepare data for the histogram
        keys = list(data.keys())
        values = list(data.values())

        # Plot the histogram
        plt.figure(figsize=(10, 6))
        plt.bar(keys, values, width=0.01, color='skyblue', edgecolor='black')
        plt.xlabel('Double Values')
        plt.ylabel('Counts (Int)')
        plt.title('Histogram of Double:Int Values')
        plt.grid(axis='y', linestyle='--', alpha=0.7)
        plt.tight_layout()
        plt.show()

    except FileNotFoundError:
        print(f"Error: The file '{filename}' was not found.")
    except ValueError as e:
        print(f"Error parsing the file: {e}")
    except Exception as e:
        print(f"An unexpected error occurred: {e}")

# Example usage
if __name__ == "__main__":
    filename = input("Enter the file name: ")
    read_and_plot_histogram(filename)
