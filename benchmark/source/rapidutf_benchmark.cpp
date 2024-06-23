#include <benchmark/benchmark.h>
#include "rapidutf/rapidutf.hpp"
#include <string>

using namespace rapidutf;

// UTF8->UTF16 Conversion Benchmarks

static void BM_UTF8_to_UTF16_ASCII(benchmark::State& state) {
    std::string utf8(1000000, 'A'); // 1,000,000 ASCII characters
    for (auto _ [[maybe_unused]] : state) {
        std::u16string result = converter::utf8_to_utf16(utf8);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(utf8.length()));
}
BENCHMARK(BM_UTF8_to_UTF16_ASCII)
    ->Unit(benchmark::kMillisecond)
    ->DisplayAggregatesOnly(true);

static void BM_UTF8_to_UTF16_NonASCII(benchmark::State& state) {
    std::string utf8;
    for(size_t i = 0; i < 1000000; ++i) {
        utf8.append("世");
    }
    for (auto _ [[maybe_unused]] : state) {
        std::u16string result = converter::utf8_to_utf16(utf8);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(utf8.length()));
}
BENCHMARK(BM_UTF8_to_UTF16_NonASCII)
    ->Unit(benchmark::kMillisecond)
    ->DisplayAggregatesOnly(true);

// UTF16->UTF8 Conversion Benchmarks

static void BM_UTF16_to_UTF8_ASCII(benchmark::State& state) {
    std::u16string utf16(1000000, u'A'); // 1,000,000 ASCII characters
    for (auto _ [[maybe_unused]] : state) {
        std::string result = converter::utf16_to_utf8(utf16);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(utf16.length()));
}
BENCHMARK(BM_UTF16_to_UTF8_ASCII)
    ->Unit(benchmark::kMillisecond)
    ->DisplayAggregatesOnly(true);

static void BM_UTF16_to_UTF8_NonASCII(benchmark::State& state) {
    std::u16string utf16;
    for(size_t i = 0; i < 1000000; ++i) {
        utf16.append(u"世");
    }
    for (auto _ [[maybe_unused]] : state) {
        std::string result = converter::utf16_to_utf8(utf16);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(utf16.length()));
}
BENCHMARK(BM_UTF16_to_UTF8_NonASCII)
    ->Unit(benchmark::kMillisecond)
    ->DisplayAggregatesOnly(true);

// UTF32->UTF16 Conversion Benchmarks

static void BM_UTF32_to_UTF16_ASCII(benchmark::State& state) {
    std::u32string utf32(1000000, U'A'); // 1,000,000 ASCII characters
    for (auto _ [[maybe_unused]] : state) {
        std::u16string result = converter::utf32_to_utf16(utf32);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(utf32.length()));
}
BENCHMARK(BM_UTF32_to_UTF16_ASCII)
    ->Unit(benchmark::kMillisecond)
    ->DisplayAggregatesOnly(true);

static void BM_UTF32_to_UTF16_NonASCII(benchmark::State& state) {
    std::u32string utf32;
    for(size_t i = 0; i < 1000000; ++i) {
        utf32.append(U"世");
    }
    for (auto _ [[maybe_unused]] : state) {
        std::u16string result = converter::utf32_to_utf16(utf32);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(utf32.length()));
}
BENCHMARK(BM_UTF32_to_UTF16_NonASCII)
    ->Unit(benchmark::kMillisecond)
    ->DisplayAggregatesOnly(true);

// UTF16->UTF32 Conversion Benchmarks

static void BM_UTF16_to_UTF32_ASCII(benchmark::State& state) {
    std::u16string utf16(1000000, u'A'); // 1,000,000 ASCII characters
    for (auto _ [[maybe_unused]] : state) {
        std::u32string result = converter::utf16_to_utf32(utf16);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(utf16.length()));
}
BENCHMARK(BM_UTF16_to_UTF32_ASCII)
    ->Unit(benchmark::kMillisecond)
    ->DisplayAggregatesOnly(true);

static void BM_UTF16_to_UTF32_NonASCII(benchmark::State& state) {
    std::u16string utf16;
    for(size_t i = 0; i < 1000000; ++i) {
        utf16.append(u"世");
    }
    for (auto _ [[maybe_unused]] : state) {
        std::u32string result = converter::utf16_to_utf32(utf16);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(utf16.length()));
}
BENCHMARK(BM_UTF16_to_UTF32_NonASCII)
    ->Unit(benchmark::kMillisecond)
    ->DisplayAggregatesOnly(true);

// UTF8->UTF32 Conversion Benchmarks

static void BM_UTF8_to_UTF32_ASCII(benchmark::State& state) {
    std::string utf8(1000000, 'A'); // 1,000,000 ASCII characters
    for (auto _ [[maybe_unused]] : state) {
        std::u32string result = converter::utf8_to_utf32(utf8);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(utf8.length()));
}
BENCHMARK(BM_UTF8_to_UTF32_ASCII)
    ->Unit(benchmark::kMillisecond)
    ->DisplayAggregatesOnly(true);

static void BM_UTF8_to_UTF32_NonASCII(benchmark::State& state) {
    std::string utf8;
    for(size_t i = 0; i < 1000000; ++i) {
        utf8.append("世");
    }
    for (auto _ [[maybe_unused]] : state) {
        std::u32string result = converter::utf8_to_utf32(utf8);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(utf8.length()));
}
BENCHMARK(BM_UTF8_to_UTF32_NonASCII)
    ->Unit(benchmark::kMillisecond)
    ->DisplayAggregatesOnly(true);

// UTF32->UTF8 Conversion Benchmarks

static void BM_UTF32_to_UTF8_ASCII(benchmark::State& state) {
    std::u32string utf32(1000000, U'A'); // 1,000,000 ASCII characters
    for (auto _ [[maybe_unused]] : state) {
        std::string result = converter::utf32_to_utf8(utf32);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(utf32.length()));
}
BENCHMARK(BM_UTF32_to_UTF8_ASCII)
    ->Unit(benchmark::kMillisecond)
    ->DisplayAggregatesOnly(true);

static void BM_UTF32_to_UTF8_NonASCII(benchmark::State& state) {
    std::u32string utf32;
    for(size_t i = 0; i < 1000000; ++i) {
        utf32.append(U"世");
    }
    for (auto _ [[maybe_unused]] : state) {
        std::string result = converter::utf32_to_utf8(utf32);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(utf32.length()));
}
BENCHMARK(BM_UTF32_to_UTF8_NonASCII)
    ->Unit(benchmark::kMillisecond)
    ->DisplayAggregatesOnly(true);

BENCHMARK_MAIN();