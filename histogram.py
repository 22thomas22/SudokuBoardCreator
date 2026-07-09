import csv
import os
import matplotlib.pyplot as plt

def find_histogram_files(start_dir="."):
    found = []
    for root, dirs, files in os.walk(start_dir):
        for filename in files:
            if filename == "histogram.csv":
                found.append(os.path.join(root, filename))
    return found

def load_histogram(path):
    bin_starts, bin_ends, counts = [], [], []
    with open(path, newline="") as f:
        for row in csv.reader(f):
            bin_starts.append(float(row[0]))
            bin_ends.append(float(row[1]))
            counts.append(int(row[2]))
    return bin_starts, bin_ends, counts

def plot_one(path, bin_starts, bin_ends, counts):
    widths = [end - start for start, end in zip(bin_starts, bin_ends)]

    plt.figure()
    plt.bar(bin_starts, counts, width=widths, align="edge")
    plt.yscale("log")
    plt.xlabel("value")
    plt.ylabel("count (log scale)")
    plt.title(path)

    out_path = path.replace("histogram.csv", "histogram.png")
    plt.savefig(out_path, dpi=150)
    print(f"saved {out_path}")

def main():
    files = find_histogram_files(".")
    if not files:
        print("no histogram.csv files found")
        return
    print(f"found {len(files)} histogram file(s).")
    for f in files:
        print(f" {f}")
    for path in files:
        bin_starts, bin_ends, counts = load_histogram(path)
        plot_one(path, bin_starts, bin_ends, counts)
if __name__ == "__main__":
    main()