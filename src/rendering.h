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

// Draw any enemy by type (non-const models because custom monsters may swap shaders)
void DrawEnemy(EntityModels* models, const Enemy& enemy, bool highlighted,
               const CustomMonster* customMonsters = nullptr, int customMonsterCount = 0);

// Draw a custom monster from data (non-const because it may swap shaders)
void DrawCustomMonster(EntityModels* models, const CustomMonster* monster,
                       Vector3 pos, float facingAngle, bool highlighted);

// Draw item on ground
void DrawWorldItem(const EntityModels* models, ItemType type, Vector3 pos);

// Draw a tree
void DrawTree(const EntityModels* models, Vector3 pos, TreeType type, bool highlighted);

// Draw a rock (ore deposit)
void DrawRock(const EntityModels* models, Vector3 pos, RockType type, bool highlighted);

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

// Draw ladders
void DrawLadder(const EntityModels* models, const Ladder& ladder, bool highlighted);
void DrawLadders(const EntityModels* models, const Ladder* ladders, int ladderCount, const Ladder* highlightedLadder);

#endif
