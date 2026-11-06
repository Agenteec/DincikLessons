// scenes.h — каждая сцена — набор из четырёх функций.
#ifndef SCENES_H
#define SCENES_H

typedef struct {
    const char* title;
    void (*init)(void);
    void (*update)(float dt);
    void (*draw)(void);
    void (*unload)(void);
} Scene;

extern const Scene scene_pointers;
extern const Scene scene_arrays;
extern const Scene scene_structs;
extern const Scene scene_dynarray;
extern const Scene scene_list;
extern const Scene scene_static;
extern const Scene scene_tree;

#endif
