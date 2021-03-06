#include "core/filters_common.h"

#include "utilities/string_builder.h"

#include "audio_file/audio_file.h"

#include "gtest/gtest.h"

#include <random>

using namespace wayverb::core;

namespace {

auto white_noise(size_t length) {
    std::default_random_engine engine{std::random_device{}()};
    const auto mag = 0.5;
    std::uniform_real_distribution<float> distribution(-mag, mag);

    util::aligned::vector<float> ret{};
    ret.reserve(length);
    for (auto i = 0ul; i != length; ++i) {
        ret.emplace_back(distribution(engine));
    }
    return ret;
}

const auto sample_rate = 44100.0;
const auto bit_depth = 16;
const auto noise = white_noise(sample_rate * 10);

template <size_t num>
void run_filter(
        util::aligned::vector<float> sig,
        const std::string& name,
        const std::array<filter::biquad::coefficients, num>& coefficients) {
    static_assert(num != 0, "Why would we want a zero-item filter?");

    auto filt = make_series_biquads(coefficients);
    run_two_pass(filt, sig.begin(), sig.end());
    write(util::build_string(name, ".wav").c_str(),
          sig,
          sample_rate,
          audio_file::format::wav,
          audio_file::bit_depth::pcm16);
}

template <size_t num>
void run_filters(const std::string& name, double cutoff) {
    run_filter(noise,
               util::build_string(name, "_", num, "_hipass"),
               filter::compute_hipass_butterworth_coefficients<num>(
                       cutoff, sample_rate));
    run_filter(noise,
               util::build_string(name, "_", num, "_lopass"),
               filter::compute_lopass_butterworth_coefficients<num>(
                       cutoff, sample_rate));
}

}  // namespace

TEST(filters_common, butterworth) {
    const auto lim = 4ul;
    for (auto i = 1ul; i != lim; ++i) {
        const auto frequency = (i * sample_rate) / (2 * lim);
        const auto name = util::build_string(frequency, "_hz");
        run_filters<1>(name, frequency);
        run_filters<2>(name, frequency);
        run_filters<4>(name, frequency);
    }
}
