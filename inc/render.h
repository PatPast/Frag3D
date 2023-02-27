#ifndef _RENDER_H_
#define _RENDER_H_

#include <glad/glad.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <assets.h>
#include <common.h>

constexpr size_t point_shadowmap_size = 1024;
constexpr size_t directional_shadowmap_size = 2048;
constexpr float near_plane = 0.001f;
constexpr float far_plane = 1000.0f;
constexpr float shadow_near_plane = 0.001f;
constexpr float shadow_far_plane = 1000.0f;
constexpr Vector2i draw_framebuffer_size(640, 360);
constexpr int max_point_light_count = 10; // TODO @CLEANUP: We have the same define in the world shader

typedef int uniform_loc_t;
typedef unsigned int shader_handle_t;
typedef unsigned int buffer_handle_t;
typedef unsigned int tex_handle_t;

constexpr shader_handle_t default_shader_handle = 0;
constexpr buffer_handle_t default_buffer_handle = 0;
constexpr tex_handle_t default_tex_handle = 0;

typedef struct shader_s {
    shader_handle_t shader_program_handle{ default_shader_handle };
    uniform_loc_t get_location(const std::string& property_name) const; 
}shader_t;

explicit Shader() = default;
explicit Shader(const std::string& file_path);
void set_int(const std::string& property_name, int i) const;
void set_vec3(const std::string& roperty_name, const Vector3& v) const;
void set_mat4(const std::string& property_name, const Matrix4& m) const;
void set_float(const std::string& property_name, float f) const;
void use() const;

struct Image {
    int width;
    int height;
    unsigned char* image_data;

    explicit Image(const std::string& file_path, bool flip_vertical);
    Image(const Image& other) = delete;
    Image(const Image&& other) = delete;
    Image& operator=(Image& other) = delete;
    Image& operator=(Image&& other) = delete;
    ~Image();
};

class StaticRenderUnit {
    buffer_handle_t vao{ default_buffer_handle };
    buffer_handle_t vbo{ default_buffer_handle };
    buffer_handle_t ibo{ default_buffer_handle };
    tex_handle_t tex_handle{ default_tex_handle };
    size_t index_data_length{ 0 };

    static const Matrix4 perspective;

public:
    StaticRenderUnit(const Material& material, const ObjSubmodelData& obj_submodel_data, const ObjModelData& obj_data,
        const Vector3& position, const Vector3& rotation);
    ~StaticRenderUnit();

    void render() const;
};

struct DirectionalLight {
    std::unique_ptr<Shader> shader;
    buffer_handle_t fbo{ default_buffer_handle };
    Matrix4 view_proj{ 0 };
    tex_handle_t depth_tex_handle{ default_tex_handle };

    DirectionalLight() = default;
    explicit DirectionalLight(const DirectionalLightInfo& info);
    ~DirectionalLight();
};

struct PointLight {
    PointLightInfo base_info;
    PointLightInfo current_info;
    int index;
    std::unique_ptr<Shader> shader;

    PointLight(const PointLightInfo& point_light_info, int light_index);
    float wiggle_intensity(float dt);
};

struct Skybox {
    std::unique_ptr<Shader> shader;
    tex_handle_t cubemap_handle{ default_tex_handle };
    buffer_handle_t vao{ default_buffer_handle };
    buffer_handle_t vbo{ default_buffer_handle };

    Skybox() = default;
    explicit Skybox(const std::string& skybox_path, const Matrix4& projection);
    ~Skybox();
};

class Renderer {
    std::vector<std::unique_ptr<StaticRenderUnit>> render_units;
    std::unique_ptr<Shader> world_shader;
    std::unique_ptr<DirectionalLight> directional_light;
    std::unique_ptr<Skybox> skybox;

    std::vector<std::unique_ptr<PointLight>> point_lights;
    tex_handle_t point_light_cubemap_handle { default_tex_handle };
    buffer_handle_t point_light_fbo { default_buffer_handle};

    buffer_handle_t draw_fbo{ default_buffer_handle };
    tex_handle_t draw_tex_handle { default_tex_handle };
    buffer_handle_t draw_rbo{ default_buffer_handle };

    void register_static_obj(const ObjModelData& obj_data, const Vector3& position, const Vector3& rotation);
    void register_point_light(const PointLightInfo& point_light_info);
    void register_directional_light(const DirectionalLightInfo& directional_light_info);

public:
    Renderer();
    ~Renderer();

    void register_scene(const Scene& scene);
    void render(const Matrix4& player_view_matrix, float dt);

};


#endif


