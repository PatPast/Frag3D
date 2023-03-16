#include <glad/glad.h>
#include <render.h>

float skybox_vertex_data[108] = {
    -1.0, 1.0, -1.0, -1.0, -1.0, -1.0, 1.0, -1.0, -1.0, 1.0, -1.0, -1.0, 1.0, 1.0, -1.0, -1.0, 1.0,
    -1.0, -1.0, -1.0, 1.0, -1.0, -1.0, -1.0, -1.0, 1.0, -1.0, -1.0, 1.0, -1.0, -1.0, 1.0, 1.0,
    -1.0, -1.0, 1.0, 1.0, -1.0, -1.0, 1.0, -1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, -1.0,
    1.0, -1.0, -1.0, -1.0, -1.0, 1.0, -1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, -1.0, 1.0,
    -1.0, -1.0, 1.0, -1.0, 1.0, -1.0, 1.0, 1.0, -1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, -1.0, 1.0, 1.0,
    -1.0, 1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, 1.0, 1.0, -1.0, -1.0, 1.0, -1.0, -1.0, -1.0,
    -1.0, 1.0, 1.0, -1.0, 1.0
};

void load_skybox_face(char* face_path, GLuint skybox_side) {
    image_t* image = image_init(face_path);
    glTexImage2D(skybox_side, 0, GL_RGBA, image->width, image->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->image_data);
    image_destroy(&image);
}

skybox_t* skybox_init(char* skybox_path, matrix4_t projection) {
    skybox_t* sb = malloc(sizeof(skybox_t));
    char sb_png_path[256];
    glGenBuffers(1, &sb->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, sb->vbo);
    glBufferData(GL_ARRAY_BUFFER, 108 * sizeof(float), skybox_vertex_data, GL_STATIC_DRAW);

    glGenVertexArrays(1, &sb->vao);
    glBindVertexArray(sb->vao);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), NULL);
    glBindBuffer(GL_ARRAY_BUFFER, sb->vbo);

    sb->cubemap_handle = 0;
    glGenTextures(1, &sb->cubemap_handle);
    glBindTexture(GL_TEXTURE_CUBE_MAP, sb->cubemap_handle);
    
    strcpy(sb_png_path, skybox_path);
    strcat(sb_png_path, "_front.png");
    load_skybox_face(sb_png_path, GL_TEXTURE_CUBE_MAP_POSITIVE_Z);

    strcpy(sb_png_path, skybox_path);
    strcat(sb_png_path, "_back.png");
    load_skybox_face(sb_png_path, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);

    strcpy(sb_png_path, skybox_path);
    strcat(sb_png_path, "_left.png");
    load_skybox_face(sb_png_path, GL_TEXTURE_CUBE_MAP_NEGATIVE_X);

    strcpy(sb_png_path, skybox_path);
    strcat(sb_png_path, "_right.png");
    load_skybox_face(sb_png_path, GL_TEXTURE_CUBE_MAP_POSITIVE_X);

    strcpy(sb_png_path, skybox_path);
    strcat(sb_png_path, "_top.png");
    load_skybox_face(sb_png_path, GL_TEXTURE_CUBE_MAP_POSITIVE_Y);

    strcpy(sb_png_path, skybox_path);
    strcat(sb_png_path, "_bottom.png");
    load_skybox_face(sb_png_path, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    sb->shader = shader_init("src/shader/skybox.glsl");
    shader_use(sb->shader);
    shader_set_mat4(sb->shader, "u_projection", projection);
    shader_set_int(sb->shader, "u_skybox", 0);

    return sb;
}

void skybox_destroy(skybox_t** sb) {
    shader_destroy(&(*sb)->shader);
    glDeleteTextures(1, &(*sb)->cubemap_handle);
    glDeleteBuffers(1, &(*sb)->vao);
    glDeleteBuffers(1, &(*sb)->vbo);
    free(*sb);
    *sb = NULL;
}