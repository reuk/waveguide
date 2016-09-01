#pragma once

#include "waveguide/boundary_coefficient_finder.h"
#include "waveguide/cl/utils.h"
#include "waveguide/descriptor.h"
#include "waveguide/program.h"

#include "common/aligned/vector.h"
#include "common/cl/include.h"
#include "common/map_to_vector.h"
#include "common/program_wrapper.h"
#include "common/spatial_division/scene_buffers.h"
#include "common/spatial_division/voxel_collection.h"
#include "common/stl_wrappers.h"

#include "glm/fwd.hpp"

class copyable_scene_data;

namespace waveguide {

class mesh_boundary;

//----------------------------------------------------------------------------//

class setup_program final {
public:
    setup_program(const compute_context& cc);

    auto get_node_position_and_neighbors_kernel() const {
        return program_wrapper.get_kernel<cl::Buffer,  /// nodes
                                          cl_int3,     /// dim
                                          cl_float3,   /// min_corner
                                          cl_float     /// spacing
                                          >("set_node_position_and_neighbors");
    }

    auto get_node_inside_kernel() const {
        return program_wrapper.get_kernel<cl::Buffer,  /// nodes
                                          cl::Buffer,  /// voxel_index
                                          aabb,        /// global_aabb
                                          cl_ulong,    /// side
                                          cl::Buffer,  /// triangles
                                          cl::Buffer   /// vertices
                                          >("set_node_inside");
    }

    auto get_node_boundary_type_kernel() const {
        return program_wrapper.get_kernel<cl::Buffer,  /// nodes
                                          cl_int3      /// dim
                                          >("set_node_boundary_type");
    }

private:
    program_wrapper program_wrapper;
};

//----------------------------------------------------------------------------//

condensed_node get_condensed(const node& n);
aligned::vector<condensed_node> get_condensed(const aligned::vector<node>& n);

constexpr bool is_inside(const condensed_node& c) {
    return c.boundary_type && id_inside;
}

//----------------------------------------------------------------------------//

class vectors final {
public:
    vectors(aligned::vector<condensed_node> nodes,
            aligned::vector<coefficients_canonical>
                    coefficients,
            boundary_index_data boundary_index_data);

    template <size_t n>
    const aligned::vector<boundary_index_array<n>>& get_boundary_indices()
            const;

    const aligned::vector<condensed_node>& get_condensed_nodes() const;
    const aligned::vector<coefficients_canonical>& get_coefficients() const;

    void set_coefficients(aligned::vector<coefficients_canonical> coefficients);

private:
    aligned::vector<condensed_node> condensed_nodes_;
    aligned::vector<coefficients_canonical> coefficients_;
    boundary_index_data boundary_index_data_;
};

template <>
inline const aligned::vector<boundary_index_array<1>>&
vectors::get_boundary_indices<1>() const {
    return boundary_index_data_.b1;
}
template <>
inline const aligned::vector<boundary_index_array<2>>&
vectors::get_boundary_indices<2>() const {
    return boundary_index_data_.b2;
}
template <>
inline const aligned::vector<boundary_index_array<3>>&
vectors::get_boundary_indices<3>() const {
    return boundary_index_data_.b3;
}

//----------------------------------------------------------------------------//

template <size_t N>
static boundary_data_array<N> construct_boundary_data_array(
        const boundary_index_array<N>& arr) {
    boundary_data_array<N> ret{};
    for (auto i = 0u; i != N; ++i) {
        ret.array[i].coefficient_index = arr.array[i];
    }
    return ret;
}

template <size_t n>
inline aligned::vector<boundary_data_array<n>> get_boundary_data(
        const vectors& d) {
    return map_to_vector(d.get_boundary_indices<n>(), [](const auto& i) {
        return construct_boundary_data_array(i);
    });
}

}  // namespace waveguide
