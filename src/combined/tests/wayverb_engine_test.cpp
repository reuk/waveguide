#include "combined/engine.h"

#include "core/cl/common.h"
#include "core/geo/box.h"
#include "core/model/receiver.h"
#include "core/scene_data.h"

#include "gtest/gtest.h"

TEST(engine, engine) {
    constexpr auto min = glm::vec3{0, 0, 0};
    constexpr auto max = glm::vec3{5.56, 3.97, 2.81};
    const auto box = core::geo::box{min, max};
    constexpr auto params = core::model::parameters{
            glm::vec3{2.09, 2.12, 2.12}, glm::vec3{2.09, 3.08, 0.96}};
    constexpr auto output_sample_rate = 96000.0;
    constexpr auto surface =
            core::make_surface<core::simulation_bands>(0.1, 0.1);

    const auto scene_data = core::geo::get_scene_data(box, surface);

    const wayverb::engine e{core::compute_context{},
                            scene_data,
                            params,
                            raytracer::simulation_parameters{1 << 16, 5},
                            waveguide::single_band_parameters{10000, 0.5}};

    const auto callback = [](auto state, auto progress) {
        std::cout << '\r' << std::setw(30) << to_string(state) << std::setw(10)
                  << progress << std::flush;
    };

    const auto intermediate = e.run(true, callback);

    if (intermediate == nullptr) {
        throw std::runtime_error{"failed to generate intermediate results"};
    }

    const auto result = intermediate->postprocess(core::attenuator::null{},
                                                  output_sample_rate);
}