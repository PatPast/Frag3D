#include <glad/glad.h>
#include <common.h>
#include <render.h>
//#include "render_debug.h"
#include "../config.h"

size_t point_shadowmap_size = 1024;
size_t directional_shadowmap_size = 2048;
float near_plane = 0.001f;
float far_plane = 1000.0f;
float shadow_near_plane = 0.001f;
float shadow_far_plane = 1000.0f;
vector2i_t draw_framebuffer_size = {640, 360};
int window_width = 1080, window_height = 720;
int max_point_light_count = 10; // TODO @CLEANUP: We have the same define in the world shader


#define aspect_ratio ((float)window_width / (float)window_height)
#define MATRIX4_PERSPECTIVE matrix4_perspective(45.0f, aspect_ratio, near_plane, far_plane)

void create_draw_fbo(buffer_handle_t draw_fbo, tex_handle_t draw_tex_handle, buffer_handle_t draw_rbo) {

    glGenTextures(1, draw_tex_handle);
    glBindTexture(GL_TEXTURE_2D, draw_tex_handle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, draw_framebuffer_size.x, draw_framebuffer_size.y,
        0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glGenFramebuffers(1, draw_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, draw_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, draw_tex_handle, 0);

    glGenRenderbuffers(1, draw_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, draw_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, draw_framebuffer_size.x, draw_framebuffer_size.y);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, draw_rbo);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void create_point_light_cubemap_and_fbo(tex_handle_t cubemap_handle, buffer_handle_t fbo, int point_light_count) {
    
    glGenTextures(1, cubemap_handle);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, cubemap_handle);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, GL_DEPTH_COMPONENT, point_shadowmap_size, point_shadowmap_size,
        6 * point_light_count, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, 0);

    check_gl_error("point_light_cubemap");

    glGenFramebuffers(1, fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, cubemap_handle, 0);
    glReadBuffer(GL_NONE);
    glDrawBuffer(GL_NONE);
    check_gl_framebuffer_complete("point_light_fbo");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

renderer_t* renderer_init() {
    renderer_t* rdr;
    glewInit(); // Needs to be after the glfw context creation
    // TODO @CLEANUP: This looks stupid. We're doing this stuff on a variable declaration in main()
    // Would look better in an Init function or something.
    glViewport(0, 0, window_width, window_height);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthFunc(GL_LESS);

    rdr = malloc(sizeof(renderer_t));

    create_draw_fbo(this->draw_fbo, this->draw_tex_handle, this->draw_rbo);
    create_point_light_cubemap_and_fbo(this->point_light_cubemap_handle, this->point_light_fbo, max_point_light_count);

    rdr->world_shader = shader_init("src/render/shader/world.glsl");
    shader_use(rdr->world_shader);
    shader_set_int(rdr->world_shader, "u_texture", 0);
    shader_set_mat4(rdr->world_shader, "u_projection", MATRIX4_PERSPECTIVE);

    // TODO @CLEANUP: Currently the render units are actually static render units
    // We change the mesh data itself to apply the initial transformation
    // For dynamic objects, we'll have to use this uniform to transform them every frame
    shader_set_mat4(rdr->world_shader, "u_model", MATRIX4_IDENTITY);
                      
    shader_set_int(rdr->world_shader, "u_shadowmap_directional", 1);
    shader_set_int(rdr->world_shader, "u_shadowmaps_point", 2);
    shader_set_float(rdr->world_shader, "u_far_plane", shadow_far_plane);

    rdr->skybox = skybox_init("assets/skybox/gehenna", MATRIX4_PERSPECTIVE);
}

void renderer_register_scene(renderer_t* rdr, scene_t* scene) {

    list_foreach(entry,scene->worldspawn) {
        renderer_register_static_obj(obj_data, ((worldspawnEntry_t*)entry)->position, ((worldspawnEntry_t*)entry)->rotation);
    }

    list_foreach(entry,scene->props) {
        renderer_register_static_obj(obj_data, ((propEntry_t*)entry)->position, ((propEntry_t*)entry)->rotation);
    }

    list_foreach(entry,scene->point_light_info) {
        renderer_register_point_light((pointLightInfo_t*)entry);
    }

    register_directional_light(scene.directional_light_info);

}

void renderer_register_static_obj(renderer_t* rdr, objModelData_t* obj_data, vector3_t position, vector3_t rotation) {
    list_foreach(obj_face_data, obj_data->submodel_data) {
        list_foreach(m, obj_data->materials) {
            if (strcmp(((material_t*)m)->name, ((objSubmodelData_t*)obj_face_data)->material_name) == 0) {
                staticRenderUnit_t* ru = staticRenderUnit_init((material_t*)m, (objSubmodelData_t*)obj_face_data, obj_data, position, rotation);
                list_add(rdr->render_units,ru);
                free(ru);
            }
        }
    }
}

void Renderer::register_point_light(const PointLightInfo& point_light_info) {
    const int index_to_add = static_cast<int>(this->point_lights.size());
    std::unique_ptr<PointLight> light = std::make_unique<PointLight>(point_light_info, index_to_add);

    this->point_lights.push_back(std::move(light));

    this->world_shader->use();
    this->world_shader->set_int("u_point_light_count", index_to_add + 1);

    const std::string index_str = std::to_string(index_to_add);
    const std::string pos_prop_name = "u_point_lights[" + index_str + "].position";
    this->world_shader->set_vec3(pos_prop_name, this->point_lights[index_to_add]->base_info.position);

    const std::string color_prop_name = "u_point_lights[" + index_str + "].color";
    this->world_shader->set_vec3(color_prop_name, this->point_lights[index_to_add]->base_info.color);

    const std::string intensity_prop_name = "u_point_lights[" + index_str + "].intensity";
    this->world_shader->set_float(intensity_prop_name, this->point_lights[index_to_add]->base_info.intensity);

    const std::string att_prop_name = "u_point_lights[" + index_str + "].attenuation";
    this->world_shader->set_float(att_prop_name, this->point_lights[index_to_add]->base_info.attenuation);
}

void Renderer::register_directional_light(const DirectionalLightInfo& directional_light_info) {
    this->directional_light = std::make_unique<DirectionalLight>(directional_light_info);
    this->world_shader->use();
    this->world_shader->set_vec3("u_directional_light_dir", directional_light_info.position);
    this->world_shader->set_vec3("u_directional_light_color", directional_light_info.color);
    this->world_shader->set_mat4("u_directional_light_vp", this->directional_light->view_proj);
}

void Renderer::render(const Matrix4& player_view_matrix, float dt) {

    // TODO @CLEANUP: Probably gonna look different when we go through the other light paramters
    for (auto& point_light : this->point_lights) {
        const float intensity = point_light->wiggle_intensity(dt);

        const std::string intensity_prop_name = "u_point_lights[" + std::to_string(point_light->index) + "].intensity";
        this->world_shader->use();
        this->world_shader->set_float(intensity_prop_name, intensity);
    }
    
    // Directional shadow
    glViewport(0, 0, point_shadowmap_size, point_shadowmap_size);
    glDisable(GL_CULL_FACE); // Write to depth buffer with all faces. Otherwise the backfaces won't cause shadows
    glBindFramebuffer(GL_FRAMEBUFFER, this->directional_light->fbo);
    glClear(GL_DEPTH_BUFFER_BIT);
    this->directional_light->shader->use();
    for (auto& ru : this->render_units) {
        ru->render();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Point shadow
    glBindFramebuffer(GL_FRAMEBUFFER, this->point_light_fbo);
    glClear(GL_DEPTH_BUFFER_BIT);
    for (auto& point_light : this->point_lights) {
        point_light->shader->use();
        for (auto& ru : this->render_units) {
            ru->render();
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glEnable(GL_CULL_FACE);

    // Draw to backbuffer
    glBindFramebuffer(GL_FRAMEBUFFER, this->draw_fbo);
    this->world_shader->use();
    this->world_shader->set_mat4("u_view", player_view_matrix);
    glViewport(0, 0, draw_framebuffer_size.x, draw_framebuffer_size.y);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    this->world_shader->use();
    this->world_shader->set_mat4("u_view", player_view_matrix);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, this->directional_light->depth_tex_handle);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, this->point_light_cubemap_handle);
    for (auto& ru : this->render_units) {
        ru->render();
    }

    // Skybox: fill fragments with depth == 1
    this->skybox->shader->use();
    glDepthFunc(GL_LEQUAL);
    Matrix4 skybox_view = player_view_matrix;
    skybox_view.data[3 * 4 + 0] = 0.0f; // Clear translation row
    skybox_view.data[3 * 4 + 1] = 0.0f;
    skybox_view.data[3 * 4 + 2] = 0.0f;
    this->skybox->shader->set_mat4("u_view", skybox_view);
    glBindVertexArray(this->skybox->vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, this->skybox->cubemap_handle);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS);

    // Blit to screen
    glBindFramebuffer(GL_READ_FRAMEBUFFER, this->draw_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, draw_framebuffer_size.x, draw_framebuffer_size.y, 0, 0, window_width, window_height,
        GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
}

void renderer_freealloc(renderer_t* rdr){
    list_destroy(&rdr->render_units);
    list_destroy(&rdr->point_lights);
}

void renderer_destroy(renderer_t** rdr) {
    glDeleteTextures(1, (*rdr)->point_light_cubemap_handle);
    glDeleteFramebuffers(1, (*rdr)->point_light_fbo);

    glDeleteTextures(1, (*rdr)->draw_tex_handle);
    glDeleteFramebuffers(1, (*rdr)->draw_fbo);
    glDeleteRenderbuffers(1, (*rdr)->draw_rbo);

    renderer_freealloc(*rdr);
    free(*rdr);
    *rdr = NULL;
   
}
