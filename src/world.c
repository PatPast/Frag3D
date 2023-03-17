#include <world.h>

triangle_t triangle_init(vector3_t p0,vector3_t p1,vector3_t p2){
    triangle_t tr = {p0, p1, p2};  
    vector3_t c = vector3_cross(vector3_sub(p1, p0), vector3_sub(p2, p0));
    tr.normal = vector3_normalize(c);
    tr.area = vector3_length(c) * 0.5f;
    return tr;
}

ray_t ray_init(vector3_t o, vector3_t d) {
    ray_t r = {o, d};
    return r;
}

vector3_t ray_at(ray_t r, float t){
    return vector3_add(r.origin, vector3_mult(r.direction, t));
}

playerShape_t playerShape_init(vector3_t player_pos, float height, float r){
    playerShape_t ps = {.radius = r};
    ps.mid_point = player_pos;
    ps.segment_up = vector3_add(player_pos, vector3_mult(VECTOR3_UP, height * 0.5f));
    ps.segment_bottom = vector3_add(player_pos, vector3_mult(VECTOR3_DOWN, height * 0.5f));
    ps.tip_up = vector3_add(ps.segment_up, vector3_mult(VECTOR3_UP, ps.radius));
    ps.tip_bottom = vector3_add(ps.segment_bottom, vector3_mult(VECTOR3_DOWN, ps.radius));;
    return ps;
}

void playerShape_displace(playerShape_t* ps, vector3_t displacement) {
    ps->mid_point = vector3_add(ps->mid_point, displacement);
    ps->segment_up = vector3_add(ps->segment_up, displacement);
    ps->segment_bottom = vector3_add(ps->segment_bottom, displacement);
    ps->tip_up = vector3_add(ps->tip_up, displacement);
    ps->tip_bottom = vector3_add(ps->tip_bottom, displacement);
}

staticCollider_t* staticCollider_init(objModelData_t* obj_data, vector3_t position, vector3_t rotation){
    staticCollider_t* sc = malloc(sizeof(staticCollider_t));
    sc->triangles = list_init(sizeof(triangle_t));
    
    list_t* position_data = obj_data->position_data;

    list_foreach(objsubdata, obj_data->submodel_data) {
        objSubmodelData_t* obj_submodel_data = (objSubmodelData_t*)objsubdata;
        list_foreach(fdata, obj_submodel_data->faces) {
            objFaceData_t* face_data = (objFaceData_t*)fdata;
            size_t* position_indices = face_data->position_indices;
            vector3_t p0 = vector3_add(vector3_rotate(*(vector3_t*)list_elem(position_data, position_indices[0]), rotation), position);
            vector3_t p1 = vector3_add(vector3_rotate(*(vector3_t*)list_elem(position_data, position_indices[1]), rotation), position);
            vector3_t p2 = vector3_add(vector3_rotate(*(vector3_t*)list_elem(position_data, position_indices[2]), rotation), position);
            
            triangle_t t = triangle_init(p0, p1, p2);
            list_add(sc->triangles, &t);
        }
    }
    return sc;
}

void staticCollider_freealloc(staticCollider_t* sc){
    list_destroy(&sc->triangles);
}
void staticCollider_destroy(staticCollider_t** sc){
    staticCollider_freealloc(*sc);
    free(*sc);
    *sc = NULL;
}

world_t* world_init(){
    world_t* w = malloc((sizeof(world_t)));
    w->static_colliders =  list_init(sizeof(staticCollider_t));
    w->player_position = VECTOR3_ZERO;
    w->player_forward = VECTOR3_FORWARD;
    w->player_velocity = VECTOR3_ZERO;
    w->is_prev_grounded = 0;
    w->fly_move_enabled = 0;
    return w;
}

void world_freealloc(world_t* w){
    list_foreach(sc, w->static_colliders){
        staticCollider_freealloc(sc);
    }
    list_destroy(&w->static_colliders);
}

void world_destroy(world_t** w){
    world_freealloc(*w);
    free(*w);
    *w = NULL;
}

void world_register_scene(world_t* w, scene_t* scene){
    w->player_position = scene->player_start;
    w->player_forward = vector3_normalize(vector3_sub(scene->player_lookat, scene->player_start));

    list_foreach(e, scene->worldspawn) {
        worldspawnEntry_t* entry = (worldspawnEntry_t*)e;
        objModelData_t* obj_data = objModelData_load(entry->obj_name);
        world_register_static_collider(w, obj_data, entry->position, entry->rotation);
    }
}

void world_register_static_collider(world_t* w, objModelData_t* obj_data, vector3_t position, vector3_t rotation){
    staticCollider_t* sc = staticCollider_init(obj_data, position, rotation);
    list_add(w->static_colliders, sc);
    free(sc); 
    //TODO voir s'il faut free
}

