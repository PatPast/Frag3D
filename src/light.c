#include <render.h>
#include <render_debug.h>
#include <stdlib.h>
#include <common.h>
float shadow_near_plane = 0.001f;
float shadow_far_plane = 1000.0f;
size_t point_shadowmap_size = 1024;
size_t directional_shadowmap_size = 2048;



pointLight_t* pointLight_init(pointLightInfo_t point_light_info, int light_index){

    pointLight_t* pl = malloc(sizeof(pointLight_t));
    pl->base_info = point_light_info;
    pl->current_info = point_light_info;
    pl->index = light_index;
    pl->shader = shader_init("../src/shader/shadowmap_depth_point.glsl");
    shader_use(pl->shader);


    matrix4_t proj = matrix4_perspective(90.0f, 1.0f, shadow_near_plane, shadow_far_plane);

    const vector3_t dirs[6] = { VECTOR3_RIGHT, VECTOR3_LEFT, VECTOR3_UP, VECTOR3_DOWN, VECTOR3_FORWARD, VECTOR3_BACK };
    const vector3_t ups[6] = { VECTOR3_DOWN, VECTOR3_DOWN, VECTOR3_FORWARD, VECTOR3_BACK, VECTOR3_DOWN, VECTOR3_DOWN };
    for (int i = 0; i < 6; i++) {
        matrix4_t view = matrix4_look_at(pl->base_info.position, vector3_add(pl->base_info.position, dirs[i]), ups[i]);
        
        char mat_prop_name[50];
        snprintf(mat_prop_name, 50, "u_shadow_matrices[%d]", i);
        shader_set_mat4(pl->shader, mat_prop_name, matrix4_mult(proj, view));

    }

    shader_set_float(pl->shader, "u_far_plane", shadow_far_plane);
    shader_set_vec3(pl->shader, "u_light_pos", pl->base_info.position);
    shader_set_int(pl->shader, "u_light_index", pl->index);
    shader_set_mat4(pl->shader, "u_model", MATRIX4_IDENTITY);

    check_gl_error("point_light_ctor");

    return pl;
}

void pointLight_freealloc(pointLight_t* pl){
    shader_destroy(&pl->shader);
}

void pointLight_destroy(pointLight_t** pl){
    pointLight_freealloc(*pl);
    free(*pl);
    *pl = NULL; 
}


float pointLight_wiggle_intensity(pointLight_t* pl, float dt) {
    
    const float range = 500.0f;
    const float perc = (float)rand()/((float)RAND_MAX/range); // valeur flottante aleatoir entre 0 et range
    const float target_intensity = pl->base_info.intensity + (pl->base_info.intensity * (perc / 100.0f));
    const float new_current = lerp(pl->current_info.intensity, target_intensity, dt * 10);
    pl->current_info.intensity = new_current;

    return new_current;
}

directionalLight_t* directionalLight_init(directionalLightInfo_t info){
    directionalLight_t* dl = malloc(sizeof(directionalLight_t));

    float s = 100.0f; // Ortho volume size
    const matrix4_t proj = matrix4_ortho(-s, s, -s, s, shadow_near_plane, shadow_far_plane);
    const matrix4_t view = matrix4_look_at(info.position, VECTOR3_ZERO, VECTOR3_UP);
    dl->view_proj = matrix4_mult(proj, view);

    dl->shader = shader_init("../src/shader/shadowmap_depth_directional.glsl");
    shader_use(dl->shader);
    shader_set_mat4(dl->shader, "u_light_vp", dl->view_proj);
    shader_set_mat4(dl->shader, "u_model", MATRIX4_IDENTITY);

    glGenTextures(1, &dl->depth_tex_handle);
    glBindTexture(GL_TEXTURE_2D, dl->depth_tex_handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    float border_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, directional_shadowmap_size, directional_shadowmap_size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    dl->fbo = 0;
    glGenFramebuffers(1, &dl->fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, dl->fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, dl->depth_tex_handle, 0);
    glReadBuffer(GL_NONE);
    glDrawBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    check_gl_error("directional_light");

    return dl;
}

void directionalLight_freealloc(directionalLight_t* dl){
    shader_destroy(&dl->shader);
    glDeleteFramebuffers(1, &dl->fbo);
    glDeleteTextures(1, &dl->depth_tex_handle);
}

void directionalLight_destroy(directionalLight_t** dl) {
    directionalLight_freealloc(*dl);
    free(*dl);
    *dl = NULL;
}