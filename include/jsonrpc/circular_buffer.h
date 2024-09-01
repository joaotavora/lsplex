#include <algorithm>
#include <array>
#include <boost/asio/buffer.hpp>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <type_traits>

namespace lsplex::jsonrpc {

template <typename T, std::size_t N> class circular_buffer {
  std::array<T, N> _data{};
  std::size_t _a{0};  // Start of the valid data
  std::size_t _b{0};  // End of the valid data
  bool _full{false};

public:
  template <bool is_const> class t_iterator {
    std::conditional_t<is_const, const circular_buffer*, circular_buffer*>
        _buffer{nullptr};
    std::size_t _index{0};
    [[nodiscard]] constexpr std::size_t buf_a() const { return _buffer->_a;}
    [[nodiscard]] constexpr std::size_t buf_b() const { return _buffer->_b;}
    [[nodiscard]] constexpr bool isend() const {return _index == N;}
    

  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = std::conditional_t<is_const, const T*, T*>;
    using reference = std::conditional_t<is_const, const T&, T&>;

    t_iterator() = default;

    constexpr t_iterator(circular_buffer& buffer, std::size_t index)
      requires(!is_const)
        : _buffer(&buffer), _index(index) {}

    constexpr t_iterator(const circular_buffer& buffer, std::size_t index)
      requires(is_const)
        : _buffer(&buffer), _index(index) {}

    constexpr reference operator*() { return _buffer->_data.at(_index); }
    constexpr reference operator*() const { return _buffer->_data.at(_index); }

    constexpr pointer operator->() { return &(_buffer->_data[_index]); }

    constexpr t_iterator& operator+=(ssize_t n) {
      auto S = static_cast<ssize_t>(N);
      ssize_t tmp = (static_cast<ssize_t>(_index) + n) % S;
      if (tmp < 0) tmp += S;
      _index = static_cast<size_t>(tmp);
      if (n > 0 && _index == _buffer->_b) _index = N;
      return *this;
    }
    constexpr t_iterator operator+(ssize_t rhs) {
      auto tmp = *this;
      tmp += rhs;
      return tmp;
    }

    constexpr difference_type friend operator-(const t_iterator& lhs,
                                               const t_iterator& rhs) {
#ifndef NDEBUG
      assert(lhs._buffer == rhs._buffer);
#endif
      auto& buf = *lhs._buffer;
      constexpr auto S = static_cast<difference_type>(N);
      auto lhsi = lhs.isend() ? lhs.buf_b() : lhs._index;
      auto rhsi = rhs.isend() ? lhs.buf_b() : rhs._index;
      auto diff = static_cast<difference_type>(lhsi - rhsi);

      auto edge = lhsi == rhsi && buf.full();
      if (edge and lhs.isend()) return S;
      if (edge and rhs.isend()) return -S;

      if (lhsi >= lhs.buf_a()) {
        if (rhsi >= lhs.buf_a()) {
          return diff;
        } else {  // NOLINT
          return diff - S;
        }
      } else if (rhsi >= lhs.buf_a()) {
        return diff + S;
      } else {
        return diff;
      }
    }

    constexpr t_iterator& operator++() {
      *this += 1;
      return *this;
    }
    constexpr t_iterator& operator--() {
      *this += (-1);
      return *this;
    }

    constexpr bool operator==(const t_iterator& other) const {
      return _index == other._index;
    }

    constexpr bool operator!=(const t_iterator& other) const {
      return !(*this == other);
    }

    std::strong_ordering operator<=>(const t_iterator& other) const {
      return _index <=> other._index;
    }
  };

  using iterator = t_iterator<false>;
  using const_iterator = t_iterator<true>;

  constexpr iterator begin() { return iterator(*this, _a); }
  [[nodiscard]] constexpr const_iterator begin() const {
    return const_iterator(*this, _a);
  }
  constexpr iterator end() { return iterator(*this, N); }
  [[nodiscard]] constexpr const_iterator end() const {
    return const_iterator(*this, N);
  }

  [[nodiscard]] size_t constexpr size() const {
    return (_b >= _a && !_full) ? (_b - _a) : (N - _a + _b);
  }

  [[nodiscard]] constexpr bool full() const { return _full; }
  [[nodiscard]] constexpr bool empty() const { return _b == _a && !_full; }

  constexpr void clear() {
    _a = _b = 0;
    _full = false;
  }

  constexpr void consume(size_t n) {
#ifndef NDEBUG
    assert(n <= size());
#endif
    if (n > 0) _full = false;
    _a = (_a + n) % N;
  }

  constexpr void grow(size_t n) {
#ifndef NDEBUG
    assert(n <= N - size());
#endif
    if (n == N - size()) _full = true;
    _b = (_b + n) % N;
  }

  auto buffer() {
    namespace asio = boost::asio;
    using asio::buffer;
    using twobufs = std::array<asio::mutable_buffer, 2>;
    return full() ? twobufs{buffer(_data, 0), buffer(_data, 0)}
           : (_b >= _a)
               ? twobufs{buffer(&_data[_b], N - _b), buffer(_data, _a)}
               : twobufs{buffer(&_data[_b], _a - _b), buffer(_data, 0)};
  }
};

namespace {  // NOLINT
  constexpr auto buf1 = []() {
    circular_buffer<char, 50> b;
    b.grow(50);
    b.consume(20);
    return b;
  }();

  static_assert(buf1.size() == 30);
  static_assert(buf1.end() - buf1.begin() == 30);
  static_assert(buf1.begin() - buf1.end() == -30);

  constexpr auto buf2 = []() {
    circular_buffer<char, 50> b;
    b.grow(40);
    b.consume(20);
    return b;
  }();

  static_assert(buf2.size() == 20);
  static_assert(buf2.end() - buf2.begin() == 20);
  static_assert(buf2.begin() - buf2.end() == -20);

  constexpr auto buf3 = []() {
    circular_buffer<char, 50> b;
    b.grow(40);
    b.consume(30);
    b.grow(20);
    return b;
  }();

  static_assert(buf3.size() == 30);
  static_assert(buf3.end() - buf3.begin() == 30);
  static_assert(buf3.begin() - buf3.end() == -30);

  constexpr auto buf4 = []() {
    circular_buffer<char, 50> b;
    b.grow(50);
    return b;
  }();

  static_assert(buf4.full());
  static_assert(buf4.size() == 50);
  static_assert(buf4.end() - buf4.begin() == 50);
  static_assert(buf4.begin() - buf4.end() == -50);

  constexpr auto buf5 = []() {
    circular_buffer<char, 50> b;
    b.grow(50);
    b.consume(20);
    b.grow(20);
    return b;
  }();

  static_assert(buf5.full());
  static_assert(buf5.size() == 50);
  static_assert(buf5.end() - buf5.begin() == 50);
  static_assert(buf5.begin() - buf5.end() == -50);

  constexpr auto n = []() {
    circular_buffer<char, 50> b;
    b.grow(40);
    b.consume(30);
    b.grow(20);
    int count = 0;
    for ([[maybe_unused]] auto c : b) ++count;
    return count;
  }();
  static_assert(n == 30);

  constexpr auto ret6 = []() {
    circular_buffer<char, 50> b;
    b.grow(30);
    b.consume(30);
    std::string_view sv{"the rain in spaiN stays"};
    std::copy(sv.begin(), sv.end(), b.begin());
    auto beg = b.begin();
    beg += 22;
    beg += -22;
    return *beg;
  }();
  static_assert(ret6 == 't');
}  // namespace
}  // namespace lsplex::jsonrpc
