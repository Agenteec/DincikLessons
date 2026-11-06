// phys.h — общее для сцен: создание мира, рисование тел, правильная уборка, панель «куча».
#ifndef PHYS_H
#define PHYS_H

#include "chipmunk/chipmunk.h"
#include "memview.h"
#include "viz.h"

// Поле с физикой у всех сцен одинаковое: слева, справа от него — код и память.
#define FIELD_X 30.0f
#define FIELD_Y 130.0f
#define FIELD_W 610.0f
#define FIELD_H 480.0f
#define PANEL_X 660.0f   // правая колонка сцены (до CODE_X)
#define PANEL_W 520.0f

static inline Rectangle field_rect(void) { return (Rectangle){ FIELD_X, FIELD_Y, FIELD_W, FIELD_H }; }

cpSpace* phys_new_space(void);
void     phys_step(cpSpace* space, float dt);

// Пол и стены по краю поля. Возвращает число добавленных форм (и кладёт их в out, если не NULL).
int      phys_add_walls(cpSpace* space, cpCollisionType floor_type, cpCollisionType wall_type, cpShape** out);

cpBody*  phys_add_box(cpSpace* space, cpVect pos, cpFloat w, cpFloat h, Color c);
cpBody*  phys_add_ball(cpSpace* space, cpVect pos, cpFloat r, Color c);

void     shape_set_color(cpShape* shape, Color c);
void     body_set_color(cpBody* body, Color c);
void     phys_draw(cpSpace* space);
cpBody*  phys_body_at(cpSpace* space, Vector2 p);   // подвижное тело под мышкой или NULL

// Правильная уборка: шарниры → формы → тело (userData не трогаем — его освобождает хозяин).
void     phys_remove_body(cpSpace* space, cpBody* body);
// Правильная уборка всего мира. free_user_data — освободить ли userData тел через mt_free.
void     phys_free_space(cpSpace* space, bool free_user_data);


#endif
