

#pragma once

#include "Vector.hh"
template <int n> class Matrix {

private:
  double data[n][n];

public:
  Matrix() {
    for (int i = 0; i < n; i++) {
      for (int j = 0; j < n; j++) {
        data[i][j] = (i == j) ? 1.0 : 0.0;
      }
    }
  }

  Matrix(std::initializer_list<std::initializer_list<double>> init) {
    int i = 0;
    for (const auto &row : init) {
      int j = 0;
      for (const auto &val : row) {
        data[i][j] = val;
        j++;
      }
      i++;
    }
  }
  double *operator[](const int i) { return data[i]; }
  double const *operator[](const int i) const { return data[i]; }

  Matrix<n> operator+(const Matrix<n> &other) const {
    Matrix<n> res;
    for (int i = 0; i < n; i++) {
      for (int j = 0; j < n; j++) {
        res[i][j] = data[i][j] + other[i][j];
      }
    }
    return res;
  }

  Matrix<n> operator*(const Matrix<n> &other) const {
    Matrix<n> res;
    for (int i = 0; i < n; i++) {

      for (int k = 0; k < n; k++) {
        double r = data[i][k];

        for (int j = 0; j < n; j++) {
          res[i][j] += r * other[k][j];
        }
      }
    }

    return res;
  }

  Matrix<n> operator*(const double scalar) const {
    Matrix<n> res;
    for (int i = 0; i < n; i++) {
      for (int j = 0; j < n; j++) {
        res[i][j] = data[i][j] * scalar;
      }
    }
    return res;
  }
  inline Vec<n> operator*(const Vec<n> &vec) const {
    Vec<n> res;
    for (int i = 0; i < n; i++) {
      res[i] = .0;

      for (int j = 0; j < n; j++) {
        res[i] += data[i][j] * vec[j];
      }
    }
    return res;
  }

  Matrix<n> transposed() const {
    Matrix<n> res;
    for (int i = 0; i < n; i++) {
      for (int j = 0; j < n; j++) {
        res[j][i] = data[i][j];
      }
    }
    return res;
  }

  Matrix<n> inverted() const {
    Matrix<n> res;
    Matrix<n> aug;

    for (int i = 0; i < n; i++) {
      for (int j = 0; j < n; j++) {
        aug[i][j] = data[i][j];
        res[i][j] = (i == j) ? 1.0 : 0.0;
      }
    }

    for (int i = 0; i < n; i++) {
      double diag_elem = aug[i][i];
      for (int j = 0; j < n; j++) {
        aug[i][j] /= diag_elem;
        res[i][j] /= diag_elem;
      }

      for (int k = 0; k < n; k++) {
        if (k != i) {
          double factor = aug[k][i];
          for (int j = 0; j < n; j++) {
            aug[k][j] -= factor * aug[i][j];
            res[k][j] -= factor * res[i][j];
          }
        }
      }
    }

    return res;
  }

  void set_col(int col_idx, const Vec<n> &col) {
    for (int i = 0; i < n; i++) {
      data[i][col_idx] = col[i];
    }
  }
};
