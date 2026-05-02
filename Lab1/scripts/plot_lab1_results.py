from pathlib import Path

from plot_runtime import plot_runtime


LAB_DIR = Path(__file__).resolve().parent.parent
RESULTS_DIR = LAB_DIR / "results"
PLOTS_DIR = RESULTS_DIR / "plots"


def main() -> None:
    plot_runtime(
        csv_path=RESULTS_DIR / "results_pi.csv",
        output_path=PLOTS_DIR / "runtime_pi.png",
        title="Pi computation: execution time vs task size",
        log_x=True,
        log_y=True,
    )

    plot_runtime(
        csv_path=RESULTS_DIR / "results_primes.csv",
        output_path=PLOTS_DIR / "runtime_primes.png",
        title="Prime counting: execution time vs task size",
        log_x=True,
        log_y=True,
    )

    print(f"Saved plots to {PLOTS_DIR}")


if __name__ == "__main__":
    main()
