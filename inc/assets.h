#ifndef _ASSETS_H_
#define _ASSETS_H_


#include <common.h>
#include <string.h>
#include <stdlib.h>
//#include "vector3.h"

typedef struct objFaceData_s {
    size_t position_indices[3];
    size_t uv_indices[3];
    size_t normal_indices[3];
}objFaceData_t;

typedef struct objSubmodelData_s {
    char* material_name;
    list_t* faces; //liste d'objFaceData
    
}objSubmodelData_t;

void objSubmodelData_freealloc(objSubmodelData_t* data);

typedef struct material_s {
    char* name;
    char* diffuse_texture_name;
    float diffuse[3];
    float transparency;
}material_t;

void material_freealloc(material_t* m);

typedef struct objModelData_s {
    list_t* materials; //material_t

    list_t* position_data; //vector3
    list_t* uv_data; //vector2
    list_t* normal_data; //vector3

    list_t* submodel_data; //objSubmodelData_t

}objModelData_t;

objModelData_t* objModelData_load(const char* file_path);
void objModelData_freealloc(objModelData_t* data);

typedef struct WorldspawnEntry_s {
    char* obj_name;
    float position[3];
    float rotation[3];
}worldspawnEntry_t;

typedef struct propEntry_s {
    char* obj_name;
    float position[3];
    float rotation[3];
}propEntry_t;

typedef struct pointLightInfo_s {
    float position[3];
    float color[3];
    float intensity;
    float attenuation;
}pointLightInfo_t;

typedef struct directionalLightInfo_s {
    float position[3];
    float color[3];
}directionalLightInfo_t;

typedef struct scene_s {
    float player_start[3];
    float player_lookat[3];
    list_t* worldspawn; //worldspawnEntry_t
    list_t* props; //propEntry_t
    list_t* point_light_info; //pointLightInfo_t
    list_t* directional_light_info; //pointLightInfo_t
}scene_t;

scene_t read_scene(const char* file_path);

#endif