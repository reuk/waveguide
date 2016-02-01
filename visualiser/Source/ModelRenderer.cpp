#include "ModelRenderer.hpp"

#include "boundaries.h"
#include "conversions.h"
#include "cl_common.h"
#include "tetrahedral_program.h"

RaytraceObject::RaytraceObject(const GenericShader &shader,
                               const RaytracerResults &results)
        : shader(shader) {
    auto impulses = results.impulses;
    std::vector<glm::vec3> v(impulses.size() + 1);
    v.front() = to_glm_vec3(results.source);
    std::transform(impulses.begin(),
                   impulses.end(),
                   v.begin() + 1,
                   [](auto i) { return to_glm_vec3(i.position); });

    std::vector<glm::vec4> c(v.size());
    c.front() = glm::vec4(1, 1, 1, 1);
    std::transform(impulses.begin(),
                   impulses.end(),
                   c.begin() + 1,
                   [](auto i) {
                       auto average = std::accumulate(std::begin(i.volume.s),
                                                      std::end(i.volume.s),
                                                      0.0f) /
                                      8;
                       auto c = i.time ? fabs(average) * 10 : 0;
                       return glm::vec4(c, c, c, c);
                   });

    std::vector<std::pair<GLuint, GLuint>> lines;
    for (auto ray = 0u; ray != results.rays; ++ray) {
        auto prev = 0u;
        for (auto pt = 0u; pt != results.rays; ++pt) {
            auto index = ray * results.reflections + pt + 1;
            lines.push_back(std::make_pair(prev, index));
            prev = index;
        }
    }

    std::vector<GLuint> indices;
    for (const auto &i : lines) {
        indices.push_back(i.first);
        indices.push_back(i.second);
    }

    size = indices.size();

    geometry.data(v);
    colors.data(c);
    ibo.data(indices);

    //  init vao
    auto s_vao = vao.get_scoped();

    geometry.bind();
    auto v_pos = shader.get_attrib_location("v_position");
    glEnableVertexAttribArray(v_pos);
    glVertexAttribPointer(v_pos, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    colors.bind();
    auto c_pos = shader.get_attrib_location("v_color");
    glEnableVertexAttribArray(c_pos);
    glVertexAttribPointer(c_pos, 4, GL_FLOAT, GL_FALSE, 0, nullptr);

    ibo.bind();
}

void RaytraceObject::draw() const {
    auto s_shader = shader.get_scoped();
    shader.set_black(false);

    auto s_vao = vao.get_scoped();
    glDrawElements(GL_LINES, size, GL_UNSIGNED_INT, nullptr);
}

//----------------------------------------------------------------------------//

static constexpr auto model_colour = 0.75;

VoxelisedObject::VoxelisedObject(const GenericShader &shader,
                                 const SceneData &scene_data,
                                 const VoxelCollection &voxel)
        : BasicDrawableObject(
              shader,
              get_vertices(scene_data),
              std::vector<glm::vec4>(
                  scene_data.get_vertices().size(),
                  glm::vec4(
                      model_colour, model_colour, model_colour, model_colour)),
              get_indices(scene_data, voxel),
              GL_TRIANGLES)
        , voxel(voxel) {
}

std::vector<glm::vec3> VoxelisedObject::get_vertices(
    const SceneData &scene_data) const {
    std::vector<glm::vec3> ret(scene_data.get_vertices().size());
    std::transform(scene_data.get_vertices().begin(),
                   scene_data.get_vertices().end(),
                   ret.begin(),
                   [](auto i) { return glm::vec3(i.x, i.y, i.z); });
    return ret;
}

void fill_indices_vector(const SceneData &scene_data,
                         const VoxelCollection &voxel,
                         std::vector<GLuint> &ret) {
    for (const auto &x : voxel.get_data()) {
        for (const auto &y : x) {
            for (const auto &z : y) {
                for (const auto &i : z.get_triangles()) {
                    const auto &tri = scene_data.get_triangles()[i];
                    ret.push_back(tri.v0);
                    ret.push_back(tri.v1);
                    ret.push_back(tri.v2);
                }
            }
        }
    }
}

std::vector<GLuint> VoxelisedObject::get_indices(
    const SceneData &scene_data, const VoxelCollection &voxel) const {
    std::vector<GLuint> ret;
    fill_indices_vector(scene_data, voxel, ret);
    return ret;
}

void VoxelisedObject::draw() const {
    BasicDrawableObject::draw();
    BoxObject box(get_shader());

    for (const auto &x : voxel.get_data()) {
        for (const auto &y : x) {
            for (const auto &z : y) {
                if (!z.get_triangles().empty()) {
                    const auto &aabb = z.get_aabb();
                    box.set_scale(to_glm_vec3(aabb.get_dimensions()));
                    box.set_position(to_glm_vec3(aabb.get_centre()));
                    box.draw();
                }
            }
        }
    }
}

//----------------------------------------------------------------------------//

DrawableScene::DrawableScene(const GenericShader &shader,
                             const SceneData &scene_data,
                             const CombinedConfig &cc)
        : shader(shader)
        , context(get_context())
        , device(get_device(context))
        , queue(context, device)
        , model_object{std::make_unique<VoxelisedObject>(
              shader, scene_data, VoxelCollection(scene_data, 4, 0.1))}
        , source_object{std::make_unique<OctahedronObject>(
              shader, to_glm_vec3(cc.get_source()), glm::vec4(1, 0, 0, 1))}
        , receiver_object{std::make_unique<OctahedronObject>(
              shader, to_glm_vec3(cc.get_mic()), glm::vec4(0, 1, 1, 1))}
        , waveguide_load_thread{&DrawableScene::init_waveguide,
                                this,
                                scene_data,
                                cc}
        , raytracer_results{
              std::move(std::async(std::launch::async,
                                   &DrawableScene::get_raytracer_results,
                                   this,
                                   scene_data,
                                   cc))} {
}

DrawableScene::~DrawableScene() noexcept {
    auto join_future = [](auto &i) {
        if (i.valid())
            i.get();
    };
    auto join_thread = [](auto &i) {
        if (i.joinable())
            i.join();
    };

    join_future(future_pressure);
    join_future(raytracer_results);
    join_thread(waveguide_load_thread);
}

void DrawableScene::init_waveguide(const SceneData &scene_data,
                                   const WaveguideConfig &cc) {
    MeshBoundary boundary(scene_data);
    auto waveguide_program =
        get_program<Waveguide::ProgramType>(context, device);
    auto w = std::make_unique<Waveguide>(
        waveguide_program, queue, boundary, cc.get_divisions(), cc.get_mic());
    auto corrected_source = cc.get_source();

    w->init(corrected_source, GaussianFunction(0.1), 0, 0);

    {
        std::lock_guard<std::mutex> lck(mut);
        waveguide = std::move(w);
        trigger_pressure_calculation();
    }
}

void DrawableScene::trigger_pressure_calculation() {
    try {
        future_pressure = std::async(
            std::launch::async, [this] { return waveguide->run_step_slow(); });
    } catch (...) {
        std::cout << "async error?" << std::endl;
    }
}

RaytracerResults DrawableScene::get_raytracer_results(
    const SceneData &scene_data, const CombinedConfig &cc) {
    auto raytrace_program = get_program<RayverbProgram>(context, device);
    return ImprovedRaytrace(raytrace_program, queue)
        .BaseRaytrace::run(scene_data,
                           cc.get_mic(),
                           cc.get_source(),
                           cc.get_rays(),
                           cc.get_impulses())
        .get_diffuse();
}

void DrawableScene::update(float dt) {
    std::lock_guard<std::mutex> lck(mut);

    if (waveguide && !mesh_object) {
        mesh_object =
            std::make_unique<MeshObject<Waveguide>>(shader, *waveguide);
        std::cout << "showing mesh with " << waveguide->get_nodes() << " nodes, of which: "
                  << std::endl;
        std::cout << "  1d boundary nodes: " << waveguide->get_mesh().compute_num_boundary<1>() << std::endl;
        std::cout << "  2d boundary nodes: " << waveguide->get_mesh().compute_num_boundary<2>() << std::endl;
        std::cout << "  3d boundary nodes: " << waveguide->get_mesh().compute_num_boundary<3>() << std::endl;
    }

    if (raytracer_results.valid()) {
        try {
            if (raytracer_results.wait_for(std::chrono::milliseconds(0)) ==
                std::future_status::ready) {
                raytrace_object = std::make_unique<RaytraceObject>(
                    shader, raytracer_results.get());
            }
        } catch (const std::exception &e) {
            std::cout << "exception fetching raytracer results: " << e.what()
                      << std::endl;
        }
    }

    if (future_pressure.valid() && waveguide && mesh_object) {
        try {
            if (future_pressure.wait_for(std::chrono::milliseconds(0)) ==
                std::future_status::ready) {
                mesh_object->set_pressures(future_pressure.get());
                trigger_pressure_calculation();
            }
        } catch (const std::exception &e) {
            std::cout << "exception fetching waveguide results: " << e.what()
                      << std::endl;
        }
    }
}

void DrawableScene::draw() const {
    std::lock_guard<std::mutex> lck(mut);

    auto s_shader = shader.get_scoped();
    auto draw_thing = [this](const auto &i) {
        if (i)
            i->draw();
    };

    draw_thing(mesh_object);
    draw_thing(raytrace_object);

    if (model_object) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        model_object->draw();

        draw_thing(source_object);
        draw_thing(receiver_object);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

//----------------------------------------------------------------------------//

SceneRenderer::SceneRenderer()
        : projection_matrix(get_projection_matrix(1)) {
}

SceneRenderer::~SceneRenderer() {
}

void SceneRenderer::load_from_file_package(const FilePackage &fp) {
    std::lock_guard<std::mutex> lck(mut);

    SceneData scene_data(fp.get_object().getFullPathName().toStdString(),
                         fp.get_material().getFullPathName().toStdString());
    CombinedConfig cc;
    try {
        cc = read_config(fp.get_config().getFullPathName().toStdString());
    } catch (...) {
    }

    cc.get_rays() = 1 << 5;
    cc.get_filter_frequency() = 2000;

    scene = std::make_unique<DrawableScene>(*shader, scene_data, cc);

    auto aabb = scene_data.get_aabb();
    auto m = aabb.get_centre();
    auto max = aabb.get_dimensions().max();
    auto s = max > 0 ? 20 / max : 1;

    translation = glm::translate(-glm::vec3(m.x, m.y, m.z));
    scale = s;
}

void SceneRenderer::newOpenGLContextCreated() {
    shader = std::make_unique<GenericShader>();
}

void SceneRenderer::renderOpenGL() {
    std::lock_guard<std::mutex> lck(mut);

    update();
    draw();
}

void SceneRenderer::openGLContextClosing() {
}

glm::mat4 SceneRenderer::get_projection_matrix(float aspect) {
    return glm::perspective(45.0f, aspect, 0.05f, 1000.0f);
}

void SceneRenderer::set_aspect(float aspect) {
    std::lock_guard<std::mutex> lck(mut);
    projection_matrix = get_projection_matrix(aspect);
}

void SceneRenderer::update() {
    if (scene)
        scene->update(0);
}

void SceneRenderer::draw() const {
    auto c = 0.0;
    glClearColor(c, c, c, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    auto s_shader = shader->get_scoped();
    shader->set_model_matrix(glm::mat4());
    shader->set_view_matrix(get_view_matrix());
    shader->set_projection_matrix(get_projection_matrix());

    auto draw_thing = [this](const auto &i) {
        if (i)
            i->draw();
    };

    draw_thing(scene);
}

glm::mat4 SceneRenderer::get_projection_matrix() const {
    return projection_matrix;
}
glm::mat4 SceneRenderer::get_view_matrix() const {
    auto rad = 20;
    glm::vec3 eye(0, 0, rad);
    glm::vec3 target(0, 0, 0);
    glm::vec3 up(0, 1, 0);
    auto mm = rotation * get_scale_matrix() * translation;
    return glm::lookAt(eye, target, up) * mm;
}

void SceneRenderer::set_rotation(float az, float el) {
    std::lock_guard<std::mutex> lck(mut);
    auto i = glm::rotate(az, glm::vec3(0, 1, 0));
    auto j = glm::rotate(el, glm::vec3(1, 0, 0));
    rotation = j * i;
}

void SceneRenderer::update_scale(float delta) {
    std::lock_guard<std::mutex> lck(mut);
    scale = std::max(0.0f, scale + delta);
}

glm::mat4 SceneRenderer::get_scale_matrix() const {
    return glm::scale(glm::vec3(scale, scale, scale));
}
