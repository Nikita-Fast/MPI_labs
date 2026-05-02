import argparse
import csv
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt


def read_results(csv_path: Path) -> dict[int, list[tuple[int, float]]]:
    results: dict[int, list[tuple[int, float]]] = {}

    with csv_path.open(newline="", encoding="utf-8") as file:
        reader = csv.DictReader(file)
        required_columns = {"n", "procs", "time_seconds"}
        missing_columns = required_columns - set(reader.fieldnames or [])
        if missing_columns:
            columns = ", ".join(sorted(missing_columns))
            raise ValueError(f"{csv_path} does not contain required columns: {columns}")

        for row in reader:
            n = int(row["n"])
            procs = int(row["procs"])
            time_seconds = float(row["time_seconds"])
            results.setdefault(procs, []).append((n, time_seconds))

    for points in results.values():
        points.sort(key=lambda item: item[0])

    return dict(sorted(results.items()))


def plot_runtime(
    csv_path: Path,
    output_path: Path,
    title: str,
    log_x: bool,
    log_y: bool,
) -> None:
    results = read_results(csv_path)

    output_path.parent.mkdir(parents=True, exist_ok=True)

    plt.figure(figsize=(10, 6), dpi=160)

    for procs, points in results.items():
        n_values = [n for n, _ in points]
        time_values = [time for _, time in points]
        plt.plot(n_values, time_values, marker="o", linewidth=2, label=f"{procs} proc.")

    plt.title(title)
    plt.xlabel("Task size n")
    plt.ylabel("Execution time, seconds")
    plt.grid(True, which="both", linestyle="--", linewidth=0.6, alpha=0.6)
    plt.legend(title="MPI processes")

    if log_x:
        plt.xscale("log")
    if log_y:
        plt.yscale("log")

    plt.tight_layout()
    plt.savefig(output_path)
    plt.close()


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Plot MPI execution time versus task size for each process count."
    )
    parser.add_argument("csv", type=Path, help="Path to CSV file with n, procs, time_seconds columns.")
    parser.add_argument("-o", "--output", type=Path, required=True, help="Output image path.")
    parser.add_argument("--title", default="MPI execution time", help="Plot title.")
    parser.add_argument("--linear-x", action="store_true", help="Use linear scale for n.")
    parser.add_argument("--linear-y", action="store_true", help="Use linear scale for execution time.")

    args = parser.parse_args()

    plot_runtime(
        csv_path=args.csv,
        output_path=args.output,
        title=args.title,
        log_x=not args.linear_x,
        log_y=not args.linear_y,
    )


if __name__ == "__main__":
    main()
