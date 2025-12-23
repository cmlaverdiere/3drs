#ifndef RENDERING_H
#define RENDERING_H

#include "raylib.h"

// Draw a simple sword shape
void DrawSword(Vector3 pos, Color bladeColor, Color handleColor);

// Draw a troll (simple humanoid shape)
void DrawTroll(Vector3 pos, bool highlighted);

#endif
