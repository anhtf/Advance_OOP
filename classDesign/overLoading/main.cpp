#include <iostream>
#include <vector>
#include <stdexcept>
#include <utility>
#include <iomanip>
#include <chrono>

class MathMatrix {
private:
    size_t rows_;
    size_t cols_;
    std::vector<double> data_;

public:
    MathMatrix(size_t rows, size_t cols) 
        : rows_(rows), cols_(cols), data_(rows * cols, 0.0) {
        if (rows == 0 || cols == 0) {
            throw std::invalid_argument("Matrix dimensions must be strictly positive");
        }
    }

    MathMatrix(const MathMatrix& other) = default;
    MathMatrix& operator=(const MathMatrix& other) = default;

    MathMatrix(MathMatrix&& other) noexcept 
        : rows_(std::exchange(other.rows_, 0)),
          cols_(std::exchange(other.cols_, 0)),
          data_(std::move(other.data_)) {}

    MathMatrix& operator=(MathMatrix&& other) noexcept {
        if (this != &other) {
            rows_ = std::exchange(other.rows_, 0);
            cols_ = std::exchange(other.cols_, 0);
            data_ = std::move(other.data_);
        }
        return *this;
    }

    double& operator()(size_t row, size_t col) {
        if (row >= rows_ || col >= cols_) {
            throw std::out_of_range("Matrix subscript out of bounds");
        }
        return data_[row * cols_ + col];
    }

    const double& operator()(size_t row, size_t col) const {
        if (row >= rows_ || col >= cols_) {
            throw std::out_of_range("Matrix subscript out of bounds");
        }
        return data_[row * cols_ + col];
    }

    MathMatrix operator+(const MathMatrix& rhs) const {
        if (rows_ != rhs.rows_ || cols_ != rhs.cols_) {
            throw std::invalid_argument("Matrix dimensions must match for addition");
        }
        MathMatrix result(rows_, cols_);
        for (size_t i = 0; i < data_.size(); ++i) {
            result.data_[i] = data_[i] + rhs.data_[i];
        }
        return result;
    }

    MathMatrix operator*(const MathMatrix& rhs) const {
        if (cols_ != rhs.rows_) {
            throw std::invalid_argument("Matrix inner dimensions must match for multiplication");
        }
        MathMatrix result(rows_, rhs.cols_);
        for (size_t i = 0; i < rows_; ++i) {
            for (size_t k = 0; k < cols_; ++k) {
                double temp = data_[i * cols_ + k];
                for (size_t j = 0; j < rhs.cols_; ++j) {
                    result.data_[i * rhs.cols_ + j] += temp * rhs.data_[k * rhs.cols_ + j];
                }
            }
        }
        return result;
    }

    size_t get_rows() const noexcept { return rows_; }
    size_t get_cols() const noexcept { return cols_; }

    void display() const {
        for (size_t i = 0; i < rows_; ++i) {
            for (size_t j = 0; j < cols_; ++j) {
                std::cout << std::setw(10) << std::setprecision(4) << (*this)(i, j) << " ";
            }
            std::cout << "\n";
        }
    }
};

int main() {
    try {
        MathMatrix mat_a(10, 10);
        //mat_a.display();
        MathMatrix mat_b(10, 10);
        //mat_b.display();

        mat_a(5, 5) = 42.5;
        mat_b(5, 5) = 2.0;

        auto start_time = std::chrono::high_resolution_clock::now();
        
        MathMatrix mat_c = mat_a + mat_b;
        //mat_c.display();
        
        MathMatrix mat_d = std::move(mat_c);
        mat_d.display();

        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end_time - start_time;

        std::cout << "Operation completed in: " << elapsed.count() << " ms\n";
        std::cout << "Value at (5, 5): " << mat_d(5, 5) << "\n";

    } catch (const std::exception& e) {
        std::cerr << "System Error: " << e.what() << "\n";
    }

    return 0;
}