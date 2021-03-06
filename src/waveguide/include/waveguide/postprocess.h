#pragma once

#include "waveguide/attenuator.h"
#include "waveguide/canonical.h"
#include "waveguide/config.h"

#include "core/attenuator/hrtf.h"
#include "core/attenuator/microphone.h"
#include "core/cl/iterator.h"
#include "core/cl/scene_structs.h"
#include "core/mixdown.h"

#include "utilities/map_to_vector.h"

namespace wayverb {
namespace waveguide {

template <typename T>
using dereferenced_t = decltype(*std::declval<T>());

template <typename T, typename U>
constexpr auto dereferenced_type_matches_v =
        std::is_same<std::decay_t<dereferenced_t<T>>, U>::value;

/// We need a unified interface for dealing with single-band microphone output
/// and multi-band hrtf output.
/// We try to determine whether the iterator is over a single- or multi-band
/// type, and then filter appropriately in the multi-band case.

/// We start with the 'audible range' defined in Hz.
/// This gives us well-defined 8-band edges and widths, also in Hz.

/// If the iterator is over `bands_type` use this one.
/// Audible range is normalised in terms of the waveguide sampling rate.
template <typename It,
          std::enable_if_t<std::is_same<std::decay_t<dereferenced_t<It>>,
                                        core::bands_type>::value,
                           int> = 0>
auto postprocess(It begin, It end, double sample_rate) {
    return core::multiband_filter_and_mixdown(
            begin, end, sample_rate, [](auto it, auto index) {
                return core::make_cl_type_iterator(std::move(it), index);
            });
}

/// If the iterator is over a floating-point type use this one.
template <typename It,
          std::enable_if_t<std::is_floating_point<
                                   std::decay_t<dereferenced_t<It>>>::value,
                           int> = 0>
auto postprocess(It begin, It end, double sample_rate) {
    return util::aligned::vector<float>(begin, end);
}

////////////////////////////////////////////////////////////////////////////////

template <typename Method>
auto postprocess(const band& band,
                 const Method& method,
                 double acoustic_impedance,
                 double output_sample_rate) {
    auto attenuated = util::map_to_vector(
            begin(band.directional),
            end(band.directional),
            make_attenuate_mapper(method, acoustic_impedance));
    const auto ret =
            postprocess(begin(attenuated), end(attenuated), band.sample_rate);

    return waveguide::adjust_sampling_rate(ret.data(),
                                           ret.size(),
                                           band.sample_rate,
                                           output_sample_rate);
}

template <typename Method>
auto postprocess(const util::aligned::vector<bandpass_band>& results,
                 const Method& method,
                 double acoustic_impedance,
                 double output_sample_rate) {
    util::aligned::vector<float> ret;

    for (const auto& band : results) {
        auto processed = postprocess(
                band.band, method, acoustic_impedance, output_sample_rate);

        const auto cutoff = band.valid_hz / output_sample_rate;

        //  Bandpass based on previous band cutoff.
        frequency_domain::filter filt{
                frequency_domain::best_fft_length(processed.size()) << 2};

        constexpr auto l = 0;
        constexpr auto width = 0.1;

        const auto b = begin(processed);
        const auto e = end(processed);
        filt.run(b, e, b, [&](auto cplx, auto freq) {
            return cplx * static_cast<float>(
                                  frequency_domain::compute_bandpass_magnitude(
                                          freq, cutoff, width, l));
        });

        //  Add results to ret.
        ret.resize(std::max(ret.size(), processed.size()), 0.0f);
        std::transform(b, e, begin(ret), begin(ret), std::plus<>{});
    }

    {
        //  DC blocking, just in case...
        //  Won't catch exponential drift, but should get really low
        //  oscillations.
        constexpr auto dc_block_hz = 10.0;
        const auto dc_block = dc_block_hz / output_sample_rate;
        frequency_domain::filter filt{
                frequency_domain::best_fft_length(ret.size()) << 2};
        const auto b = begin(ret);
        const auto e = end(ret);
        filt.run(b, e, b, [&](auto cplx, auto freq) {
            return cplx * static_cast<float>(
                                  frequency_domain::compute_hipass_magnitude(
                                          freq, dc_block, 0.9, 0));
        });
    }

    return ret;
}

}  // namespace waveguide
}  // namespace wayverb
