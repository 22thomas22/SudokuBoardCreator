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

def histogram_stats(bin_starts, bin_ends, counts):
    total_count = sum(counts)
    if total_count == 0:
        return None

    #weighted mean
    mean = 0.0
    for start, end, count in zip(bin_starts, bin_ends, counts):
        midpoint = (start + end) / 2
        mean += midpoint * count
    mean /= total_count

    # weighted variance
    variance = 0.0
    for start, end, count in zip(bin_starts, bin_ends, counts):
        midpoint = (start + end) / 2
        variance += count * (midpoint - mean) ** 2
    variance /= (total_count - 1)
    stddev = variance ** 0.5

    # min/max
    nonzero = [i for i,c in enumerate(counts) if c > 0]
    approx_min = bin_starts[nonzero[0]] if nonzero else None
    approx_max = bin_ends[nonzero[-1]] if nonzero else None

    return {
        "mean": mean,
        "stddev": stddev,
        "min": approx_min,
        "max": approx_max,
        "count": total_count
    }

def plot_one(csv_path, out_dir):
    bin_starts, bin_ends, counts = load_histogram(csv_path)
    widths = [e - s for s,e in zip(bin_starts, bin_ends)]

    stats = histogram_stats(bin_starts, bin_ends, counts)
    name = os.path.splitext(os.path.basename(csv_path))[0]

    print(f"--- {name} ---")
    print(f"  count: {stats['count']:,}")
    print(f"  mean:  {stats['mean']:.6f}")
    print(f"  stddev:{stats['stddev']:.6f}")
    print(f"  min:   {stats['min']:.6f}")
    print(f"  max:   {stats['max']:.6f}")

    plt.figure()
    plt.bar(bin_starts, counts, width=widths, align="edge")
    plt.yscale("log")
    name = os.path.splitext(os.path.basename(csv_path))[0]
    plt.title(f"{name}\nmean={stats['mean']:.5f}  stddev={stats['stddev']:.5f}")

    os.makedirs(out_dir, exist_ok=True)
    out_path = os.path.join(out_dir, name + ".png")
    plt.savefig(out_path, dpi=300)
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