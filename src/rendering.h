#ifndef RENDERING_H
#define RENDERING_H

#include "raylib.h"
#include "types.h"

// Draw a simple sword shape
void DrawSword(Vector3 pos, Color bladeColor, Color handleColor);

// Draw a troll (simple humanoid shape)
void DrawTroll(Vector3 pos, float facingAngle, bool highlighted);

// Draw a cow
void DrawCow(Vector3 pos, float facingAngle, bool highlighted);

// Draw any enemy by type
void DrawEnemy(const Enemy& enemy, bool highlighted);

// Draw item on ground
void DrawWorldItem(ItemType type, Vector3 pos);

// Draw a tree
void DrawTree(Vector3 pos, bool highlighted);

#endif
