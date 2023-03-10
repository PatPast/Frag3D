#include <glad/glad.h>
#include <common.h>
#include <render.h>
#include <string.h>
#include <assets.h>
//#include "render_debug.h"
#include <config.h>


size_t point_shadowmap_size = 1024;
size_t directional_shadowmap_size = 2048;
float near_plane = 0.001f;
float far_plane = 1000.0f;
float shadow_near_plane = 0.001f;
float shadow_far_plane = 1000.0f;
vector2i_t draw_framebuffer_size = {640, 360};
int window_width = 1080; //TODO
int window_height = 720; //les mettre dans un fichier de config en static
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

    check_gl_error("point_light_cubemap"); //TODO verifier si necessaire

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

    create_draw_fbo(rdr->draw_fbo, rdr->draw_tex_handle, rdr->draw_rbo);
    create_point_light_cubemap_and_fbo(rdr->point_light_cubemap_handle, rdr->point_light_fbo, max_point_light_count);

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
        worldspawnEntry_t* e = (worldspawnEntry_t*)entry;
        objModelData_t* obj_data = objModelData_load(e->obj_name); // TODO convertir en c
        renderer_register_static_obj(rdr, obj_data ,e->position, e->rotation);
        objModelData_destroy(&obj_data);
    }

    list_foreach(entry,scene->props) {
        propEntry_t* e = (propEntry_t*)entry;
        objModelData_t* obj_data = objModelData_load(e->obj_name); // TODO convertir en c
        renderer_register_static_obj(rdr, obj_data, e->position, e->rotation);
        objModelData_destroy(&obj_data);
    }

    list_foreach(entry,scene->point_light_info) {
        renderer_register_point_light(rdr, *(pointLightInfo_t*)entry);
    }

    register_directional_light(rdr, scene->directional_light_info);

}

void renderer_register_static_obj(renderer_t* rdr, objModelData_t* obj_data, vector3_t position, vector3_t rotation) {
    //TODO list_foreach imbrication erreur
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

void renderer_register_point_light(renderer_t* rdr, pointLightInfo_t point_light_info) {
    const int index_to_add = (int)(rdr->point_lights->size);
    pointLight_t* light = pointLight_init(point_light_info, index_to_add);
    pointLight_t* pl_at_index_to_add = (pointLight_t*)list_elem(rdr->point_lights, index_to_add); //le pointLight à la position 'index_to_add' de rdr->point_lights

    list_add(rdr->point_lights, light);
    free(light);

    shader_use(rdr->world_shader);
    shader_set_int(rdr->world_shader, "u_point_light_count", index_to_add + 1);

    char index_str[10];
    snprintf(index_str, 10, "%d", index_to_add);

    char pos_prop_name[50];
    snprintf(pos_prop_name, 50, "u_point_lights[%s].position", index_str);

    shader_set_vec3(rdr->world_shader, pos_prop_name, pl_at_index_to_add->base_info.position);

    char color_prop_name[50];
    snprintf(color_prop_name, 50, "u_point_lights[%s].color", index_str);
    shader_set_vec3(rdr->world_shader, color_prop_name, pl_at_index_to_add->base_info.color);

    char intensity_prop_name[50];
    snprintf(intensity_prop_name, 50, "u_point_lights[%s].intensity", index_str);
    shader_set_float(rdr->world_shader, intensity_prop_name, pl_at_index_to_add->base_info.intensity);


    char attenuation_prop_name[50];
    snprintf(intensity_prop_name, 50, "u_point_lights[%s].attenuation", index_str);
    shader_set_float(rdr->world_shader, attenuation_prop_name, pl_at_index_to_add->base_info.attenuation);
}

void renderer_register_directional_light(renderer_t* rdr, directionalLightInfo_t directional_light_info) {
    rdr->directional_light = directionalLight_init(directional_light_info);
    shader_use(rdr->world_shader );
    shader_set_vec3(rdr->world_shader, "u_directional_light_dir", directional_light_info.position);
    shader_set_vec3(rdr->world_shader, "u_directional_light_color", directional_light_info.color);
    shader_set_mat4(rdr->world_shader, "u_directional_light_vp", rdr->directional_light->view_proj);
}

void renderer_render(renderer_t* rdr, matrix4_t player_view_matrix, float dt) {
    
    // TODO @CLEANUP: Probably gonna look different when we go through the other light paramters
    list_foreach(point_light, rdr->point_lights) {
        pointLight_t* pl = (pointLight_t*)point_light;
        const float intensity = pointLight_wiggle_intensity(pl, dt);
        
        char intensity_prop_name[50];
        snprintf(intensity_prop_name, 50, "u_point_lights[%d].intensity", pl->index);
        shader_use(rdr->world_shader);
        shader_set_float(rdr->world_shader, intensity_prop_name, intensity);

    }
    
    // Directional shadow
    glViewport(0, 0, point_shadowmap_size, point_shadowmap_size);
    glDisable(GL_CULL_FACE); // Write to depth buffer with all faces. Otherwise the backfaces won't cause shadows
    glBindFramebuffer(GL_FRAMEBUFFER, rdr->directional_light->fbo);
    glClear(GL_DEPTH_BUFFER_BIT);
    shader_use(rdr->directional_light->shader);
    list_foreach(ru, rdr->render_units) {
        staticRenderUnit_render((staticRenderUnit_t*)ru);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Point shadow
    glBindFramebuffer(GL_FRAMEBUFFER, rdr->point_light_fbo);
    glClear(GL_DEPTH_BUFFER_BIT);
    list_foreach(point_light, rdr->point_lights) {
        shader_use(((pointLight_t*)point_light)->shader);
        list_foreach(ru, rdr->render_units) {
            staticRenderUnit_render((staticRenderUnit_t*)ru);
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glEnable(GL_CULL_FACE);

    // Draw to backbuffer
    glBindFramebuffer(GL_FRAMEBUFFER, rdr->draw_fbo);
    shader_use(rdr->world_shader);
    shader_set_mat4(rdr->world_shader, "u_view", player_view_matrix);
    glViewport(0, 0, draw_framebuffer_size.x, draw_framebuffer_size.y);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    shader_use(rdr->world_shader);
    shader_set_mat4(rdr->world_shader, "u_view", player_view_matrix);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, rdr->directional_light->depth_tex_handle);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, rdr->point_light_cubemap_handle);
    list_foreach(ru, rdr->render_units) {
        staticRenderUnit_render((staticRenderUnit_t*)ru);
    }

    // Skybox: fill fragments with depth == 1
    shader_use(rdr->skybox->shader);
    glDepthFunc(GL_LEQUAL);
    matrix4_t skybox_view = player_view_matrix;
    skybox_view.data[3 * 4 + 0] = 0.0f; // Clear translation row
    skybox_view.data[3 * 4 + 1] = 0.0f;
    skybox_view.data[3 * 4 + 2] = 0.0f;
    shader_set_mat4(rdr->skybox->shader, "u_view", skybox_view);
    glBindVertexArray(rdr->skybox->vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, rdr->skybox->cubemap_handle);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS);

    // Blit to screen
    glBindFramebuffer(GL_READ_FRAMEBUFFER, rdr->draw_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, draw_framebuffer_size.x, draw_framebuffer_size.y, 0, 0, window_width, window_height,
        GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
}

void renderer_freealloc(renderer_t* rdr){
    glDeleteTextures(1, rdr->point_light_cubemap_handle);
    glDeleteFramebuffers(1, rdr->point_light_fbo);

    glDeleteTextures(1, rdr->draw_tex_handle);
    glDeleteFramebuffers(1, rdr->draw_fbo);
    glDeleteRenderbuffers(1, rdr->draw_rbo);

    //TODO a modifier
    directionalLight_destroy(&rdr->directional_light);
    skybox_destroy(&rdr->skybox);
    list_foreach(ru,rdr->render_units){
        staticRenderUnit_freealloc((staticRenderUnit_t*)ru);
    }
    list_destroy(&rdr->render_units);
    list_foreach(pl,rdr->point_lights){
        pointLight_freealloc((pointLight_t*)pl);
    }
    list_destroy(&rdr->point_lights);
}

void renderer_destroy(renderer_t** rdr) {
    renderer_freealloc(*rdr);
    free(*rdr);
    *rdr = NULL;
   
}
