#include <stdio.h>
#include <stdlib.h>
#include <glad/glad.h>
#include <render.h>
//#include "render_debug.h"
#define BUFFER_SIZE 512

shader_handle_t invalid_shader = 0;

char* file_to_string(FILE* fp){
    char *buffer;
    long file_size;

    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    rewind(fp);

    buffer = malloc(file_size * sizeof(char));
    if (buffer == NULL) {
        printf("Erreur d'allocation de mémoire\n");
        exit(1);
    }

    fread(buffer, sizeof(char), file_size, fp);

}

shader_handle_t load_shader(char* header, char* program_string, int shader_type) {
    char* shader_string = malloc(sizeof(char) * (strlen(header) + strlen(program_string) + 1));
    strcpy(shader_string, header);
    strcat(shader_string, program_string);

    const shader_handle_t shader_id = glCreateShader(shader_type);
    glShaderSource(shader_id, 1, &shader_string, NULL);
    glCompileShader(shader_id);

    // Check for compile errors
    GLint is_success;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &is_success);
    if (is_success == GL_FALSE) {
        
        int log_length;
        glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

        char* log = malloc(sizeof(char) * (log_length + 1));
        glGetShaderInfoLog(shader_id, log_length, &log_length, log);

        fprintf(stderr, "Failed to compile shader : %s", log);
        
        free(log);
        glDeleteShader(shader_id);
        return invalid_shader;
    }

    return shader_id;
}

shader_t shader_init(char* file_path) {
    shader_t sh = {invalid_shader};
    char buffer[BUFFER_SIZE];

    FILE* shader_file = fopen(file_path, "r");

    if (shader_file == NULL) {
        fprintf(stderr, "Impossible d'ouvrir le fichier de shader : '%s'\n", shader_file);
        return sh;
    }

    char* program_string = file_to_string(shader_file); //malloc
    fclose(shader_file);

    shader_handle_t vert_shader = load_shader("#version 450\n#define VERTEX\n", program_string, GL_VERTEX_SHADER);
    shader_handle_t frag_shader = load_shader("#version 450\n#define FRAGMENT\n", program_string, GL_FRAGMENT_SHADER);
    shader_handle_t geom_shader = invalid_shader;
    
    int has_geom_shader = strstr(program_string, "#ifdef GEOMETRY") != NULL;
    if (has_geom_shader) {
        geom_shader = load_shader("#version 450\n#define GEOMETRY\n", program_string, GL_GEOMETRY_SHADER);
    }

    free(program_string);

    if (vert_shader == invalid_shader || frag_shader == invalid_shader
         && (has_geom_shader && geom_shader == invalid_shader)) {
        return sh; // The game is gonna die anyway
    }

    shader_handle_t shader_program = glCreateProgram();
    glAttachShader(shader_program, frag_shader);
    glAttachShader(shader_program, vert_shader);
    if (has_geom_shader && geom_shader != invalid_shader) {
        glAttachShader(shader_program, geom_shader);
    }
    glLinkProgram(shader_program);

    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader);
    if (has_geom_shader && geom_shader != invalid_shader) {
        glDeleteShader(geom_shader);
    }

    // Check for link errors
    int is_success;
    glGetProgramiv(shader_program, GL_LINK_STATUS, &is_success);
    if (is_success == GL_FALSE) {

        GLint log_length;
        glGetShaderiv(shader_program, GL_INFO_LOG_LENGTH, &log_length);

        char* log = malloc(sizeof(char) * (log_length + 1));
        glGetShaderInfoLog(shader_program, log_length, &log_length, log);

        fprintf(stderr, "Shader link error: %s\n", log);

        free(log);
        glDeleteShader(shader_program);
        return sh;
    }

    sh.shader_program_handle = shader_program;
    check_gl_error("shader_init");

    return sh;
}

void shader_use(shader_t sh) {
    glUseProgram(sh.shader_program_handle);
}

uniform_loc_t shader_get_location(shader_t sh, char* property_name) {
    // TODO @PERF: These locations don't change once the program is linked
    const uniform_loc_t loc = glGetUniformLocation(sh.shader_program_handle, property_name);
    if (loc == -1) {
        fprintf(stderr, "Error when getting shader property location: %s\n", property_name);
    }
    return loc;
}

void check_uniform_error(char* property_name) {
    const GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        fprintf(stderr, "Error when setting uniform [%d] error code: ", error);
    }
}

void shader_set_int(shader_t sh, char* property_name, int i){
    glUniform1i(shader_get_location(sh, property_name), i);
    check_uniform_error(property_name);
}

void shader_set_vec3(shader_t sh, char* property_name, vector3_t v){
    glUniform3f(shader_get_location(sh, property_name), v.x, v.y, v.z);
    check_uniform_error(property_name);
}

void shader_set_mat4(shader_t sh, char* property_name, matrix4_t m){
    glUniformMatrix4fv(shader_get_location(sh, property_name), 1, GL_FALSE, m.data);
    check_uniform_error(property_name);
}

void shader_set_float(shader_t sh, char* property_name, float f){
    glUniform1f(shader_get_location(sh, property_name), f);
    check_uniform_error(property_name);
}

void shader_freealloc(shader_t sh) {
    glDeleteProgram(sh.shader_program_handle);
    sh.shader_program_handle = 0;
}

void shader_destroy(shader_t* sh) {
    shader_freealloc(*sh); //traitement de la structure des shader très simple 
}
