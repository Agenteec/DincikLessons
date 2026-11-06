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

extern const Scene scene_bodies;
extern const Scene scene_userdata;
extern const Scene scene_callbacks;
extern const Scene scene_bridge;
extern const Scene scene_cleanup;

#endif
