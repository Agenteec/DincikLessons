// Сцена 4. Мост: доски на шарнирах — почти связный список. Важно, в каком порядке разбирать.
#include "phys.h"
#include "scenes.h"
#include "tickets.h"

#include <math.h>
#include <stdio.h>

#define BRIDGE_Y   (FIELD_Y + 250)
#define BRIDGE_L   (FIELD_X + 70)
#define BRIDGE_R   (FIELD_X + FIELD_W - 70)
#define PLANK_STEP ((BRIDGE_R - BRIDGE_L) / PLANKS)

static cpSpace* space;
static Bridge   bridge;
static const char* message;
static Color    message_color;

// пошаговое удаление (правильный порядок)
static Demo  demo;
static int   demo_i;
static cpConstraint* demo_joints[2];
static Vector2 ghost[4];
static float   ghost_alpha;

static const char* remove_code[] = {
    "cpSpaceRemoveConstraint(space, joint[i]);",
    "cpConstraintFree(joint[i]);   // и joint[i+1]",
    "cpSpaceRemoveShape(space, shape[i]);",
    "cpShapeFree(shape[i]);",
    "cpSpaceRemoveBody(space, plank[i]);",
    "cpBodyFree(plank[i]);",
};

static void build(void)
{
    space = phys_new_space();
    phys_add_walls(space, 0, 0, NULL);
    cpBody* ground = cpSpaceGetStaticBody(space);

    // два берега
    cpBB banks[2] = { cpBBNew(FIELD_X, BRIDGE_Y, BRIDGE_L, FIELD_Y + FIELD_H),
                      cpBBNew(BRIDGE_R, BRIDGE_Y, FIELD_X + FIELD_W, FIELD_Y + FIELD_H) };
    for (int i = 0; i < 2; i++) {
        cpShape* s = cpSpaceAddShape(space, cpBoxShapeNew2(ground, banks[i], 0));
        cpShapeSetFriction(s, 0.9);
        shape_set_color(s, (Color){ 120, 100, 80, 255 });
    }

    bridge = (Bridge){ .space = space };
    for (int i = 0; i < PLANKS; i++) {
        cpVect c = cpv(BRIDGE_L + PLANK_STEP * (i + 0.5f), BRIDGE_Y);
        bridge.plank[i] = cpSpaceAddBody(space, cpBodyNew(1, cpMomentForBox(1, PLANK_STEP - 6, 14)));
        cpBodySetPosition(bridge.plank[i], c);
        bridge.shape[i] = cpSpaceAddShape(space, cpBoxShapeNew(bridge.plank[i], PLANK_STEP - 6, 14, 1));
        cpShapeSetFriction(bridge.shape[i], 0.9);
        shape_set_color(bridge.shape[i], (Color){ 188, 140, 90, 255 });
    }
    // шарнир j держит доску j-1 и доску j; крайние держатся за берег
    for (int j = 0; j <= PLANKS; j++) {
        cpBody* a = j == 0 ? ground : bridge.plank[j - 1];
        cpBody* b = j == PLANKS ? ground : bridge.plank[j];
        bridge.joint[j] = cpSpaceAddConstraint(space, cpPivotJointNew(a, b, cpv(BRIDGE_L + PLANK_STEP * j, BRIDGE_Y)));
    }
}

static void init(void)
{
    build();
    demo_stop(&demo);
    ghost_alpha = 0;
    message = "Брось на мост шарики (ЛКМ). Потом попробуй убрать доску: D — по шагам, ПКМ — функцией из tickets.c.";
    message_color = COL_DIM;
}

static void demo_step(void);

static void unload(void)
{
    if (demo.active) {   // ушли со сцены посреди демо — доводим удаление до конца
        while (demo.active) demo_step();
    }
    phys_free_space(space, false);
}

static int plank_at(Vector2 m)
{
    cpBody* b = phys_body_at(space, m);
    for (int i = 0; i < PLANKS; i++)
        if (b && bridge.plank[i] == b) return i;
    return -1;
}

static void demo_step(void)
{
    long long seq = mt_seq();
    int i = demo_i;
    switch (demo.line) {
    case 0:
        for (int k = 0; k < 2; k++) {
            demo_joints[k] = bridge.joint[i + k];
            if (demo_joints[k]) cpSpaceRemoveConstraint(space, demo_joints[k]);
        }
        viz_log(remove_code[0], "шарниры больше не в мире — доску ничто не держит");
        break;
    case 1:
        for (int k = 0; k < 2; k++) {
            if (demo_joints[k]) cpConstraintFree(demo_joints[k]);
            bridge.joint[i + k] = NULL;
        }
        viz_log(remove_code[1], "оба шарнира освобождены");
        mem_log_since(seq);
        break;
    case 2: {
        cpBody* body = bridge.plank[i];
        for (int k = 0; k < 4; k++) {
            cpVect v = cpBodyLocalToWorld(body, cpPolyShapeGetVert(bridge.shape[i], k));
            ghost[k] = (Vector2){ (float)v.x, (float)v.y };
        }
        ghost_alpha = 1;
        cpSpaceRemoveShape(space, bridge.shape[i]);
        viz_log(remove_code[2], "форма убрана из мира — доску больше не видно");
        break;
    }
    case 3:
        cpShapeFree(bridge.shape[i]);
        bridge.shape[i] = NULL;
        viz_log(remove_code[3], NULL);
        mem_log_since(seq);
        break;
    case 4:
        cpSpaceRemoveBody(space, bridge.plank[i]);
        viz_log(remove_code[4], NULL);
        break;
    case 5:
        cpBodyFree(bridge.plank[i]);
        bridge.plank[i] = NULL;
        viz_log(remove_code[5], "тело освобождено последним — на него уже никто не ссылается");
        mem_log_since(seq);
        break;
    default:
        demo_stop(&demo);
        message = "Правильный порядок: те, кто хранит адрес доски (шарниры, форма), уходят раньше самой доски.";
        message_color = COL_GOOD;
        return;
    }
    demo.line++;
}

static void update(float dt)
{
    ghost_alpha = viz_approach(ghost_alpha, 0, 1.2f, dt);
    phys_step(space, dt);

    if (demo.active) {
        if (demo_next(&demo, dt, false)) demo_step();
        if (!demo.active && demo.line > 0 && demo.line < 6) {
            // X посреди демо — доводим удаление до конца, чтобы ничего не повисло
            while (demo.line < 6) demo_step();
            demo_stop(&demo);
            message = "Демо прервано — доска убрана до конца.";
            message_color = COL_DIM;
        }
        return;
    }

    Vector2 m = GetMousePosition();
    int hovered = plank_at(m);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hovered < 0 && CheckCollisionPointRec(m, field_rect())) {
        phys_add_ball(space, cpv(m.x, m.y), GetRandomValue(10, 20), (Color){ 66, 165, 245, 255 });
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && hovered >= 0) {
        long long seq = mt_seq();
        viz_log(TextFormat("remove_plank(&bridge, %d);", hovered), "функция из tickets.c");
        remove_plank(&bridge, hovered);   // задание 4: если тут программа упала — порядок неверный
        mem_log_since(seq);
        message = "Доска убрана функцией из tickets.c.";
        message_color = COL_GOOD;
    }
    if (IsKeyPressed(KEY_D) && hovered >= 0) {
        demo_i = hovered;
        demo_start(&demo);
        message = "";
    }
    if (IsKeyPressed(KEY_R)) { unload(); init(); viz_log("// новый мост", NULL); }
}

static Vector2 joint_pos(cpConstraint* c)
{
    cpVect p = cpBodyLocalToWorld(cpConstraintGetBodyA(c), cpPivotJointGetAnchorA(c));
    return (Vector2){ (float)p.x, (float)p.y };
}

static void draw_chain(float x, float y)
{
    viz_text("Мост как цепочка: доски держатся друг за друга шарнирами", x, y, 17, COL_DIM);
    float tile = 40, gap = (PANEL_W - tile * (PLANKS + 2)) / (PLANKS + 1);
    float ty = y + 32;
    for (int k = 0; k < PLANKS + 2; k++) {
        Rectangle r = { x + k * (tile + gap), ty, tile, 34 };
        bool bank = k == 0 || k == PLANKS + 1;
        int i = k - 1;
        bool alive = bank || bridge.plank[i];
        bool target = demo.active && i == demo_i;
        Color c = bank ? (Color){ 120, 100, 80, 255 } : (alive ? (Color){ 188, 140, 90, 255 } : COL_CELL);
        if (alive) viz_fill(r, c);
        else viz_frame(r, 1.5f, COL_DIM);
        if (target) viz_frame(r, 2, COL_HILITE);
        viz_code(bank ? "#" : TextFormat("%d", i), r.x + 14, r.y + 7, 18, alive ? COL_BG : COL_DIM);
    }
    for (int j = 0; j <= PLANKS; j++) {
        float jx = x + (j + 1) * (tile + gap) - gap / 2;
        Vector2 p = { jx, ty + 17 };
        if (bridge.joint[j]) {
            bool removing = demo.active && (j == demo_i || j == demo_i + 1) && demo.line >= 1;
            DrawCircleV(p, 6, removing ? COL_BAD : COL_PTR);
        } else {
            DrawCircleLinesV(p, 6, COL_DIM);
        }
    }
    viz_text("кружок — шарнир: он хранит адреса двух досок", x, ty + 46, 16, COL_DIM);
}

static void draw(void)
{
    viz_title("Мост", "Каждый шарнир хранит адреса двух досок. Если освободить доску раньше шарнира — он укажет в пустоту.");

    phys_draw(space);
    for (int j = 0; j <= PLANKS; j++) {
        if (!bridge.joint[j] || !cpConstraintGetSpace(bridge.joint[j])) continue;
        bool removing = demo.active && (j == demo_i || j == demo_i + 1);
        DrawCircleV(joint_pos(bridge.joint[j]), 5, removing ? COL_BAD : COL_PTR);
    }
    if (ghost_alpha > 0.02f) {
        Color c = viz_tint(COL_BAD, (unsigned char)(255 * ghost_alpha));
        for (int k = 0; k < 4; k++) DrawLineEx(ghost[k], ghost[(k + 1) % 4], 2, c);
    }
    int hovered = demo.active ? -1 : plank_at(GetMousePosition());
    if (hovered >= 0) {
        cpVect p = cpBodyGetPosition(bridge.plank[hovered]);
        viz_code(TextFormat("plank[%d] = %s", hovered, viz_addr(bridge.plank[hovered])), (float)p.x - 70, (float)p.y - 40, 16, COL_HILITE);
    }

    viz_code_panel(PANEL_X, FIELD_Y, PANEL_W, remove_code, 6, demo.active ? demo.line - 1 : -1);
    draw_chain(PANEL_X, FIELD_Y + 200);
    mem_panel(PANEL_X, FIELD_Y + 400, PANEL_W);

    viz_message(message, message_color);
    if (demo.active) demo_hint(&demo);
    else viz_hint("ЛКМ — шарик     D на доске — удалить по шагам     ПКМ по доске — remove_plank() из tickets.c     R — новый мост");
}

const Scene scene_bridge = { "Мост", init, update, draw, unload };
