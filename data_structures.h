//
// Created by arcticlemming on 10/21/18.
//

#ifndef PROJ_3_DATA_STRUCTURES_H
#define PROJ_3_DATA_STRUCTURES_H
/////////////////////
/// Data structures//
/////////////////////
typedef struct{
    off_t address;
    off_t next;
} list;
typedef struct{
    int max_size;
    off_t dirs[32];
    int top;
} stack;
#endif //PROJ_3_DATA_STRUCTURES_H
