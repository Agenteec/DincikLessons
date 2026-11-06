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

extern const Scene scene_cells;
extern const Scene scene_neighbors;
extern const Scene scene_step;
extern const Scene scene_resize;
extern const Scene scene_gallery;

#endif
