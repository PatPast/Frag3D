#include <assets.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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
        buffer[(int)(strchr(buffer, '\n') - buffer)] = '\0';

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
            current_material.diffuse.x = 0.0f;
            current_material.diffuse.y = 0.0f;
            current_material.diffuse.z = 0.0f;
            current_material.transparency = 1.0f;
        }
        //lire diffuse_texture_name
        else if (strncmp(buffer, "map_Kd ", 7) == 0) {
            current_material.diffuse_texture_name = strdup(buffer + 7);
        }
        //lire diffuse
        else if (strncmp(buffer, "Kd ", 3) == 0) {
            sscanf(buffer + 3, "%f %f %f", &current_material.diffuse.x, &current_material.diffuse.y, &current_material.diffuse.z);
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

    vector3_t current_position_data;
    vector2_t current_uv_data;
    vector3_t current_normal_data;

    char buffer[BUFFER_SIZE]; //stocke la ligne courante

    

    FILE* obj_file = fopen(file_path, "r");

    if (obj_file == NULL) {
        fprintf(stderr, "Impossible d'ouvrir le fichier obj '%s'\n", obj_file);
        free(model);
        return model;
    }

    //ignorer tout avant mtllib
    while (fscanf(obj_file, "%[^\n]", buffer)) {
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
    
    model->position_data = list_init(sizeof(vector3_t));
    model->uv_data = list_init(sizeof(vector2_t));
    model->normal_data = list_init(sizeof(vector3_t));
    model->submodel_data = list_init(sizeof(objSubmodelData_t));



    while (fgets(buffer, BUFFER_SIZE, obj_file)) {

        //place un marqueur de fin de chaine de caractère a la fin d'une ligne 
        buffer[(int)(strchr(buffer, '\n') - buffer)] = '\0';

        //ignorer les lignes vides et les commentaires
        if (buffer[0] == '\0' || buffer[0] == '#') {
            continue;
        }
        if (strncmp(buffer, "vt ", 3) == 0) {
            sscanf(buffer + 3, "%f %f", &current_uv_data.x, &current_uv_data.y);
            list_add(model->uv_data, (void*)&current_uv_data);
        }
        else if (strncmp(buffer, "vn ", 3) == 0) {
            sscanf(buffer + 3, "%f %f %f", &current_normal_data.x, &current_normal_data.y, &current_normal_data.z);
            list_add(model->normal_data, (void*)&current_normal_data);
        }
        else if (strncmp(buffer, "v ", 2) == 0) {
            sscanf(buffer + 2, "%f %f %f", &current_position_data.x, &current_position_data.y, &current_position_data.z);
            list_add(model->position_data, (void*)&current_position_data);
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


scene_t* scene_read_scene(char* file_path){

    scene_t* scene = malloc(sizeof(scene_t));
    
    worldspawnEntry_t current_worldspawn;
    propEntry_t current_prop;
    pointLightInfo_t current_pointLightInfo;
    directionalLightInfo_t current_directionalLightInfo;

    char buffer[BUFFER_SIZE]; //stocke la ligne courante

    FILE* scene_file = fopen(file_path, "r");
    
    if (scene_file == NULL) {
        fprintf(stderr, "Impossible d'ouvrir le fichier de scene '%s'\n", file_path);
        free(scene);
        return scene;
    }

    scene->worldspawn = list_init(sizeof(worldspawnEntry_t));
    scene->props = list_init(sizeof(propEntry_t));
    scene->point_light_info = list_init(sizeof(pointLightInfo_t));

    while (fgets(buffer, BUFFER_SIZE, scene_file)) {

        //place un marqueur de fin de chaine de caractère a la fin d'une ligne 
        buffer[(int)(strchr(buffer, '\n') - buffer)] = '\0'; 
        
        

        //ignorer les lignes vides et les commentaires
        if (buffer[0] == '\0' || buffer[0] == '#') {
            continue;
        }

        if (strncmp(buffer, "@player_start", 13) == 0) {
            fgets(buffer, BUFFER_SIZE, scene_file); 
            buffer[(int)(strchr(buffer, '\n') - buffer)] = '\0';
            sscanf(buffer, "%f %f %f", &scene->player_start.x, &scene->player_start.y, &scene->player_start.z);
        }
        else if (strncmp(buffer, "@player_lookat", 14) == 0) {
            fgets(buffer, BUFFER_SIZE, scene_file);
            buffer[(int)(strchr(buffer, '\n') - buffer)] = '\0';
            sscanf(buffer, "%f %f %f", &scene->player_lookat.x, &scene->player_lookat.y, &scene->player_lookat.z);
        }
        else if (strncmp(buffer, "@worldspawn", 11) == 0) {
            fgets(buffer, BUFFER_SIZE, scene_file);
            buffer[(int)(strchr(buffer, '\n') - buffer)] = '\0';
            current_worldspawn.obj_name = malloc(sizeof(char) *(strlen(buffer) + 1) );
            sscanf(buffer, "%s", current_worldspawn.obj_name);

            fgets(buffer, BUFFER_SIZE, scene_file);
            buffer[(int)(strchr(buffer, '\n') - buffer)] = '\0';
            sscanf(buffer, "%f %f %f", &current_worldspawn.position.x, &current_worldspawn.position.y, &current_worldspawn.position.z);
            
            fgets(buffer, BUFFER_SIZE, scene_file);
            buffer[(int)(strchr(buffer, '\n') - buffer)] = '\0';
            sscanf(buffer, "%f %f %f", &current_worldspawn.rotation.x, &current_worldspawn.rotation.y, &current_worldspawn.rotation.z);

            list_add(scene->worldspawn, (void*)&current_worldspawn);
        }
        else if (strncmp(buffer, "@prop", 5) == 0) {
            fgets(buffer, BUFFER_SIZE, scene_file);
            buffer[(int)(strchr(buffer, '\n') - buffer)] = '\0';
            current_prop.obj_name = malloc(sizeof(char) *(strlen(buffer) + 1) );
            sscanf(buffer, "%s", current_prop.obj_name);

            fgets(buffer, BUFFER_SIZE, scene_file);
            buffer[(int)(strchr(buffer, '\n') - buffer)] = '\0';
            sscanf(buffer, "%f %f %f", &current_prop.position.x, &current_prop.position.y, &current_prop.position.z);
            
            fgets(buffer, BUFFER_SIZE, scene_file);
            buffer[(int)(strchr(buffer, '\n') - buffer)] = '\0';
            sscanf(buffer, "%f %f %f", &current_prop.rotation.x, &current_prop.rotation.y, &current_prop.rotation.z);

            list_add(scene->worldspawn, (void*)&current_prop);
        }

        else if (strncmp(buffer, "@point_light", 12) == 0) {
            fgets(buffer, BUFFER_SIZE, scene_file);
            buffer[(int)(strchr(buffer, '\n') - buffer)] = '\0';
            sscanf(buffer, "%f %f %f", &current_pointLightInfo.position.x, &current_pointLightInfo.position.y, &current_pointLightInfo.position.z);
            
            fgets(buffer, BUFFER_SIZE, scene_file);
            buffer[(int)(strchr(buffer, '\n') - buffer)] = '\0';
            sscanf(buffer, "%f %f %f", &current_pointLightInfo.color.x, &current_pointLightInfo.color.y, &current_pointLightInfo.color.z);
            
            fgets(buffer, BUFFER_SIZE, scene_file);
            buffer[(int)(strchr(buffer, '\n') - buffer)] = '\0';
            sscanf(buffer, "%f", &current_pointLightInfo.intensity);
            
            fgets(buffer, BUFFER_SIZE, scene_file);
            buffer[(int)(strchr(buffer, '\n') - buffer)] = '\0';
            sscanf(buffer, "%f", &current_pointLightInfo.attenuation);

            list_add(scene->worldspawn, (void*)&current_pointLightInfo);
        }

        else if (strncmp(buffer, "@directional_light", 17) == 0) {
            fgets(buffer, BUFFER_SIZE, scene_file);
            buffer[(int)(strchr(buffer, '\n') - buffer)] = '\0';
            sscanf(buffer, "%f %f %f", &current_directionalLightInfo.position.x, &current_directionalLightInfo.position.y, &current_directionalLightInfo.position.z);
            
            fgets(buffer, BUFFER_SIZE, scene_file);
            buffer[(int)(strchr(buffer, '\n') - buffer)] = '\0';
            sscanf(buffer, "%f %f %f", &current_directionalLightInfo.color.x, &current_directionalLightInfo.color.y, &current_directionalLightInfo.color.z);

            scene->directional_light_info = current_directionalLightInfo;
        }
        
    }

    fclose(scene_file);

    return scene;
}

void scene_destroy(scene_t** scene){
    list_foreach(worldspawn,(*scene)->worldspawn){
        worldspawnEntry_freealloc((worldspawnEntry_t*)worldspawn);
    }
    list_foreach(props,(*scene)->props){
        propEntry_freealloc((propEntry_t*)props);
    }
    list_destroy(&(*scene)->worldspawn);
    list_destroy(&(*scene)->props);
    list_destroy(&(*scene)->point_light_info);
    free(*scene);
    *scene = NULL;

}

void worldspawnEntry_freealloc(worldspawnEntry_t* entry){
    free(entry->obj_name);
}

void propEntry_freealloc(propEntry_t* entry){
    free(entry->obj_name);
}