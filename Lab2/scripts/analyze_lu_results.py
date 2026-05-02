import argparse
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import pandas as pd


REQUIRED_COLUMNS = {
    "algorithm",
    "n",
    "procs",
    "repeat",
    "time_seconds",
    "reconstruction_error",
    "residual",
}

ALGORITHM_LABELS = {
    "lu_hilbert_no_pivot": "Без выбора ведущего элемента",
    "lu_hilbert_row_pivot": "Выбор по строке",
    "lu_hilbert_column_pivot": "Выбор по столбцу",
    "lu_hilbert_global_pivot": "Глобальный выбор",
}


def algorithm_label(algorithm: str) -> str:
    return ALGORITHM_LABELS.get(algorithm, algorithm)


def safe_name(value: str) -> str:
    return value.replace("lu_hilbert_", "").replace("_pivot", "").replace("_", "-")


def load_results(csv_path: Path) -> pd.DataFrame:
    if not csv_path.exists():
        raise FileNotFoundError(f"CSV file was not found: {csv_path}")

    data = pd.read_csv(csv_path)
    missing = REQUIRED_COLUMNS - set(data.columns)
    if missing:
        missing_text = ", ".join(sorted(missing))
        raise ValueError(f"{csv_path} does not contain required columns: {missing_text}")

    for column in ["n", "procs", "repeat", "time_seconds", "reconstruction_error", "residual"]:
        data[column] = pd.to_numeric(data[column], errors="raise")

    return data


def aggregate_results(data: pd.DataFrame) -> pd.DataFrame:
    aggregated = (
        data.groupby(["algorithm", "n", "procs"], as_index=False)
        .agg(
            time_seconds=("time_seconds", "median"),
            reconstruction_error=("reconstruction_error", "median"),
            residual=("residual", "median"),
            repeats=("repeat", "count"),
        )
        .sort_values(["algorithm", "n", "procs"])
    )

    base_times = (
        aggregated[aggregated["procs"] == 1][["algorithm", "n", "time_seconds"]]
        .rename(columns={"time_seconds": "time_1"})
    )

    aggregated = aggregated.merge(base_times, on=["algorithm", "n"], how="left")
    aggregated["speedup"] = aggregated["time_1"] / aggregated["time_seconds"]
    aggregated["efficiency"] = aggregated["speedup"] / aggregated["procs"]
    aggregated = aggregated.drop(columns=["time_1"])

    return aggregated


def configure_axes(title: str, xlabel: str, ylabel: str, log_x: bool = False, log_y: bool = False) -> None:
    plt.title(title)
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    plt.grid(True, which="both", linestyle="--", linewidth=0.6, alpha=0.6)
    if log_x:
        plt.xscale("log")
    if log_y:
        plt.yscale("log")
    plt.tight_layout()


def save_current_plot(output_path: Path) -> None:
    output_path.parent.mkdir(parents=True, exist_ok=True)
    plt.savefig(output_path, bbox_inches="tight")
    plt.close()


def plot_runtime_vs_n(aggregated: pd.DataFrame, plots_dir: Path) -> None:
    for algorithm, group in aggregated.groupby("algorithm"):
        plt.figure(figsize=(10, 6), dpi=160)

        for procs, procs_group in group.groupby("procs"):
            procs_group = procs_group.sort_values("n")
            plt.plot(
                procs_group["n"],
                procs_group["time_seconds"],
                marker="o",
                linewidth=2,
                label=f"{procs} проц.",
            )

        plt.legend(title="Число процессов")
        configure_axes(
            title=f"Время выполнения от размера задачи: {algorithm_label(algorithm)}",
            xlabel="Размер матрицы n",
            ylabel="Время выполнения, с",
            log_x=True,
            log_y=True,
        )
        save_current_plot(plots_dir / f"runtime_vs_n_{safe_name(algorithm)}.png")


def plot_speedup_vs_n(aggregated: pd.DataFrame, plots_dir: Path) -> None:
    for algorithm, group in aggregated.groupby("algorithm"):
        plt.figure(figsize=(10, 6), dpi=160)

        for procs, procs_group in group.groupby("procs"):
            procs_group = procs_group.sort_values("n")
            plt.plot(
                procs_group["n"],
                procs_group["speedup"],
                marker="o",
                linewidth=2,
                label=f"{procs} проц.",
            )

        plt.legend(title="Число процессов")
        configure_axes(
            title=f"Ускорение от размера задачи: {algorithm_label(algorithm)}",
            xlabel="Размер матрицы n",
            ylabel="Ускорение T1 / Tp",
            log_x=True,
        )
        save_current_plot(plots_dir / f"speedup_vs_n_{safe_name(algorithm)}.png")


def plot_runtime_vs_procs(aggregated: pd.DataFrame, plots_dir: Path) -> None:
    for algorithm, group in aggregated.groupby("algorithm"):
        plt.figure(figsize=(10, 6), dpi=160)

        for n_value, n_group in group.groupby("n"):
            n_group = n_group.sort_values("procs")
            plt.plot(
                n_group["procs"],
                n_group["time_seconds"],
                marker="o",
                linewidth=2,
                label=f"n={n_value}",
            )

        plt.legend(title="Размер матрицы")
        configure_axes(
            title=f"Время выполнения от числа процессов: {algorithm_label(algorithm)}",
            xlabel="Число процессов",
            ylabel="Время выполнения, с",
            log_y=True,
        )
        plt.xticks(sorted(group["procs"].unique()))
        save_current_plot(plots_dir / f"runtime_vs_procs_{safe_name(algorithm)}.png")


def plot_quality_metric(
    aggregated: pd.DataFrame,
    plots_dir: Path,
    metric: str,
    output_name: str,
    title: str,
    ylabel: str,
) -> None:
    plt.figure(figsize=(10, 6), dpi=160)

    metric_by_algorithm = (
        aggregated.groupby(["algorithm", "n"], as_index=False)[metric]
        .median()
        .sort_values(["algorithm", "n"])
    )

    for algorithm, group in metric_by_algorithm.groupby("algorithm"):
        plt.plot(
            group["n"],
            group[metric],
            marker="o",
            linewidth=2,
            label=algorithm_label(algorithm),
        )

    plt.legend(title="Алгоритм")
    configure_axes(
        title=title,
        xlabel="Размер матрицы n",
        ylabel=ylabel,
        log_x=True,
        log_y=True,
    )
    save_current_plot(plots_dir / output_name)


def build_plots(aggregated: pd.DataFrame, plots_dir: Path) -> None:
    plot_runtime_vs_n(aggregated, plots_dir)
    plot_speedup_vs_n(aggregated, plots_dir)
    plot_runtime_vs_procs(aggregated, plots_dir)
    plot_quality_metric(
        aggregated,
        plots_dir,
        metric="reconstruction_error",
        output_name="reconstruction_error_vs_n.png",
        title="Ошибка восстановления LU-разложения",
        ylabel="||P A Q - L U||F / ||A||F",
    )
    plot_quality_metric(
        aggregated,
        plots_dir,
        metric="residual",
        output_name="residual_vs_n.png",
        title="Относительная невязка решения системы",
        ylabel="||A x - b||2 / ||b||2",
    )


def main() -> None:
    script_dir = Path(__file__).resolve().parent
    lab_dir = script_dir.parent

    parser = argparse.ArgumentParser(description="Analyze Lab2 LU MPI benchmark results.")
    parser.add_argument(
        "--input",
        type=Path,
        default=lab_dir / "results" / "results_lu.csv",
        help="Path to results_lu.csv.",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=lab_dir / "results" / "results_lu_agg.csv",
        help="Path to save aggregated CSV.",
    )
    parser.add_argument(
        "--plots-dir",
        type=Path,
        default=lab_dir / "results" / "plots",
        help="Directory for generated PNG plots.",
    )

    args = parser.parse_args()

    data = load_results(args.input)
    aggregated = aggregate_results(data)

    args.output.parent.mkdir(parents=True, exist_ok=True)
    aggregated.to_csv(args.output, index=False)
    build_plots(aggregated, args.plots_dir)

    print(f"Saved aggregated table to {args.output}")
    print(f"Saved plots to {args.plots_dir}")


if __name__ == "__main__":
    main()
