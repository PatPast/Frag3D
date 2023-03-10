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

void objSubmodelData_freealloc(objSubmodelData_t* submodel);

typedef struct material_s {
    char* name;
    char* diffuse_texture_name;
    vector3_t diffuse;
    float transparency;
}material_t;

void material_freealloc(material_t* material);

typedef struct objModelData_s {
    list_t* materials; //material_t

    list_t* position_data; //vector3
    list_t* uv_data; //vector2
    list_t* normal_data; //vector3

    list_t* submodel_data; //objSubmodelData_t

}objModelData_t;

objModelData_t* objModelData_load(const char* file_path);
void objModelData_freealloc(objModelData_t* model);
void objModelData_destroy(objModelData_t** model);


typedef struct worldspawnEntry_s {
    char* obj_name;
    vector3_t position;
    vector3_t rotation;
}worldspawnEntry_t;

typedef struct propEntry_s {
    char* obj_name;
    vector3_t position;
    vector3_t rotation;
}propEntry_t;

typedef struct pointLightInfo_s {
    vector3_t position;
    vector3_t color;
    float intensity;
    float attenuation;
}pointLightInfo_t;

typedef struct directionalLightInfo_s {
    vector3_t position;
    vector3_t color;
}directionalLightInfo_t;

typedef struct scene_s {
    vector3_t player_start;
    vector3_t player_lookat;
    list_t* worldspawn; //worldspawnEntry_t
    list_t* props; //propEntry_t
    list_t* point_light_info; //pointLightInfo_t
    list_t* directional_light_info; //pointLightInfo_t
}scene_t;

scene_t* scene_read_scene(char* file_path);

#endif