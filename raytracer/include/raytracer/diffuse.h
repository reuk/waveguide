#pragma once

#include "raytracer/cl_structs.h"
#include "raytracer/iterative_builder.h"

#include "common/aligned/vector.h"
#include "common/cl_include.h"

namespace raytracer {

class diffuse_finder final {
public:
    diffuse_finder(const cl::Context&,
                   const cl::Device&,
                   size_t rays,
                   size_t depth);

    void push(const aligned::vector<Reflection>& reflections);
    const aligned::vector<aligned::vector<Impulse>>& get_results() const;
    aligned::vector<aligned::vector<Impulse>>& get_results();

private:
    cl::Context context;
    cl::Device device;

    iterative_builder<Impulse> impulse_builder;
};

}  // namespace raytracer