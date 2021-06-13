# LazyCat :cat: — fast lazy string concatenation

LazyCat is a lightweight header-only C++ library for fast lazy string concatenation.  Definitely faster than doing it the obvious way, and often faster than `absl::StrCat`.  No dependencies apart from the C++ standard library.

## Basic Usage

Just `#include <lazycat/lazycat.hpp>`, and start using it:

```cpp
std::string name = "LazyCat";
std::string result = lazycat("Hello, my name is ", name, '!'); // "Hello, my name is LazyCat!"
```

LazyCat is _lazy_ because the object returned by `lazycat()` holds references to the given arguments.  The resulting string is only materialized when it is coerced to `std::string`.  For example:

```cpp
std::string name = "LazyCat";
auto tmp = lazycat("Hello, my name is ", name, '!'); // `tmp` is a POD object that holds references to the string arguments
std::string result = tmp; // this statement causes the arguments to be written into the result string
```

## Benchmarks

TODO

## Advanced Usage

TODO
