#include "raytracer/raytracer.h"
#include "raytracer/image_source.h"
#include "raytracer/reflector.h"
#include "raytracer/scene_buffers.h"
#include "raytracer/diffuse.h"

#include "common/azimuth_elevation.h"
#include "common/boundaries.h"
#include "common/conversions.h"
#include "common/geometric.h"
#include "common/hrtf.h"
#include "common/triangle.h"
#include "common/voxel_collection.h"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

#include <cmath>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <streambuf>

namespace {
constexpr auto volume_factor = 0.001f;
constexpr VolumeType air_coefficient{{
        volume_factor * -0.1f,
        volume_factor * -0.2f,
        volume_factor * -0.5f,
        volume_factor * -1.1f,
        volume_factor * -2.7f,
        volume_factor * -9.4f,
        volume_factor * -29.0f,
        volume_factor * -60.0f,
}};
}  // namespace

namespace raytracer {

int compute_optimum_reflection_number(float min_amp, float max_reflectivity) {
    return std::log(min_amp) / std::log(max_reflectivity);
}

aligned::vector<aligned::vector<aligned::vector<float>>> flatten_impulses(
        const aligned::vector<aligned::vector<AttenuatedImpulse>>& attenuated,
        float samplerate) {
    aligned::vector<aligned::vector<aligned::vector<float>>> flattened(
            attenuated.size());
    proc::transform(attenuated, begin(flattened), [samplerate](const auto& i) {
        return flatten_impulses(i, samplerate);
    });
    return flattened;
}

double pressure_to_intensity(double pressure, double Z = 400) {
    return pressure * pressure / Z;
}

double intensity_to_pressure(double intensity, double Z = 400) {
    return std::sqrt(intensity * Z);
}

/// Turn a collection of AttenuatedImpulses into a vector of 8 vectors, where
/// each of the 8 vectors represent sample values in a different frequency band.
aligned::vector<aligned::vector<float>> flatten_impulses(
        const aligned::vector<AttenuatedImpulse>& impulse, float samplerate) {
    const auto MAX_TIME_LIMIT = 20.0f;
    // Find the index of the final sample based on time and samplerate
    auto maxtime = std::min(
            std::accumulate(impulse.begin(),
                            impulse.end(),
                            0.0f,
                            [](auto i, auto j) { return std::max(i, j.time); }),
            MAX_TIME_LIMIT);

    const auto MAX_SAMPLE = round(maxtime * samplerate) + 1;

    //  Create somewhere to store the results.
    aligned::vector<aligned::vector<float>> flattened(
            sizeof(VolumeType) / sizeof(float),
            aligned::vector<float>(MAX_SAMPLE, 0));

    //  For each impulse, calculate its index, then add the impulse's volumes
    //  to the volumes already in the output array.
    for (const auto& i : impulse) {
        const auto SAMPLE = round(i.time * samplerate);
        if (SAMPLE < MAX_SAMPLE) {
            for (auto j = 0u; j != flattened.size(); ++j) {
                flattened[j][SAMPLE] += i.volume.s[j];
            }
        }
    }

    //  impulses are intensity levels, now we need to convert to pressure
    proc::for_each(flattened, [](auto& i) {
        proc::for_each(i, [](auto& j) {
            j = std::copysign(intensity_to_pressure(std::abs(j)), j);
        });
    });

    return flattened;
}

/// Find the index of the last sample with an amplitude of minVol or higher,
/// then resize the vectors down to this length.
void trimTail(aligned::vector<aligned::vector<float>>& audioChannels,
              float minVol) {
    using index_type = std::common_type_t<
            std::iterator_traits<
                    aligned::vector<float>::reverse_iterator>::difference_type,
            int>;

    // Find last index of required amplitude or greater.
    auto len = proc::accumulate(
            audioChannels, 0, [minVol](auto current, const auto& i) {
                return std::max(
                        index_type{current},
                        index_type{
                                distance(i.begin(),
                                         std::find_if(i.rbegin(),
                                                      i.rend(),
                                                      [minVol](auto j) {
                                                          return std::abs(j) >=
                                                                 minVol;
                                                      })
                                                 .base()) -
                                1});
            });

    // Resize.
    for (auto&& i : audioChannels)
        i.resize(len);
}

/// Collects together all the post-processing steps.
aligned::vector<aligned::vector<float>> process(
        aligned::vector<aligned::vector<aligned::vector<float>>>& data,
        float sr,
        bool do_normalize,
        float lo_cutoff,
        bool do_trim_tail,
        float volume_scale) {
    aligned::vector<aligned::vector<float>> ret;
    ret.reserve(data.size());
    for (const auto& i : data) {
        ret.push_back(multiband_filter_and_mixdown(i, sr));
    }

    if (do_normalize)
        normalize(ret);

    if (volume_scale != 1)
        mul(ret, volume_scale);

    if (do_trim_tail)
        trimTail(ret, 0.00001);

    return ret;
}

/// Call binary operation u on pairs of elements from a and b, where a and b are
/// cl_floatx types.
template <typename T, typename U>
inline T elementwise(const T& a, const T& b, const U& u) {
    T ret;
    proc::transform(a.s, std::begin(b.s), std::begin(ret.s), u);
    return ret;
}

//----------------------------------------------------------------------------//

VolumeType air_attenuation_for_distance(float distance) {
    VolumeType ret;
    proc::transform(air_coefficient.s, std::begin(ret.s), [distance](auto i) {
        return pow(M_E, distance * i);
    });
    return ret;
}

float power_attenuation_for_distance(float distance) {
    return 1 / (4 * M_PI * distance * distance);
}

VolumeType attenuation_for_distance(float distance) {
    auto ret   = air_attenuation_for_distance(distance);
    auto power = power_attenuation_for_distance(distance);
    proc::for_each(ret.s, [power](auto& i) { i *= power; });
    return ret;
}

std::experimental::optional<Impulse> get_direct_impulse(
        const glm::vec3& source,
        const glm::vec3& receiver,
        const CopyableSceneData& scene_data) {
    if (geo::point_intersection(receiver,
                                source,
                                scene_data.get_triangles(),
                                scene_data.get_converted_vertices())) {
        auto init_diff = source - receiver;
        auto init_dist = glm::length(init_diff);
        return Impulse{attenuation_for_distance(init_dist),
                       to_cl_float3(receiver + init_diff),
                       static_cast<cl_float>(init_dist / SPEED_OF_SOUND)};
    }
    return std::experimental::optional<Impulse>();
}

/*
raytracer::raytracer(const cl::Context& context, const cl::Device& device)
        : queue(context, device)
        , kernel(raytracer_program(context, device).get_raytrace_kernel()) {}

results raytracer::run(const CopyableSceneData& scene_data,
                       const glm::vec3& micpos,
                       const glm::vec3& source,
                       size_t rays,
                       size_t reflections,
                       size_t num_image_source,
                       std::atomic_bool& keep_going,
                       const std::function<void()>& callback) {
    return run(scene_data,
               micpos,
               source,
               get_random_directions(rays),
               reflections,
               num_image_source,
               keep_going,
               callback);
}

results raytracer::run(const CopyableSceneData& scene_data,
                       const glm::vec3& micpos,
                       const glm::vec3& source,
                       const aligned::vector<cl_float3>& directions,
                       size_t reflections,
                       size_t num_image_source,
                       std::atomic_bool& keep_going,
                       const std::function<void()>& callback) {
    aligned::vector<RayInfo> ray_info;
    ray_info.reserve(directions.size());
    for (const auto& i : directions) {
        ray_info.push_back(RayInfo{Ray{to_cl_float3(source), i},
                                   VolumeType{{1, 1, 1, 1, 1, 1, 1, 1}},
                                   to_cl_float3(micpos),
                                   0,
                                   true});
    }

    auto context = queue.getInfo<CL_QUEUE_CONTEXT>();

    auto load_to_buffer = [this, context](auto i, bool readonly) {
        return cl::Buffer(context, i.begin(), i.end(), readonly);
    };

    auto cl_ray_info  = load_to_buffer(ray_info, false);
    auto cl_triangles = load_to_buffer(scene_data.get_triangles(), true);
    auto cl_vertices  = load_to_buffer(scene_data.get_vertices(), true);
    auto cl_surfaces  = load_to_buffer(scene_data.get_surfaces(), true);

    auto cl_impulses = cl::Buffer(
            context, CL_MEM_READ_WRITE, directions.size() * sizeof(Impulse));
    auto cl_image_source = cl::Buffer(
            context, CL_MEM_READ_WRITE, directions.size() * sizeof(Impulse));
    auto cl_prev_primitives = cl::Buffer(
            context,
            CL_MEM_READ_WRITE,
            directions.size() * num_image_source * sizeof(TriangleVerts));
    auto cl_image_source_index = cl::Buffer(
            context, CL_MEM_READ_WRITE, directions.size() * sizeof(cl_ulong));

    VoxelCollection vox(scene_data, 4, 0.1);
    auto cl_voxel_index = load_to_buffer(vox.get_flattened(), true);

    aligned::vector<aligned::vector<cl_ulong>> image_source_index(
            num_image_source, aligned::vector<cl_ulong>(directions.size()));
    aligned::vector<aligned::vector<Impulse>> image_source(
            num_image_source, aligned::vector<Impulse>(directions.size()));

    auto result_image_source = get_direct_impulse(micpos, source, scene_data);

    aligned::vector<aligned::vector<Impulse>> result_diffuse(
            reflections, aligned::vector<Impulse>(directions.size()));

    for (auto i = 0u; i != reflections; ++i) {
        if (!keep_going) {
            throw std::runtime_error("flag state false, stopping");
        }

        //  load random numbers
        auto rng    = get_direction_rng(directions.size());
        auto cl_rng = load_to_buffer(rng, true);

        kernel(cl::EnqueueArgs(queue, cl::NDRange(directions.size())),
               cl_ray_info,
               cl_voxel_index,
               AABB{to_cl_float3(vox.get_aabb().get_c0()),
                    to_cl_float3(vox.get_aabb().get_c1())},
               vox.get_side(),
               cl_triangles,
               scene_data.get_triangles().size(),
               cl_vertices,
               cl_surfaces,
               cl_rng,
               to_cl_float3(source),
               to_cl_float3(micpos),
               air_coefficient,
               i,
               num_image_source,
               cl_impulses,
               cl_image_source,
               cl_prev_primitives,
               cl_image_source_index);

        auto copy_out_unchecked = [this, i](const auto& from, auto& to) {
            cl::copy(queue, from, to[i].begin(), to[i].end());
        };

        copy_out_unchecked(cl_impulses, result_diffuse);

        auto copy_out = [this, copy_out_unchecked, i](const auto& from,
                                                      auto& to) {
            if (i < to.size()) {
                copy_out_unchecked(from, to);

#ifndef NDEBUG
                using vt = typename std::decay_t<decltype(
                        to)>::value_type::value_type;
                assert(std::find_if(to[i].begin(), to[i].end(), [](auto i) {
                           return i != vt{};
                       }) != to[i].end());
#endif
            }
        };

        copy_out(cl_image_source_index, image_source_index);
        copy_out(cl_image_source, image_source);

        callback();
    }

    result_diffuse     = transpose(result_diffuse);
    image_source       = transpose(image_source);
    image_source_index = transpose(image_source_index);

    auto no_duplicates = remove_duplicates(
            image_source_index, image_source, std::move(result_image_source));

    return results(std::move(no_duplicates),
                   std::move(result_diffuse),
                   micpos,
                   source);
}
*/

raytracer::raytracer(const cl::Context& context, const cl::Device& device)
        : context(context)
        , device(device) {}

std::experimental::optional<results> raytracer::run(
        const CopyableSceneData& scene_data,
        const glm::vec3& source,
        const glm::vec3& receiver,
        size_t rays,
        size_t reflections,
        size_t image_source,
        std::atomic_bool& keep_going,
        const PerStepCallback& callback) {
    //  set up all the rendering context stuff

    //  load the scene into device memory
    scene_buffers scene_buffers(context, device, scene_data);

    //  this is the object that generates first-pass reflections
    reflector reflector(context, device, rays);
    reflector.init(source);

    //  this will collect the first reflections, to a specified depth,
    //  and use them to find unique image-source paths
    image_source_finder image_source_finder(rays, image_source);

    //  this will incrementally process diffuse responses
    diffuse diffuse(context, device, rays, reflections);

    //  run the simulation proper

    //  up until the max reflection depth
    for (auto i = 0u; i != reflections; ++i) {
        //  if the user cancelled, return an empty result
        if (!keep_going) {
            return std::experimental::optional<results>{};
        }

        //  get a single step of the reflections
        auto reflections = reflector.run_step(scene_buffers);

        //  find diffuse impulses for these reflections
        diffuse.push(reflections);

        {
            //  extract intersected surfaces and add to the path builder
            aligned::vector<cl_ulong> triangles;
            triangles.reserve(rays);
            for (const auto& i : reflections) {
                triangles.push_back(i.triangle);
            }

            image_source_finder.push(triangles);
        }
    }

    return results(get_direct_impulse(source, receiver, scene_data),
                   image_source_finder.get_results(),
                   diffuse.get_results(),
                   receiver);
}

}  // namespace raytracer
