
#pragma once

#include <cmath>
#include <ostream>

template <int n> class Vec {
private:
  double data[n];

public:
  Vec() {
    for (int i = 0; i < n; i++)
      data[i] = .0;
  }

  Vec(std::initializer_list<double> init) {
    int i = 0;
    for (const auto &val : init) {
      data[i] = val;
      i++;
    }
  }

  double &operator[](const int i) { return data[i]; }

  const double &operator[](const int i) const { return data[i]; }

  Vec<n> operator*(const double a) const {
    Vec<n> res;

    for (int i = 0; i < n; i++) {
      res[i] = (*this)[i] * a;
    }

    return res;
  }

  double operator*(const Vec<n> &other) {
    double res = .0;

    for (int i = 0; i < n; i++) {
      res += (*this)[i] * other[i];
    }

    return res;
  }

  Vec<n> operator-(const Vec<n> &other) const {
    Vec<n> res;

    for (int i = 0; i < n; i++) {
      res[i] = (*this)[i] - other[i];
    }

    return res;
  }

  Vec<n> operator+(const Vec<n> &other) const {
    Vec<n> res;

    for (int i = 0; i < n; i++) {
      res[i] = (*this)[i] + other[i];
    }

    return res;
  }

  Vec<n> operator/(const double a) const {
    Vec<n> res;

    for (int i = 0; i < n; i++) {
      res[i] = (*this)[i] / a;
    }

    return res;
  }

  Vec<n> normalized() const {
    Vec<n> res;
    double len = .0;

    for (int i = 0; i < n; i++) {
      len += (*this)[i] * (*this)[i];
    }

    len = std::sqrt(len);

    for (int i = 0; i < n; i++) {
      res[i] = (*this)[i] / len;
    }

    return res;
  }

  Vec<n> cross(const Vec<n> &other) const {
    static_assert(n == 3, "Cross product is only defined for 3D vectors.");

    Vec<3> res;

    res[0] = (*this)[1] * other[2] - (*this)[2] * other[1];
    res[1] = (*this)[2] * other[0] - (*this)[0] * other[2];
    res[2] = (*this)[0] * other[1] - (*this)[1] * other[0];

    return res;
  }
  template <int k>
  friend std::ostream &operator<<(std::ostream &, const Vec<n> &);
};

template <int n> std::ostream &operator<<(std::ostream &out, const Vec<n> &v) {
  out << "(";
  for (int i = 0; i < n; i++) {
    out << v[i];
    if (i != n - 1) {
      out << ", ";
    }
  }
  out << ")";
  return out;
}

template <int n> inline double norm(const Vec<n> &v) {
  double len = .0;
  for (int i = 0; i < n; i++) {
    len += v[i] * v[i];
  }
  return std::sqrt(len);
}
