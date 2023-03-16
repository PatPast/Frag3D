#include <stdio.h>
#include <common.h>
#include <vector3.h>

int main(){
    vector3_t v = vector3_init(0, 1, 2);
    printf("test\n");
    printf("%f %f %f\n",v.x, v.y, v.z);
    return 0;
}