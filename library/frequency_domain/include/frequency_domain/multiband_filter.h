#pragma once

#include "frequency_domain/envelope.h"
#include "frequency_domain/filter.h"

#include "utilities/map_to_vector.h"
#include "utilities/mapping_iterator_adapter.h"
#include "utilities/range.h"

namespace frequency_domain {

template <size_t N>
struct edges_and_width_factor final {
    std::array<double, N> edges;
    double width_factor;
};

template <size_t N>
constexpr auto make_edges_and_width_factor(const std::array<double, N>& edges,
                                           double width_factor) {
    return edges_and_width_factor<N>{edges, width_factor};
}

template <size_t N>
auto compute_multiband_params(range<double> audible_range, double overlap) {
    return make_edges_and_width_factor(band_edges<N>(audible_range),
                                       width_factor(audible_range, N, overlap));
}

////////////////////////////////////////////////////////////////////////////////

template <size_t bands_plus_one, typename It, typename Callback>
auto multiband_filter(It begin,
                      It end,
                      const edges_and_width_factor<bands_plus_one>& params,
                      const Callback& callback,
                      size_t l = 0) {
    constexpr auto bands = bands_plus_one - 1;
    filter filt{static_cast<size_t>(std::distance(begin, end) << 5)};

    //  Will store the area under each frequency-domain window.
    std::array<double, bands> integrated_bands{};

    for (auto i = 0ul; i != bands; ++i) {
        const auto b = callback(begin, i);
        const auto e = callback(end, i);
        filt.run(b, e, b, [&](auto cplx, auto freq) {
            const auto amp = compute_bandpass_magnitude(
                    freq,
                    make_range(params.edges[i + 0], params.edges[i + 1]),
                    params.width_factor,
                    l);

            integrated_bands[i] += amp;

            return cplx * static_cast<float>(amp);
        });
    }

    return integrated_bands;
}

template <typename It>
auto square_sum(It b, It e) {
    return std::accumulate(
            b, e, std::decay_t<decltype(*b)>(), [](auto a, auto b) {
                return a + b * b;
            });
}

template <typename It>
auto rms(It b, It e) {
    using std::sqrt;
    return sqrt(square_sum(b, e));
}

template <typename It, typename Callback>
auto band_rms(It begin, It end, const Callback& callback, size_t band) {
    const auto b = callback(begin, band);
    const auto e = callback(end, band);
    return rms(b, e);
}

template <typename It, typename Callback, size_t... Ix>
auto multiband_rms(It begin,
                   It end,
                   const Callback& callback,
                   std::index_sequence<Ix...>) {
    return std::array<double, sizeof...(Ix)>{
            {band_rms(begin, end, callback, Ix)...}};
}

template <size_t bands, typename It, typename Callback>
auto multiband_rms(It begin, It end, const Callback& callback) {
    return multiband_rms(
            begin, end, callback, std::make_index_sequence<bands>{});
}

template <size_t bands, typename T>
auto init_array(const T& t) {
    std::array<T, bands> ret{};
    std::fill(begin(ret), end(ret), t);
    return ret;
}

template <size_t bands, typename It>
auto make_multiband(It begin, It end) {
    return map_to_vector(
            begin, end, [](const auto& i) { return init_array<bands>(i); });
}

class indexer final {
public:
    constexpr explicit indexer(size_t index)
            : index_{index} {}

    template <typename T>
    constexpr auto& operator()(T& t) const {
        return t[index_];
    }

    template <typename T>
    constexpr const auto& operator()(const T& t) const {
        return t[index_];
    }

private:
    size_t index_;
};

struct make_indexer_iterator final {
    template <typename It>
    constexpr auto operator()(It it, size_t index) const {
        return make_mapping_iterator_adapter(std::move(it), indexer{index});
    }
};

template <size_t bands_plus_one, typename It>
auto per_band_energy(It begin,
                     It end,
                     const edges_and_width_factor<bands_plus_one>& params) {
    constexpr auto bands = bands_plus_one - 1;

    auto multiband = make_multiband<bands>(begin, end);

    const auto band_widths = multiband_filter(std::begin(multiband),
                                              std::end(multiband),
                                              params,
                                              make_indexer_iterator{});

    auto rms = multiband_rms<bands>(std::begin(multiband),
                                    std::end(multiband),
                                    make_indexer_iterator{});

    for (auto i = 0ul; i != bands; ++i) {
        rms[i] *= bands / sqrt(band_widths[i]);
    }

    return rms;
}

}  // namespace frequency_domain
