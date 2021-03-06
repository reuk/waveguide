#pragma once

#include "hrtf/entry.h"

#include "frequency_domain/multiband_filter.h"

#include "utilities/map.h"

namespace hrtf_data {

constexpr util::range<double> audible_range{20, 20000};

inline auto hrtf_band_centres_hz() {
    return frequency_domain::band_centres<entry::bands>(audible_range);
}

inline auto hrtf_band_centres(double sample_rate) {
    return util::map([&](auto i) { return i / sample_rate; },
                     hrtf_band_centres_hz());
}

inline auto hrtf_band_params_hz() {
    return frequency_domain::compute_multiband_params<entry::bands>(
            audible_range, 1);
}

inline auto hrtf_band_params(double sample_rate) {
    const auto hz = hrtf_band_params_hz();
    return frequency_domain::make_edges_and_width_factor(
            util::map([&](auto i) { return i / sample_rate; }, hz.edges),
            hz.width_factor);
}

template <typename It, typename Callback>
void multiband_filter(It begin,
                      It end,
                      double sample_rate,
                      const Callback& callback) {
    frequency_domain::multiband_filter(
            begin, end, hrtf_band_params(sample_rate), callback);
}

template <typename It>
auto per_band_energy(It begin, It end, double sample_rate) {
    return frequency_domain::per_band_energy(
            begin, end, hrtf_band_params(sample_rate));
}

}  // namespace hrtf_data
