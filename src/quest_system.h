#ifndef QUEST_SYSTEM_H
#define QUEST_SYSTEM_H

#include "types.h"

// Loading
bool LoadQuest(const char* filepath, Quest* quest);
int LoadAllQuests(Quest* quests, int maxQuests);
void FreeQuest(Quest* quest);

// Quest lookup
int FindQuestByNPC(const Quest* quests, int questCount, NPCType npc);
int FindQuestById(const Quest* quests, int questCount, const char* id);

// Dialogue selection based on quest state
// Returns dialogue lines array and sets lineCount
// Also sets showAcceptPrompt to true if we should show accept/decline buttons
const char** GetQuestDialogue(const Quest* quest, const QuestProgress* progress,
                               const PlayerState* state, int* lineCount,
                               bool* showAcceptPrompt);

// Check if quest can be advanced (has required item, or can start)
bool CanAdvanceQuest(const Quest* quest, const QuestProgress* progress,
                     const PlayerState* state);

// Advance quest (removes items, gives rewards if completing)
// Returns true if quest was completed
bool AdvanceQuest(const Quest* quest, QuestProgress* progress, PlayerState* state);

// Initialize quest progress array to default values
void InitQuestProgress(QuestProgress* progress, int count);

#endif
