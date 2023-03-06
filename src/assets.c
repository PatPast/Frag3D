#include <stdio.h>
#include <stdlib.h>
#include <assets.h>
#define BUFFER_SIZE 1024

void material_freealloc(material_t* material){
    free(material->name);
    free(material->diffuse_texture_name);
}

void objSubmodelData_freealloc(objSubmodelData_t* submodel){
    free(submodel->material_name);
    list_destroy(&submodel->faces);
}

void objModelData_freealloc(objModelData_t* model){
    //foreach material in model->materials
    list_foreach(material,model->materials){
        material_freealloc((material_t*)material);
    }
    list_destroy(&model->materials);
    list_destroy(&model->position_data);
    list_destroy(&model->uv_data);
    list_destroy(&model->normal_data);
    //foreach submodel in model->submodel_data
    list_foreach(submodel,model->submodel_data){
        objSubmodelData_freealloc((objSubmodelData_t*)submodel);
    }
    list_destroy(&model->submodel_data);
}

list_t* load_mtl_file(const char* file_path) {
    list_t* materials = list_init(sizeof(material_t)); //liste des material à retourner
    material_t current_material; // material courant à stocker

    char buffer[BUFFER_SIZE]; //stocke la ligne courante
    char* end; //pointe vers la fin de la ligne

    FILE* mtl_file = fopen(file_path, "r");

    if (mtl_file == NULL) {
        fprintf(stderr, "Impossible d'ouvrir le fichier mtl '%s'\n", mtl_file);
        list_destroy(&materials);
        return materials;
    }
    
   
    while (fgets(buffer, BUFFER_SIZE, mtl_file)) {

        //place un marqueur de fin de chaine de caractère a la fin d'une ligne 
        *end = buffer + strlen(buffer) - 1;
        while (end >= buffer && (*end == '\n' || *end == '\r')) {
            *end = '\0';
            end--;
        }

        //passe la ligne si elle est vide ou est un commentaire
        if (buffer[0] == '\0' || buffer[0] == '#') {
            continue;
        }

        if (strncmp(buffer, "newmtl ", 7) == 0) {
            if(current_material.name != NULL){
                list_add(materials, (void*)&current_material);
            }

            //initialisation du material
            current_material.name = strdup(buffer + 7); //copie le nom qui se trouve à coté
            current_material.diffuse_texture_name = NULL;
            current_material.diffuse[0] = 0.0f;
            current_material.diffuse[1] = 0.0f;
            current_material.diffuse[2] = 0.0f;
            current_material.transparency = 1.0f;
        }
        //lire diffuse_texture_name
        else if (strncmp(buffer, "map_Kd ", 7) == 0) {
            current_material.diffuse_texture_name = strdup(buffer + 7);
        }
        //lire diffuse
        else if (strncmp(buffer, "Kd ", 3) == 0) {
            sscanf(buffer + 3, "%f %f %f", &current_material.diffuse[0], &current_material.diffuse[1], &current_material.diffuse[2]);
        }
        //lire transparency
        else if (strncmp(buffer, "d ", 2) == 0) {
            sscanf(buffer + 2, "%f", &current_material.transparency);
        }
        
    }

    list_add(materials, (void*)&current_material);

    fclose(mtl_file);

    return materials;
}


objModelData_t* objModelData_load(const char* file_path){

    objModelData_t* model = malloc(sizeof(objModelData_t)); //model à retourner

    objSubmodelData_t current_submodel = {NULL, list_init(sizeof(objFaceData_t))}; //objSubmodelData_t courant
    objSubmodelData_t prev_face_data;
    objFaceData_t current_face_data;

    float current_position_data[3];
    float current_uv_data[2];
    float current_normal_data[3];

    char buffer[BUFFER_SIZE]; //stocke la ligne courante
    char* end; //pointe vers la fin de la ligne

    

    FILE* obj_file = fopen(file_path, "r");

    if (obj_file == NULL) {
        fprintf(stderr, "Impossible d'ouvrir le fichier obj '%s'\n", obj_file);
        free(model);
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
    
    model->position_data = list_init(sizeof(float)*3);
    model->uv_data = list_init(sizeof(float)*2);
    model->normal_data = list_init(sizeof(float)*3);
    model->submodel_data = list_init(sizeof(objSubmodelData_t));



    while (fgets(buffer, BUFFER_SIZE, obj_file)) {

        //place un marqueur de fin de chaine de caractère a la fin d'une ligne 
        end = buffer + strlen(buffer) - 1;
        while (end >= buffer && (*end == '\n' || *end == '\r')) {
            *end = '\0';
            end--;
        }

        if (buffer[0] == '\0' || buffer[0] == '#') {
            continue;
        }
        if (strncmp(buffer, "vt ", 3) == 0) {
            sscanf(buffer + 3, "%f %f", &current_uv_data[0], &current_uv_data[1]);
            list_add(model->uv_data, (void*)current_uv_data);
        }
        else if (strncmp(buffer, "vn ", 3) == 0) {
            sscanf(buffer + 3, "%f %f %f", &current_normal_data[0], &current_normal_data[1], &current_normal_data[2]);
            list_add(model->normal_data, (void*)current_normal_data);
        }
        else if (strncmp(buffer, "v ", 2) == 0) {
            sscanf(buffer + 2, "%f %f %f", &current_position_data[0], &current_position_data[1], &current_position_data[2]);
            list_add(model->position_data, (void*)current_position_data);
        }
        else if (strncmp(buffer, "usemtl ", 7) == 0) {
            if (current_submodel.material_name != NULL) {
                prev_face_data.material_name = strdup(current_submodel.material_name);
                prev_face_data.faces = list_duplicate(current_submodel.faces);

                list_add(model->submodel_data, (void*)&prev_face_data);
            }
            current_submodel.material_name = realloc(current_submodel.material_name, sizeof(char) * strlen(buffer + 7)+1);
            strcpy(current_submodel.material_name, buffer + 7);
            list_clear(current_submodel.faces);
        }
        else if (strncmp(buffer, "f ", 2) == 0) {
            sscanf(buffer + 2, "%zu/%zu/%zu %zu/%zu/%zu %zu/%zu/%zu",
                &current_face_data.position_indices[0], &current_face_data.uv_indices[0], &current_face_data.normal_indices[0],
                &current_face_data.position_indices[1], &current_face_data.uv_indices[1], &current_face_data.normal_indices[1],
                &current_face_data.position_indices[2], &current_face_data.uv_indices[2], &current_face_data.normal_indices[2]);

            for (size_t i = 0; i < 3; i++) {
                current_face_data.position_indices[i]--;
                current_face_data.uv_indices[i]--;
                current_face_data.normal_indices[i]--;
            }

            list_add(current_submodel.faces,(void*)&current_face_data);
        }
    }

    list_add(model->submodel_data, (void*)&current_submodel);

    fclose(obj_file);

    return model;
}

void objModelData_destroy(objModelData_t** model){
    objModelData_freealloc(*model);
    free(*model);
    *model = NULL;
}


scene_t read_scene(const char* file_path){

    scene_t* scene = malloc(sizeof(scene_t));

    char buffer[BUFFER_SIZE]; //stocke la ligne courante
    char* end; //pointe vers la fin de la ligne

    FILE* scene_file = fopen(file_path, "r");
    
    if (obj_file == NULL) {
        fprintf(stderr, "Impossible d'ouvrir le fichier de scene '%s'\n", obj_file);
        free(scene);
        return model;
    }

    while (fgets(buffer, BUFFER_SIZE, scene_file)) {

        //place un marqueur de fin de chaine de caractère a la fin d'une ligne 
        end = buffer + strlen(buffer) - 1;
        while (end >= buffer && (*end == '\n' || *end == '\r')) {
            *end = '\0';
            end--;
        }

        if (buffer[0] == '\0' || buffer[0] == '#') {
            continue;
        }
        //TODO terminer la fonction
        if (strncmp(buffer, "@player_start", 13) == 0) {
            fgets(buffer, BUFFER_SIZE, scene_file);
            sscanf(buffer, "%f %f", &current_uv_data[0], &current_uv_data[1]);
            list_add(model->uv_data, (void*)current_uv_data);
        }
        else if (strncmp(buffer, "@player_lookat", 14) == 0) {
            sscanf(buffer + 3, "%f %f %f", &current_normal_data[0], &current_normal_data[1], &current_normal_data[2]);
            list_add(model->normal_data, (void*)current_normal_data);
        }
        else if (strncmp(buffer, "@worldspawn", 11) == 0) {
            sscanf(buffer + 2, "%f %f %f", &current_position_data[0], &current_position_data[1], &current_position_data[2]);
            list_add(model->position_data, (void*)current_position_data);
        }
        else if (strncmp(buffer, "usemtl ", 7) == 0) {
            if (current_submodel.material_name != NULL) {
                prev_face_data.material_name = strdup(current_submodel.material_name);
                prev_face_data.faces = list_duplicate(current_submodel.faces);

                list_add(model->submodel_data, (void*)&prev_face_data);
            }
            current_submodel.material_name = realloc(current_submodel.material_name, sizeof(char) * strlen(buffer + 7)+1);
            strcpy(current_submodel.material_name, buffer + 7);
            list_clear(current_submodel.faces);
        }
        else if (strncmp(buffer, "f ", 2) == 0) {
            sscanf(buffer + 2, "%zu/%zu/%zu %zu/%zu/%zu %zu/%zu/%zu",
                &current_face_data.position_indices[0], &current_face_data.uv_indices[0], &current_face_data.normal_indices[0],
                &current_face_data.position_indices[1], &current_face_data.uv_indices[1], &current_face_data.normal_indices[1],
                &current_face_data.position_indices[2], &current_face_data.uv_indices[2], &current_face_data.normal_indices[2]);

            for (size_t i = 0; i < 3; i++) {
                current_face_data.position_indices[i]--;
                current_face_data.uv_indices[i]--;
                current_face_data.normal_indices[i]--;
            }

            list_add(current_submodel.faces,(void*)&current_face_data);
        }
    }

    list_add(model->submodel_data, (void*)&current_submodel);

    fclose(obj_file);

    while (std::getline(stream, line)) {
        if (line.find('#') == 0) {
            continue;
        }
        if (line.find("@player_start") == 0) {
            std::getline(stream, line);
            scene.player_start = read_vec3(line, line_stream);
        }
        else if (line.find("@player_lookat") == 0) {
            std::getline(stream, line);
            scene.player_lookat = read_vec3(line, line_stream);
        }
        else if (line.find("@worldspawn") == 0) {
            std::getline(stream, line);
            std::string obj_name = read_string(line, line_stream);

            std::getline(stream, line);
            Vector3 pos = read_vec3(line, line_stream);

            std::getline(stream, line);
            Vector3 rot = read_vec3(line, line_stream);

            scene.worldspawn.push_back({ obj_name, pos, rot });
        }
        else if (line.find("@prop") == 0) {
            std::getline(stream, line);
            std::string obj_name = read_string(line, line_stream);

            std::getline(stream, line);
            Vector3 pos = read_vec3(line, line_stream);

            std::getline(stream, line);
            Vector3 rot = read_vec3(line, line_stream);

            scene.props.push_back({ obj_name, pos, rot });
        }
        else if (line.find("@point_light") == 0) {
            std::getline(stream, line);
            Vector3 pos = read_vec3(line, line_stream);

            std::getline(stream, line);
            Vector3 color = read_vec3(line, line_stream);

            std::getline(stream, line);
            float attenuation = read_float(line, line_stream);

            std::getline(stream, line);
            float intensity = read_float(line, line_stream);

            scene.point_light_info.push_back({ pos, color, attenuation, intensity });
        }
        else if (line.find("@directional_light") == 0) {
            std::getline(stream, line);
            Vector3 pos = read_vec3(line, line_stream);

            std::getline(stream, line);
            Vector3 color = read_vec3(line, line_stream);

            scene.directional_light_info = { pos, color };
        }
    }

    return scene;
}