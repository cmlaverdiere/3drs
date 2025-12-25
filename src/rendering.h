#ifndef RENDERING_H
#define RENDERING_H

#include "raylib.h"
#include "types.h"

// Forward declare EntityModels (defined in game_init.h)
struct EntityModels;

// Draw primitives using models (proper normals for lighting)
void DrawModelCube(const EntityModels* models, Vector3 pos, float width, float height, float depth, Color color);
void DrawModelSphere(const EntityModels* models, Vector3 pos, float radius, Color color);
void DrawModelCylinder(const EntityModels* models, Vector3 pos, float radiusBottom, float radiusTop, float height, Color color);

// Draw a simple sword shape
void DrawSword(const EntityModels* models, Vector3 pos, Color bladeColor, Color handleColor);

// Draw a troll (simple humanoid shape)
void DrawTroll(const EntityModels* models, Vector3 pos, float facingAngle, bool highlighted);

// Draw a cow
void DrawCow(const EntityModels* models, Vector3 pos, float facingAngle, bool highlighted);

// Draw a scorpion
void DrawScorpion(const EntityModels* models, Vector3 pos, float facingAngle, bool highlighted);

// Draw any enemy by type
void DrawEnemy(const EntityModels* models, const Enemy& enemy, bool highlighted);

// Draw item on ground
void DrawWorldItem(const EntityModels* models, ItemType type, Vector3 pos);

// Draw a tree
void DrawTree(const EntityModels* models, Vector3 pos, TreeType type, bool highlighted);

// Draw a humanoid NPC
void DrawHumanoid(const EntityModels* models, Vector3 pos, float facingAngle,
                  Color skinColor, Color shirtColor, Color pantsColor, float heightScale);

// Draw NPC by type
void DrawNPC(const EntityModels* models, const NPC& npc);

// Draw all NPCs
void DrawNPCs(const EntityModels* models, const NPC* npcs, int npcCount);

// Draw light sources (lamps, campfires)
void DrawLamp(const EntityModels* models, Vector3 pos, bool lit);
void DrawCampfire(const EntityModels* models, Vector3 pos);
void DrawLightSource(const EntityModels* models, const LightSource& light, bool lampsOn);
void DrawLightSources(const EntityModels* models, const LightSource* lights, int lightCount, bool lampsOn);

#endif
