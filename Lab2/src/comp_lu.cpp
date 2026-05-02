#include <mpi.h>

#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

struct Options {
    int n = 0;
    int repeats = 1;
    std::string pivot = "none";
};

struct PivotPosition {
    double value = -1.0;
    int row = 0;
    int col = 0;
};

std::size_t index_of(int row, int col, int n) {
    return static_cast<std::size_t>(row) * static_cast<std::size_t>(n) +
           static_cast<std::size_t>(col);
}

int owner(int row, int world_size) {
    return row % world_size;
}

double hilbert_value(int row, int col) {
    return 1.0 / static_cast<double>(row + col + 1);
}

void print_usage(const char* program) {
    std::cerr << "Usage: " << program
              << " --n <int> --pivot <none|row|column|global> --repeats <int>\n";
}

int parse_positive_int(const char* value, const std::string& name) {
    try {
        std::size_t parsed = 0;
        int result = std::stoi(value, &parsed);
        if (parsed != std::string(value).size() || result <= 0) {
            throw std::invalid_argument("not a positive integer");
        }
        return result;
    } catch (const std::exception&) {
        throw std::invalid_argument(name + " must be a positive integer");
    }
}

Options parse_options(int argc, char** argv) {
    Options options;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--n" && i + 1 < argc) {
            options.n = parse_positive_int(argv[++i], "--n");
        } else if (arg == "--pivot" && i + 1 < argc) {
            options.pivot = argv[++i];
        } else if (arg == "--repeats" && i + 1 < argc) {
            options.repeats = parse_positive_int(argv[++i], "--repeats");
        } else {
            throw std::invalid_argument("unknown or incomplete argument: " + arg);
        }
    }

    if (options.n <= 0) {
        throw std::invalid_argument("--n is required");
    }
    if (options.pivot != "none" &&
        options.pivot != "row" &&
        options.pivot != "column" &&
        options.pivot != "global") {
        throw std::invalid_argument("only --pivot none, --pivot row, --pivot column, and --pivot global are supported");
    }

    return options;
}

bool is_better_pivot(const PivotPosition& candidate, const PivotPosition& current) {
    if (candidate.value > current.value) {
        return true;
    }
    if (candidate.value < current.value) {
        return false;
    }
    if (candidate.row < current.row) {
        return true;
    }
    if (candidate.row > current.row) {
        return false;
    }
    return candidate.col < current.col;
}

void reduce_pivot_positions(void* invec, void* inoutvec, int* len, MPI_Datatype*) {
    auto* input = static_cast<PivotPosition*>(invec);
    auto* output = static_cast<PivotPosition*>(inoutvec);

    for (int i = 0; i < *len; ++i) {
        if (is_better_pivot(input[i], output[i])) {
            output[i] = input[i];
        }
    }
}

MPI_Datatype create_pivot_position_type() {
    PivotPosition sample;
    int block_lengths[3] = {1, 1, 1};
    MPI_Aint base = 0;
    MPI_Aint displacements[3] = {0, 0, 0};
    MPI_Datatype types[3] = {MPI_DOUBLE, MPI_INT, MPI_INT};
    MPI_Datatype datatype = MPI_DATATYPE_NULL;

    MPI_Get_address(&sample, &base);
    MPI_Get_address(&sample.value, &displacements[0]);
    MPI_Get_address(&sample.row, &displacements[1]);
    MPI_Get_address(&sample.col, &displacements[2]);

    for (MPI_Aint& displacement : displacements) {
        displacement -= base;
    }

    MPI_Type_create_struct(3, block_lengths, displacements, types, &datatype);
    MPI_Type_commit(&datatype);
    return datatype;
}

void fill_hilbert(std::vector<double>& matrix, int n) {
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            matrix[index_of(i, j, n)] = hilbert_value(i, j);
        }
    }
}

void initialize_permutation(std::vector<int>& permutation) {
    for (std::size_t i = 0; i < permutation.size(); ++i) {
        permutation[i] = static_cast<int>(i);
    }
}

void swap_columns(std::vector<double>& matrix, int n, int col_a, int col_b) {
    if (col_a == col_b) {
        return;
    }

    for (int i = 0; i < n; ++i) {
        std::swap(matrix[index_of(i, col_a, n)], matrix[index_of(i, col_b, n)]);
    }
}

void swap_owned_rows(
    std::vector<double>& matrix,
    int n,
    int row_a,
    int row_b,
    int rank,
    int world_size
) {
    if (row_a == row_b) {
        return;
    }

    const int owner_a = owner(row_a, world_size);
    const int owner_b = owner(row_b, world_size);

    if (owner_a == owner_b) {
        if (rank == owner_a) {
            for (int j = 0; j < n; ++j) {
                std::swap(matrix[index_of(row_a, j, n)], matrix[index_of(row_b, j, n)]);
            }
        }
        return;
    }

    std::vector<double> received_row(static_cast<std::size_t>(n), 0.0);

    if (rank == owner_a) {
        MPI_Sendrecv(
            &matrix[index_of(row_a, 0, n)],
            n,
            MPI_DOUBLE,
            owner_b,
            0,
            received_row.data(),
            n,
            MPI_DOUBLE,
            owner_b,
            0,
            MPI_COMM_WORLD,
            MPI_STATUS_IGNORE
        );

        for (int j = 0; j < n; ++j) {
            matrix[index_of(row_a, j, n)] = received_row[static_cast<std::size_t>(j)];
        }
    } else if (rank == owner_b) {
        MPI_Sendrecv(
            &matrix[index_of(row_b, 0, n)],
            n,
            MPI_DOUBLE,
            owner_a,
            0,
            received_row.data(),
            n,
            MPI_DOUBLE,
            owner_a,
            0,
            MPI_COMM_WORLD,
            MPI_STATUS_IGNORE
        );

        for (int j = 0; j < n; ++j) {
            matrix[index_of(row_b, j, n)] = received_row[static_cast<std::size_t>(j)];
        }
    }
}

int select_row_pivot_column(const std::vector<double>& lu, int n, int row) {
    int pivot_col = row;
    double best_abs = std::abs(lu[index_of(row, row, n)]);

    for (int j = row + 1; j < n; ++j) {
        const double candidate_abs = std::abs(lu[index_of(row, j, n)]);
        if (candidate_abs > best_abs) {
            best_abs = candidate_abs;
            pivot_col = j;
        }
    }

    return pivot_col;
}

int select_column_pivot_row(
    const std::vector<double>& lu,
    int n,
    int column,
    int rank,
    int world_size
) {
    struct PivotCandidate {
        double value;
        int row;
    };

    PivotCandidate local{-1.0, column};
    PivotCandidate global{-1.0, column};

    for (int i = column; i < n; ++i) {
        if (owner(i, world_size) != rank) {
            continue;
        }

        const double candidate = std::abs(lu[index_of(i, column, n)]);
        if (candidate > local.value) {
            local.value = candidate;
            local.row = i;
        }
    }

    MPI_Allreduce(&local, &global, 1, MPI_DOUBLE_INT, MPI_MAXLOC, MPI_COMM_WORLD);
    return global.row;
}

PivotPosition select_global_pivot_position(
    const std::vector<double>& lu,
    int n,
    int k,
    int rank,
    int world_size,
    MPI_Datatype pivot_position_type,
    MPI_Op pivot_position_op
) {
    PivotPosition local;
    local.value = -1.0;
    local.row = k;
    local.col = k;

    for (int i = k; i < n; ++i) {
        if (owner(i, world_size) != rank) {
            continue;
        }

        for (int j = k; j < n; ++j) {
            PivotPosition candidate;
            candidate.value = std::abs(lu[index_of(i, j, n)]);
            candidate.row = i;
            candidate.col = j;

            if (is_better_pivot(candidate, local)) {
                local = candidate;
            }
        }
    }

    PivotPosition global;
    MPI_Allreduce(
        &local,
        &global,
        1,
        pivot_position_type,
        pivot_position_op,
        MPI_COMM_WORLD
    );
    return global;
}

int lu_decompose(
    std::vector<double>& lu,
    std::vector<int>& row_permutation,
    std::vector<int>& column_permutation,
    int n,
    int rank,
    int world_size,
    const std::string& pivot_mode,
    MPI_Datatype pivot_position_type,
    MPI_Op pivot_position_op
) {
    for (int k = 0; k < n; ++k) {
        if (pivot_mode == "global") {
            const PivotPosition pivot =
                select_global_pivot_position(
                    lu,
                    n,
                    k,
                    rank,
                    world_size,
                    pivot_position_type,
                    pivot_position_op
                );

            if (pivot.row != k) {
                swap_owned_rows(lu, n, k, pivot.row, rank, world_size);
                std::swap(row_permutation[static_cast<std::size_t>(k)],
                          row_permutation[static_cast<std::size_t>(pivot.row)]);
            }

            if (pivot.col != k) {
                swap_columns(lu, n, k, pivot.col);
                std::swap(column_permutation[static_cast<std::size_t>(k)],
                          column_permutation[static_cast<std::size_t>(pivot.col)]);
            }
        } else if (pivot_mode == "column") {
            const int pivot_row = select_column_pivot_row(lu, n, k, rank, world_size);

            if (pivot_row != k) {
                swap_owned_rows(lu, n, k, pivot_row, rank, world_size);
                std::swap(row_permutation[static_cast<std::size_t>(k)],
                          row_permutation[static_cast<std::size_t>(pivot_row)]);
            }
        }

        const int pivot_owner = owner(k, world_size);
        double* pivot_row = &lu[index_of(k, 0, n)];

        MPI_Bcast(pivot_row, n, MPI_DOUBLE, pivot_owner, MPI_COMM_WORLD);

        int pivot_col = k;
        if (pivot_mode == "row") {
            if (rank == pivot_owner) {
                pivot_col = select_row_pivot_column(lu, n, k);
            }

            MPI_Bcast(&pivot_col, 1, MPI_INT, pivot_owner, MPI_COMM_WORLD);

            if (pivot_col != k) {
                swap_columns(lu, n, k, pivot_col);
                std::swap(column_permutation[static_cast<std::size_t>(k)],
                          column_permutation[static_cast<std::size_t>(pivot_col)]);
            }
        }

        const double pivot = lu[index_of(k, k, n)];
        if (pivot == 0.0 || !std::isfinite(pivot)) {
            return k + 1;
        }

        for (int i = k + 1; i < n; ++i) {
            if (owner(i, world_size) != rank) {
                continue;
            }

            const std::size_t ik = index_of(i, k, n);
            lu[ik] /= pivot;
            const double multiplier = lu[ik];

            for (int j = k + 1; j < n; ++j) {
                lu[index_of(i, j, n)] -= multiplier * lu[index_of(k, j, n)];
            }
        }
    }

    return 0;
}

double compute_relative_reconstruction_error(
    const std::vector<double>& lu,
    const std::vector<int>& row_permutation,
    const std::vector<int>& column_permutation,
    int n,
    int rank,
    int world_size
) {
    double local_diff_sq = 0.0;
    double local_a_sq = 0.0;

    for (int i = 0; i < n; ++i) {
        if (owner(i, world_size) != rank) {
            continue;
        }

        for (int j = 0; j < n; ++j) {
            double product = 0.0;
            const int limit = (i < j) ? i : j;

            for (int k = 0; k <= limit; ++k) {
                const double l_value =
                    (i == k) ? 1.0 : lu[index_of(i, k, n)];
                const double u_value = lu[index_of(k, j, n)];
                product += l_value * u_value;
            }

            const int original_row = row_permutation[static_cast<std::size_t>(i)];
            const int original_col = column_permutation[static_cast<std::size_t>(j)];
            const double original = hilbert_value(original_row, original_col);
            const double diff = original - product;
            local_diff_sq += diff * diff;
            const double unpermuted_original = hilbert_value(i, j);
            local_a_sq += unpermuted_original * unpermuted_original;
        }
    }

    double global_diff_sq = 0.0;
    double global_a_sq = 0.0;
    MPI_Reduce(&local_diff_sq, &global_diff_sq, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&local_a_sq, &global_a_sq, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank != 0) {
        return 0.0;
    }

    return std::sqrt(global_diff_sq) / std::sqrt(global_a_sq);
}

std::vector<double> build_rhs_for_ones(int n) {
    std::vector<double> rhs(static_cast<std::size_t>(n), 0.0);

    for (int i = 0; i < n; ++i) {
        double sum = 0.0;
        for (int j = 0; j < n; ++j) {
            sum += hilbert_value(i, j);
        }
        rhs[static_cast<std::size_t>(i)] = sum;
    }

    return rhs;
}

std::vector<double> solve_lu_system(const std::vector<double>& lu, const std::vector<double>& rhs, int n) {
    std::vector<double> y(static_cast<std::size_t>(n), 0.0);
    std::vector<double> x(static_cast<std::size_t>(n), 0.0);

    for (int i = 0; i < n; ++i) {
        double sum = rhs[static_cast<std::size_t>(i)];
        for (int j = 0; j < i; ++j) {
            sum -= lu[index_of(i, j, n)] * y[static_cast<std::size_t>(j)];
        }
        y[static_cast<std::size_t>(i)] = sum;
    }

    for (int i = n - 1; i >= 0; --i) {
        double sum = y[static_cast<std::size_t>(i)];
        for (int j = i + 1; j < n; ++j) {
            sum -= lu[index_of(i, j, n)] * x[static_cast<std::size_t>(j)];
        }
        x[static_cast<std::size_t>(i)] = sum / lu[index_of(i, i, n)];
    }

    return x;
}

double compute_relative_residual(
    const std::vector<double>& lu,
    const std::vector<int>& row_permutation,
    const std::vector<int>& column_permutation,
    int n,
    int rank,
    int world_size
) {
    const std::vector<double> rhs = build_rhs_for_ones(n);
    std::vector<double> permuted_rhs(static_cast<std::size_t>(n), 0.0);

    for (int i = 0; i < n; ++i) {
        const int original_row = row_permutation[static_cast<std::size_t>(i)];
        permuted_rhs[static_cast<std::size_t>(i)] = rhs[static_cast<std::size_t>(original_row)];
    }

    const std::vector<double> pivoted_solution = solve_lu_system(lu, permuted_rhs, n);
    std::vector<double> x(static_cast<std::size_t>(n), 0.0);

    for (int j = 0; j < n; ++j) {
        const int original_col = column_permutation[static_cast<std::size_t>(j)];
        x[static_cast<std::size_t>(original_col)] =
            pivoted_solution[static_cast<std::size_t>(j)];
    }

    double local_residual_sq = 0.0;
    double local_rhs_sq = 0.0;

    for (int i = 0; i < n; ++i) {
        if (owner(i, world_size) != rank) {
            continue;
        }

        double ax = 0.0;
        for (int j = 0; j < n; ++j) {
            ax += hilbert_value(i, j) * x[static_cast<std::size_t>(j)];
        }

        const double diff = ax - rhs[static_cast<std::size_t>(i)];
        local_residual_sq += diff * diff;
        local_rhs_sq += rhs[static_cast<std::size_t>(i)] * rhs[static_cast<std::size_t>(i)];
    }

    double global_residual_sq = 0.0;
    double global_rhs_sq = 0.0;
    MPI_Reduce(&local_residual_sq, &global_residual_sq, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&local_rhs_sq, &global_rhs_sq, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank != 0) {
        return 0.0;
    }

    return std::sqrt(global_residual_sq) / std::sqrt(global_rhs_sq);
}

}  // namespace

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank = 0;
    int world_size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    Options options;
    try {
        options = parse_options(argc, argv);
    } catch (const std::exception& error) {
        if (rank == 0) {
            std::cerr << "Argument error: " << error.what() << '\n';
            print_usage(argv[0]);
        }
        MPI_Finalize();
        return 1;
    }

    MPI_Datatype pivot_position_type = MPI_DATATYPE_NULL;
    MPI_Op pivot_position_op = MPI_OP_NULL;
    if (options.pivot == "global") {
        pivot_position_type = create_pivot_position_type();
        MPI_Op_create(&reduce_pivot_positions, 1, &pivot_position_op);
    }

    std::vector<double> lu(static_cast<std::size_t>(options.n) * static_cast<std::size_t>(options.n));
    std::vector<int> row_permutation(static_cast<std::size_t>(options.n));
    std::vector<int> column_permutation(static_cast<std::size_t>(options.n));

    for (int repeat = 1; repeat <= options.repeats; ++repeat) {
        fill_hilbert(lu, options.n);
        initialize_permutation(row_permutation);
        initialize_permutation(column_permutation);

        MPI_Barrier(MPI_COMM_WORLD);
        const double start = MPI_Wtime();
        const int local_info =
            lu_decompose(
                lu,
                row_permutation,
                column_permutation,
                options.n,
                rank,
                world_size,
                options.pivot,
                pivot_position_type,
                pivot_position_op
            );
        const double elapsed = MPI_Wtime() - start;

        int global_info = 0;
        double time_seconds = 0.0;
        MPI_Allreduce(&local_info, &global_info, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
        MPI_Reduce(&elapsed, &time_seconds, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

        double reconstruction_error = std::numeric_limits<double>::quiet_NaN();
        double residual = std::numeric_limits<double>::quiet_NaN();

        if (global_info == 0) {
            reconstruction_error =
                compute_relative_reconstruction_error(
                    lu,
                    row_permutation,
                    column_permutation,
                    options.n,
                    rank,
                    world_size
                );
            residual =
                compute_relative_residual(
                    lu,
                    row_permutation,
                    column_permutation,
                    options.n,
                    rank,
                    world_size
                );
        }

        if (rank == 0) {
            const char* algorithm = "lu_hilbert_no_pivot";
            if (options.pivot == "row") {
                algorithm = "lu_hilbert_row_pivot";
            } else if (options.pivot == "column") {
                algorithm = "lu_hilbert_column_pivot";
            } else if (options.pivot == "global") {
                algorithm = "lu_hilbert_global_pivot";
            }

            std::cout << std::setprecision(17)
                      << algorithm << ','
                      << options.n << ','
                      << world_size << ','
                      << repeat << ','
                      << time_seconds << ','
                      << reconstruction_error << ','
                      << residual << '\n';
        }
    }

    if (pivot_position_op != MPI_OP_NULL) {
        MPI_Op_free(&pivot_position_op);
    }
    if (pivot_position_type != MPI_DATATYPE_NULL) {
        MPI_Type_free(&pivot_position_type);
    }

    MPI_Finalize();
    return 0;
}
