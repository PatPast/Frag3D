
#include <SDL2/SDL_image.h>

#include <glad/glad.h>
#include <common.h>
#include <stdio.h>
#include "render.h"

staticRenderUnit_t* staticRenderUnit_init(material_t* material, objSubmodelData_t* obj_submodel_data, objModelData_t* obj_data,
    vector3_t position, vector3_t rotation) {
    float* vertex_data; //float
    int vertex_data_size;
    int* index_data; //float
    image_t* image;
    int i = 0;

    staticRenderUnit_t* sru = malloc(sizeof(staticRenderUnit_t));
    vertex_data_size = obj_submodel_data->faces->size * 24;
    vertex_data = malloc(sizeof(float) * vertex_data_size);
    

    list_foreach(face, obj_submodel_data->faces) {
        const float uv_scale = 0.1; // Makes the test environment look better
        objFaceData_t* face_data = (objFaceData_t*) face;

        vector3_t obj_p0 = list_elem(obj_data->position_data, face_data->position_indices[0]); //creer la methode list_elem(list_t*, indice)
        vector3_t p0 = vector3_add(vector3_rotate(obj_p0, rotation), position);
        vector2_t uv0 = vector2_mult(list_elem(obj_data->uv_data, face_data->uv_indices[0]), uv_scale);
        vector3_t obj_n0 = list_elem(obj_data->position_data, face_data->position_indices[0]);
        vector3_t n0 = vector3_rotate(obj_n0, rotation);
        float v0[8] = { p0.x, p0.y, p0.z, uv0.x, uv0.y, n0.x, n0.y, n0.z };

        vector3_t obj_p1 = list_elem(obj_data->position_data, face_data->position_indices[1]);
        vector3_t p1 = vector3_add(vector3_rotate(obj_p1, rotation), position);
        vector2_t uv1 = vector2_mult(list_elem(obj_data->uv_data, face_data->uv_indices[1]), uv_scale);
        vector3_t obj_n1 = list_elem(obj_data->position_data, face_data->position_indices[1]);
        vector3_t n1 = vector3_rotate(obj_n1, rotation);
        float v1[8] = { p1.x, p1.y, p1.z, uv1.x, uv1.y, n1.x, n1.y, n1.z };

        vector3_t obj_p2 = list_elem(obj_data->position_data, face_data->position_indices[2]); 
        vector3_t p2 = vector3_add(vector3_rotate(obj_p2, rotation), position);
        vector2_t uv2 = vector2_mult(list_elem(obj_data->uv_data, face_data->uv_indices[2]), uv_scale);
        vector3_t obj_n2 = list_elem(obj_data->position_data, face_data->position_indices[2]);
        vector3_t n2 = vector3_rotate(obj_n2, rotation);
        float v2[8] = { p2.x, p2.y, p2.z, uv2.x, uv2.y, n2.x, n2.y, n2.z };

        memcpy(vertex_data + (24 * i + 0), v0, sizeof(float) * 8);
        memcpy(vertex_data + (24 * i + 8), v1, sizeof(float) * 8);
        memcpy(vertex_data + (24 * i + 16), v2, sizeof(float) * 8);
        i++;

    }

    int vertex_count = obj_submodel_data->faces->size * 3;
    index_data = malloc(sizeof(int) * vertex_count);
    for (i = 0; i < vertex_count; i++) {
        index_data[i] = i;
    }
    
    sru->index_data_length = vertex_count;

    glGenBuffers(1, &(sru->vbo));
    glBindBuffer(GL_ARRAY_BUFFER, sru->vbo);
    glBufferData(GL_ARRAY_BUFFER, vertex_data_size * sizeof(float), vertex_data, GL_STATIC_DRAW);

    glGenVertexArrays(1, &(sru->vao));
    glBindVertexArray(sru->vao);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glBindBuffer(GL_ARRAY_BUFFER, sru->vbo);

    glGenBuffers(1, &(sru->ibo));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sru->ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sru->index_data_length * sizeof(int), index_data, GL_STATIC_DRAW);

    sru->tex_handle = 0;
    if (!material->diffuse_texture_name != NULL) { //et != de \0
        char* image_path[512] = "assets/textures/";
        strcat(image_path, material->diffuse_texture_name);

        image =  image_init(image_path);
        glGenTextures(1, &(sru->tex_handle));
        glBindTexture(GL_TEXTURE_2D, sru->tex_handle);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->width, image->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->image_data); //checker si ca marche ou pas

        image_destroy(&image);
    }
    free(vertex_data);
    free(index_data);
}

void staticRenderUnit_render(staticRenderUnit_t* sru){
    glBindVertexArray(sru->vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sru->ibo);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sru->tex_handle);
    glDrawElements(GL_TRIANGLES, (GLsizei)sru->index_data_length, GL_UNSIGNED_INT, NULL);
}

void staticRenderUnit_destroy(staticRenderUnit_t** sru) {
    glDeleteVertexArrays(1, &((*sru)->vao));
    glDeleteBuffers(1, &((*sru)->vbo));
    glDeleteBuffers(1, &((*sru)->ibo));
    free(*sru);
    *sru = NULL;
}

image_t* image_init(char* file_path) {
    image_t* img = malloc(sizeof(image_t));    
    SDL_Surface* data = IMG_Load(file_path);

    img->image_data = data->pixels;
    img->width = data->w;
    img->height = data->h;
    if (img->image_data == NULL) {
        printf("Couldn't load the image at: %s\n", file_path);
    }
    SDL_FreeSurface(img);
    return img;
}
void image_freealloc(image_t* img){ //modifier dans le render.h (ou supprimer)
    free(img->image_data);
}

void image_destroy(image_t** img) {
    image_freealloc(*img);
    free(*img);
    *img = NULL;
}
