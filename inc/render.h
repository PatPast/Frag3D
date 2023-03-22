#ifndef _RENDER_H_
#define _RENDER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assets.h>
#include <vector3.h>
#include <common.h>


typedef int uniform_loc_t;
typedef unsigned int shader_handle_t;
typedef unsigned int buffer_handle_t;
typedef unsigned int tex_handle_t;

typedef struct shader_s {
    shader_handle_t shader_program_handle;
}shader_t;

shader_t shader_init(char* file_path);
void shader_freealloc(shader_t sh);
void shader_destroy(shader_t* sh);
void shader_set_int(shader_t sh, char* property_name, int i);
void shader_set_vec3(shader_t sh, char* property_name, vector3_t v);
void shader_set_mat4(shader_t sh, char* property_name, matrix4_t m);
void shader_set_float(shader_t sh, char* property_name, float f);
void shader_use(shader_t sh);
void shader_print(shader_t sh);

typedef struct image_s {
    int width;
    int height;
    uint32_t* image_data; 
}image_t;

image_t* image_init(char* file_path);
void image_freealloc(image_t* img);
void image_destroy(image_t** img);


typedef struct staticRenderUnit_s {
    buffer_handle_t vao;
    buffer_handle_t vbo;
    buffer_handle_t ibo;
    tex_handle_t tex_handle;
    size_t index_data_length;
}staticRenderUnit_t;


staticRenderUnit_t* staticRenderUnit_init(material_t* material, objSubmodelData_t* obj_submodel_data, objModelData_t* obj_data, vector3_t position, vector3_t rotation);
void staticRenderUnit_render(staticRenderUnit_t* sru);
void staticRenderUnit_freealloc(staticRenderUnit_t* sru);
void staticRenderUnit_destroy(staticRenderUnit_t** sru);
void staticRenderUnit_print(staticRenderUnit_t* sru);

void render();

typedef struct directionalLight_s {
    shader_t shader;
    buffer_handle_t fbo;
    matrix4_t view_proj;
    tex_handle_t depth_tex_handle;

}directionalLight_t;

directionalLight_t* directionalLight_init(directionalLightInfo_t info);
void directionalLight_destroy(directionalLight_t** dl);
void directionalLight_print(directionalLight_t* dl);

typedef struct pointLight_s {
    pointLightInfo_t base_info;
    pointLightInfo_t current_info;
    int index;
    shader_t shader;

}pointLight_t;


pointLight_t* pointLight_init(pointLightInfo_t point_light_info, int light_index);
void pointLight_freealloc(pointLight_t* pl);
void pointLight_destroy(pointLight_t ** pl);
float pointLight_wiggle_intensity(pointLight_t* pl, float dt);
void pointLight_print(pointLight_t* pl);

typedef struct skybox_s {
    shader_t shader;
    tex_handle_t cubemap_handle;
    buffer_handle_t vao;
    buffer_handle_t vbo;

}skybox_t;

skybox_t* skybox_init(char* skybox_path, matrix4_t projection);
void skybox_destroy(skybox_t** sb);
void skybox_print(skybox_t* sb);

typedef struct renderer_s {
    list_t* render_units; //staticRenderUnit
    shader_t world_shader;
    directionalLight_t* directional_light;
    skybox_t* skybox;

    list_t* point_lights; //pointLight
    tex_handle_t point_light_cubemap_handle;
    buffer_handle_t point_light_fbo;

    buffer_handle_t draw_fbo;
    tex_handle_t draw_tex_handle;
    buffer_handle_t draw_rbo;
    
}renderer_t;

void renderer_register_static_obj(renderer_t* rdr, objModelData_t* obj_data, vector3_t position, vector3_t rotation);
void renderer_register_point_light(renderer_t* rdr, pointLightInfo_t point_light_info);
void renderer_register_directional_light(renderer_t* rdr, directionalLightInfo_t directional_light_info);
renderer_t* renderer_init();
void renderer_freealloc(renderer_t* rdr);
void renderer_destroy(renderer_t** rdr);
void renderer_register_scene(renderer_t* rdr, scene_t* scene);
void renderer_render(renderer_t* rdr, matrix4_t player_view_matrix, float dt);
void renderer_print(renderer_t* rdr);

#endif


