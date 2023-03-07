
#pragma warning(push, 0) // Hide warnings from the external dependency
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#pragma warning(pop)

#define GLEW_STATIC
#include <glad/glad.h>
#include <stdio.h>
#include "render.h"

StaticRenderUnit::StaticRenderUnit(const Material& material, const ObjSubmodelData& obj_submodel_data, const ObjModelData& obj_data, 
                                   const Vector3& position, const Vector3& rotation) {
    std::vector<float> vertex_data;
    vertex_data.reserve(obj_submodel_data.faces.size() * 24);

    for (const ObjFaceData& face_data : obj_submodel_data.faces) {
        const float uv_scale = 0.1f; // Makes the test environment look better

        const Vector3& obj_p0 = obj_data.position_data[face_data.position_indices[0]];
        const Vector3 p0 = Vector3::rotate(obj_p0, rotation) + position;
        const Vector2& uv0 = obj_data.uv_data[face_data.uv_indices[0]] * uv_scale;
        const Vector3& obj_n0 = obj_data.normal_data[face_data.normal_indices[0]];
        const Vector3 n0 = Vector3::rotate(obj_n0, rotation);
        vertex_data.insert(vertex_data.end(), { p0.x, p0.y, p0.z, uv0.x, uv0.y, n0.x, n0.y, n0.z });

        const Vector3& obj_p1 = obj_data.position_data[face_data.position_indices[1]];
        const Vector3 p1 = Vector3::rotate(obj_p1, rotation) + position;
        const Vector2& uv1 = obj_data.uv_data[face_data.uv_indices[1]] * uv_scale;
        const Vector3& obj_n1 = obj_data.normal_data[face_data.normal_indices[1]];
        const Vector3 n1 = Vector3::rotate(obj_n1,rotation);
        vertex_data.insert(vertex_data.end(), { p1.x, p1.y, p1.z, uv1.x, uv1.y, n1.x, n1.y, n1.z });

        const Vector3& obj_p2 = obj_data.position_data[face_data.position_indices[2]];
        const Vector3 p2 = Vector3::rotate(obj_p2, rotation) + position;
        const Vector2& uv2 = obj_data.uv_data[face_data.uv_indices[2]] * uv_scale;
        const Vector3& obj_n2 = obj_data.normal_data[face_data.normal_indices[2]];
        const Vector3 n2 = Vector3::rotate(obj_n2, rotation);
        vertex_data.insert(vertex_data.end(), { p2.x, p2.y, p2.z, uv2.x, uv2.y, n2.x, n2.y, n2.z });
    }

    std::vector<int> index_data;
    const size_t vertex_count = obj_submodel_data.faces.size() * 3;
    index_data.reserve(vertex_count);
    for (size_t i = 0; i < vertex_count; i++) {
        index_data.push_back(static_cast<int>(i));
    }
    
    this->index_data_length = vertex_count;

    glGenBuffers(1, &(this->vbo));
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glBufferData(GL_ARRAY_BUFFER, vertex_data.size() * sizeof(float), vertex_data.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &(this->vao));
    glBindVertexArray(this->vao);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(5 * sizeof(float)));
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glGenBuffers(1, &(this->ibo));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->index_data_length * sizeof(int), index_data.data(), GL_STATIC_DRAW);

    this->tex_handle = default_tex_handle;
    if (!material.diffuse_texture_name.empty()) {
        const std::string image_path = "assets/textures/" + material.diffuse_texture_name;
        const Image image(image_path, true);
        glGenTextures(1, &(this->tex_handle));
        glBindTexture(GL_TEXTURE_2D, this->tex_handle);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.image_data);
    }
}

void StaticRenderUnit::render() const {
    glBindVertexArray(this->vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ibo);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, this->tex_handle);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(this->index_data_length), GL_UNSIGNED_INT, nullptr);
}

StaticRenderUnit::~StaticRenderUnit() {
    glDeleteVertexArrays(1, &(this->vao));
    glDeleteBuffers(1, &(this->vbo));
    glDeleteBuffers(1, &(this->ibo));
}

Image::Image(const std::string& file_path, bool flip_vertical) {
    stbi_set_flip_vertically_on_load(flip_vertical);
    int x, y, n;
    this->image_data = stbi_load(file_path.c_str(), &x, &y, &n, 0);
    this->width = x;
    this->height = y;
    if (image_data == nullptr) {
        std::cout << "Couldn't load the image at: " << file_path << std::endl;
    }
}

Image::~Image() {
    stbi_image_free(this->image_data);
}
