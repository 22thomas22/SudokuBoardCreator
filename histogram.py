import csv
import os
import matplotlib.pyplot as plt

RESULTS_DIR = "./benchmark_results"
OUTPUT_DIR = os.path.join(RESULTS_DIR, "plots")

def load_histogram(path):
    bin_starts, bin_ends, counts = [], [], []
    with open(path, newline="") as f:
        for row in csv.reader(f):
            bin_starts.append(float(row[0]))
            bin_ends.append(float(row[1]))
            counts.append(int(row[2]))
    return bin_starts, bin_ends, counts
def plot_one(csv_path, out_dir):
    bin_starts, bin_ends, counts = load_histogram(csv_path)
    widths = [e - s for s,e in zip(bin_starts, bin_ends)]

    plt.figure()
    plt.bar(bin_starts, counts, width=widths, align="edge")
    plt.yscale("log")
    name = os.path.splitext(os.path.basename(csv_path))[0]
    plt.title(name)

    os.makedirs(out_dir, exist_ok=True)
    out_path = os.path.join(out_dir, name + ".png")
    plt.savefig(out_path, dpi=150)
    print(f"saved {out_path}")

def main():
    csv_files = [f for f in os.listdir(RESULTS_DIR) if f.endswith(".csv")]
    if not csv_files:
        print(f"no CSV files found in {RESULTS_DIR}.")
        return
    for filename in csv_files:
        plot_one(os.path.join(RESULTS_DIR, filename), OUTPUT_DIR)

if __name__ == "__main__":
    main()