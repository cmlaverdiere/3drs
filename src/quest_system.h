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
// talkingTo: which NPC the player is currently talking to (for multi-NPC quests)
const char** GetQuestDialogue(const Quest* quest, const QuestProgress* progress,
                               const PlayerState* state, NPCType talkingTo,
                               int* lineCount, bool* showAcceptPrompt);

// Check if quest can be advanced (has required item, or can start)
// talkingTo: which NPC the player is currently talking to
bool CanAdvanceQuest(const Quest* quest, const QuestProgress* progress,
                     const PlayerState* state, NPCType talkingTo);

// Advance quest (removes items, gives rewards if completing)
// Returns true if quest was completed
// talkingTo: which NPC the player is currently talking to
bool AdvanceQuest(const Quest* quest, QuestProgress* progress,
                  PlayerState* state, NPCType talkingTo);

// Initialize quest progress array to default values
void InitQuestProgress(QuestProgress* progress, int count);

#endif
