#include <iostream>

int f4(int n);

int main(int argc, char const *argv[]) {
  f4(6);
  return 0;
}

struct matrix {
  int a00, a01, a10, a11;
  matrix operator*(matrix &b) {
    matrix r;
    r.a00 = a00 * b.a00 + a01 * b.a10;
    r.a01 = a00 * b.a01 + a01 * b.a11;
    r.a10 = a10 * b.a00 + a11 * b.a10;
    r.a11 = a10 * b.a01 + a11 * b.a11;
    return r;
  }
  void print() {
    std::cout << "[ " << a00 << ", " << a01 << "\n  " << a10 << ", " << a11
              << " ]" << std::endl;
  }
};
int f4(int n) {
  if (n <= 0) return 0;
  matrix a = {1, 1, 1, 0};
  matrix r = {1, 0, 0, 1};
  n -= 1;
  /*
  for (int i = 0; i < n; ++i) {
      r = r * a;
      cout << r.a00 << endl;
  }
  */
  while (n > 0) {
    r.print();
    a.print();
    if (n % 2 == 1) r = r * a;
    a = a * a;
    n /= 2;
  }

  r.print();
  return r.a00;
}