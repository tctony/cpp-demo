#include <array>
#include <iostream>

template <int... Ns>
struct int_sequence {
  using Type = std::array<int, sizeof...(Ns)>;
  Type data{{Ns...}};
  Type make() { return Type(); }
};

template <int N, int... Ns>
struct make_int_sequence : make_int_sequence<N - 1, N - 1, Ns...> {};

template <int... Ns>
struct make_int_sequence<0, Ns...> : int_sequence<Ns...> {};

int main(int argc, char const *argv[]) {
  auto data = make_int_sequence<3>().data;
  for (int i = 0; i < data.size(); ++i) {
    std::cout << data[i] << " ";
  }
  return 0;
}
