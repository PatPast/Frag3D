#include <stdio.h>
#include <stdlib.h>
#include "assets.h"
#define BUFFER_SIZE 1024

list_t* load_mtl_file(const char* file_path) {
    list_t* materials = list_init(); //liste des material à retourner
    material_t* current_material; // material courant à stocker
    char buffer[BUFFER_SIZE]; //stocke la ligne courante
    char* end; //pointe vers la fin de la ligne

    FILE* mtl_file = fopen(file_path, "r");

    if (mtl_file == NULL) {
        fprintf(stderr, "Impossible d'ouvrir le fichier mtl '%s'\n", mtl_file);
        return materials;
    }
    
   
    while (fgets(buffer, BUFFER_SIZE, mtl_file)) {

        if (strstr(buffer, "newmtl") != NULL) {
            continue; // Start parsing from here
        }

        //place un marqueur de fin de chaine de caractère a la fin d'une ligne 
        char *end = buffer + strlen(buffer) - 1;
        while (end >= buffer && (*end == '\n' || *end == '\r')) {
            *end = '\0';
            end--;
        }

        //passe la ligne si elle est vide ou est un commentaire
        if (buffer[0] == '\0' || buffer[0] == '#') {
            continue;
        }

        if (strncmp(buffer, "newmtl ", 7) == 0) {
            //initialisation du material
            current_material = malloc(sizeof(material_t));
            current_material->name = strdup(buffer + 7); //copie le nom qui se trouve à coté
            current_material->diffuse_texture_name = NULL;
            current_material->diffuse[0] = 0.0f;
            current_material->diffuse[1] = 0.0f;
            current_material->diffuse[2] = 0.0f;
            current_material->transparency = 1.0f;
        }
        //lire diffuse_texture_name
        else if (strncmp(buffer, "map_Kd ", 7) == 0) {
            current_material->diffuse_texture_name = strdup(buffer + 7);
        }
        //lire diffuse
        else if (strncmp(buffer, "Kd ", 3) == 0) {
            sscanf(buffer + 3, "%f %f %f", &current_material->diffuse[0], &current_material->diffuse[1], &current_material->diffuse[2]);
        }
        //lire transparency
        else if (strncmp(buffer, "d ", 2) == 0) {
            sscanf(buffer + 2, "%f", &current_material->transparency);
        }

        list_add(materials, (void*)current_material);
    }


    fclose(mtl_file);

    return materials;
}


objModelData_t* objModelData_load(const char* file_path){

    objModelData_t* model = NULL; //model à retourner

    objSubmodelData_t* current_submodel = NULL; //objSubmodelData_t courant
    objSubmodelData_t* prev_face_data;
    list_t* faces = list_init(); //liste des faces à retourner

    float* current_position_data;
    float* current_uv_data;
    float* current_normal_data;

    char buffer[BUFFER_SIZE]; //stocke la ligne courante
    char* end; //pointe vers la fin de la ligne

    FILE* obj_file = fopen(file_path, "r");

    if (obj_file == NULL) {
        fprintf(stderr, "Impossible d'ouvrir le fichier obj '%s'\n", obj_file);
        return model;
    }

    //ignorer tout avant mtllib
    while (fscanf(obj_file, "%[^\n] ", buffer)) {
        if (strstr(buffer, "mtllib") != NULL) {
            break; 
        }
    }
    
    char* mtllib_name = strdup(buffer + 9); //commence après le "mtllib ./"
    char* mtllib_path = strdup(file_path);
    char* slash_index = strrchr(mtllib_path, '/'); //dernier slash du file_path
    *++slash_index = '\0'; //efface ce qui est après
    strcat(mtllib_path, mtllib_name); //pour choisir le fichier mtl

    model->materials = load_mtl_file(mtllib_path);
    free(mtllib_name);
    free(mtllib_path);
    mtllib_name = mtllib_path = slash_index = NULL;
    
    model->position_data = list_init();
    model->uv_data = list_init();
    model->normal_data = list_init();
    model->submodel_data = list_init();


    while (fgets(buffer, BUFFER_SIZE, obj_file)) {

        //place un marqueur de fin de chaine de caractère a la fin d'une ligne 
        char *end = buffer + strlen(buffer) - 1;
        while (end >= buffer && (*end == '\n' || *end == '\r')) {
            *end = '\0';
            end--;
        }

        if (buffer[0] == '\0' || buffer[0] == '#') {
            continue;
        }
        if (strncmp(buffer, "vt ", 3) == 0) {
            current_uv_data = malloc(sizeof(float) * 2);
            sscanf(buffer + 3, "%f %f", &current_uv_data[0], &current_uv_data[1]);
            list_add(model->uv_data, current_uv_data);
        }
        else if (strncmp(buffer, "vn ", 3) == 0) {
            current_normal_data = malloc(sizeof(float) * 3);
            sscanf(buffer + 3, "%f %f %f", &current_normal_data[0], &current_normal_data[1], &current_normal_data[2]);
            list_add(model->normal_data, current_normal_data);
        }
        else if (strncmp(buffer, "v ", 2) == 0) {
            current_position_data = malloc(sizeof(float) * 3);
            sscanf(buffer + 2, "%f %f %f", &current_position_data[0], &current_position_data[1], &current_position_data[2]);
            list_add(model->position_data, current_position_data);
        }
        else if (strncmp(buffer, "v ", 2) == 0) {
            current_position_data = malloc(sizeof(float) * 3);
            sscanf(buffer + 2, "%f %f %f", &current_position_data[0], &current_position_data[1], &current_position_data[2]);
            list_add(model->position_data, current_position_data);
        }
        else if (strncmp(buffer, "usemtl ", 7) == 0) {
            if (current_submodel->material_name != NULL) {
                prev_face_data = malloc(sizeof(objSubmodelData_t));
                prev_face_data->material_name = strdup(current_submodel->material_name);

                list_add(model->submodel_data, prev_face_data);
            }
            line_stream >> command >> current_submodel.material_name;
            current_submodel.faces.clear();
        }
        else if (line.find('f') == 0) {
            char dummy;
            objFaceData_t face_data;
            sscanf(line.c_str(), "%c %zu/%zu/%zu %zu/%zu/%zu %zu/%zu/%zu", &dummy,
                &face_data.position_indices[0], &face_data.uv_indices[0], &face_data.normal_indices[0],
                &face_data.position_indices[1], &face_data.uv_indices[1], &face_data.normal_indices[1],
                &face_data.position_indices[2], &face_data.uv_indices[2], &face_data.normal_indices[2]);

            for (size_t i = 0; i < 3; i++) {
                face_data.position_indices[i]--;
                face_data.uv_indices[i]--;
                face_data.normal_indices[i]--;
            }

            current_submodel.faces.push_back(face_data);
        }
    }

    this->submodel_data.push_back(current_submodel);
}


scene_t read_scene(const char* file_path);
