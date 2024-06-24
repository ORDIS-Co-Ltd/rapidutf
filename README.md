# RapidUTF

RapidUTF is a lightweight, high-performance C++ library designed for fast Unicode conversions between UTF-8, UTF-16, and UTF-32 encodings. Leveraging SIMD (Single Instruction, Multiple Data) instructions, RapidUTF achieves exceptional speed in character encoding transformations, making it ideal for applications requiring efficient text processing and manipulation.

## Features

- **Fast Unicode Conversions**: RapidUTF provides optimized functions for converting between UTF-8, UTF-16, and UTF-32 encodings, utilizing SIMD instructions for maximum performance.
- **Automatic SIMD Optimization**: The library automatically detects and utilizes the available SIMD instructions on the target platform, such as AVX2 or NEON, for optimal performance.
- **Comprehensive Validation**: RapidUTF includes functions to validate the correctness of UTF-8, UTF-16, and UTF-32 strings, ensuring data integrity.
- **Header-Only Library**: The core functionality of RapidUTF is provided as a header-only library, making it easy to integrate into your projects.
- **Benchmarking Suite**: A comprehensive benchmarking suite is included to measure and compare the performance of various conversion scenarios.
- **Catch2 Unit Tests**: The library comes with a comprehensive set of unit tests using the Catch2 testing framework, ensuring code correctness and reliability.

## Installation

RapidUTF is a header-only library, so you can simply include the `rapidutf.hpp` header file in your project. However, if you prefer to build the library and link against it, you can follow these steps:

1. Clone the repository:

```bash
git clone https://github.com/ORDIS-Co-Ltd/rapidutf.git
```

2. Navigate to the project directory:

```bash
cd rapidutf
```

3. Create a build directory and navigate to it:

```bash
mkdir build
cd build
```

4. Configure the project using CMake:

```bash
cmake ..
```

5. Build the project:

```bash
cmake --build .
```

After building, you can link against the `rapidutf` library in your project.

## Usage

Here's a simple example of how to use RapidUTF:

```cpp
#include "rapidutf/rapidutf.hpp"

int main() {
    std::string utf8 = "Hello, 世界!";
    std::u16string utf16 = rapidutf::converter::utf8_to_utf16(utf8);
    std::u32string utf32 = rapidutf::converter::utf16_to_utf32(utf16);

    // Validate UTF-8 string
    if (rapidutf::converter::is_valid_utf8(utf8)) {
        // Process valid UTF-8 string
    }

    // Convert back to UTF-8
    std::string converted_utf8 = rapidutf::converter::utf32_to_utf8(utf32);

    return 0;
}
```

For more examples and detailed usage, please refer to the documentation and examples provided in the repository.

## Contributing

Contributions to RapidUTF are welcome! If you find any issues or have suggestions for improvements, please open an issue or submit a pull request on the GitHub repository.

## License

RapidUTF is released under the [MIT License](LICENSE).
