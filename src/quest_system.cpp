#include "quest_system.h"
#include "inventory.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <dirent.h>

// Map item name string to ItemType
static ItemType ParseItemType(const char* name) {
    if (strcmp(name, "bones") == 0) return ITEM_BONES;
    if (strcmp(name, "chitin") == 0) return ITEM_CHITIN;
    if (strcmp(name, "gil") == 0) return ITEM_GIL;
    if (strcmp(name, "logs") == 0) return ITEM_LOGS;
    if (strcmp(name, "cow_hide") == 0) return ITEM_COW_HIDE;
    if (strcmp(name, "bronze_shortsword") == 0) return ITEM_BRONZE_SHORTSWORD;
    if (strcmp(name, "bronze_axe") == 0) return ITEM_BRONZE_AXE;
    if (strcmp(name, "iron_2h_sword") == 0) return ITEM_IRON_2H_SWORD;
    // Trading Expedition quest items
    if (strcmp(name, "trade_manifest") == 0) return ITEM_TRADE_MANIFEST;
    if (strcmp(name, "silk") == 0) return ITEM_SILK;
    if (strcmp(name, "spice") == 0) return ITEM_SPICE;
    if (strcmp(name, "iron_ore") == 0) return ITEM_IRON_ORE;
    if (strcmp(name, "rare_wine") == 0) return ITEM_RARE_WINE;
    if (strcmp(name, "bandit_orders") == 0) return ITEM_BANDIT_ORDERS;
    if (strcmp(name, "desert_artifact") == 0) return ITEM_DESERT_ARTIFACT;
    if (strcmp(name, "trade_ledger") == 0) return ITEM_TRADE_LEDGER;
    return ITEM_NONE;
}

// Map NPC name string to NPCType
static NPCType ParseNPCType(const char* name) {
    if (strcmp(name, "hans") == 0) return NPC_HANS;
    if (strcmp(name, "shopkeeper") == 0) return NPC_SHOPKEEPER;
    if (strcmp(name, "guard") == 0) return NPC_GUARD;
    if (strcmp(name, "cook") == 0) return NPC_COOK;
    // Trading Expedition quest NPCs
    if (strcmp(name, "varrock_trader") == 0) return NPC_VARROCK_TRADER;
    if (strcmp(name, "varrock_bartender") == 0) return NPC_VARROCK_BARTENDER;
    if (strcmp(name, "alkharid_silk") == 0) return NPC_ALKHARID_SILK;
    if (strcmp(name, "alkharid_spice") == 0) return NPC_ALKHARID_SPICE;
    return NPC_HANS;  // Default
}

// Parse objective type string
static ObjectiveType ParseObjectiveType(const char* name) {
    if (strcmp(name, "item") == 0) return OBJ_ITEM;
    if (strcmp(name, "talk_to") == 0) return OBJ_TALK_TO;
    return OBJ_ITEM;  // Default
}

// Allocate and copy a string
static char* DuplicateString(const char* str) {
    size_t len = strlen(str);
    char* dup = (char*)malloc(len + 1);
    if (dup) {
        strcpy(dup, str);
    }
    return dup;
}

// Parse a dialogue block (reads until line with just ".")
// Returns array of strings (caller owns) and sets count
static char** ParseDialogueBlock(FILE* file, int* count) {
    char** lines = (char**)malloc(sizeof(char*) * MAX_QUEST_DIALOGUE_LINES);
    *count = 0;

    char buffer[512];
    while (fgets(buffer, sizeof(buffer), file)) {
        // Remove trailing newline
        size_t len = strlen(buffer);
        while (len > 0 && (buffer[len-1] == '\n' || buffer[len-1] == '\r')) {
            buffer[--len] = '\0';
        }

        // Single "." ends the block
        if (strcmp(buffer, ".") == 0) {
            break;
        }

        // Skip empty lines at start
        if (*count == 0 && len == 0) continue;

        // Add line
        if (*count < MAX_QUEST_DIALOGUE_LINES) {
            lines[*count] = DuplicateString(buffer);
            (*count)++;
        }
    }

    return lines;
}

bool LoadQuest(const char* filepath, Quest* quest) {
    FILE* file = fopen(filepath, "r");
    if (!file) {
        printf("Failed to open quest file: %s\n", filepath);
        return false;
    }

    // Initialize quest
    memset(quest, 0, sizeof(Quest));
    quest->startNPC = NPC_HANS;

    char line[512];
    while (fgets(line, sizeof(line), file)) {
        // Remove trailing newline
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
            line[--len] = '\0';
        }

        // Skip empty lines and comments
        if (len == 0 || line[0] == '#') continue;

        // Parse directive
        char directive[64];
        if (sscanf(line, "%63s", directive) != 1) continue;

        if (strcmp(directive, "quest") == 0) {
            sscanf(line, "quest %31s", quest->id);
        }
        else if (strcmp(directive, "name") == 0) {
            // Copy everything after "name "
            const char* nameStart = line + 5;
            while (*nameStart == ' ') nameStart++;
            strncpy(quest->name, nameStart, sizeof(quest->name) - 1);
        }
        else if (strcmp(directive, "start_npc") == 0) {
            // New directive: which NPC starts the quest
            char npcName[32];
            if (sscanf(line, "start_npc %31s", npcName) == 1) {
                quest->startNPC = ParseNPCType(npcName);
                // Also add to NPCs list
                if (quest->npcCount < MAX_QUEST_NPCS) {
                    quest->npcs[quest->npcCount++] = quest->startNPC;
                }
            }
        }
        else if (strcmp(directive, "npc") == 0) {
            // Legacy single NPC format OR new multi-NPC format
            // Try to parse multiple NPCs from the line
            char* p = line + 4; // Skip "npc "
            char npcName[32];
            bool first = true;
            while (sscanf(p, "%31s", npcName) == 1) {
                NPCType npc = ParseNPCType(npcName);
                // Add to NPCs list if not already there
                bool found = false;
                for (int i = 0; i < quest->npcCount; i++) {
                    if (quest->npcs[i] == npc) { found = true; break; }
                }
                if (!found && quest->npcCount < MAX_QUEST_NPCS) {
                    quest->npcs[quest->npcCount++] = npc;
                }
                // First NPC in legacy format becomes startNPC
                if (first && quest->startNPC == NPC_HANS) {
                    quest->startNPC = npc;
                }
                first = false;
                // Move pointer past this NPC name
                p += strlen(npcName);
                while (*p == ' ') p++;
                if (*p == '\0') break;
            }
        }
        else if (strcmp(directive, "objective") == 0) {
            // New format: objective <type> <target> [npc]
            // OR legacy format: objective <item_name>
            char arg1[32], arg2[32], arg3[32];
            int parsed = sscanf(line, "objective %31s %31s %31s", arg1, arg2, arg3);

            if (quest->objectiveCount < MAX_QUEST_OBJECTIVES) {
                QuestObjective* obj = &quest->objectives[quest->objectiveCount];

                if (parsed >= 2 && (strcmp(arg1, "item") == 0 || strcmp(arg1, "talk_to") == 0)) {
                    // New format
                    obj->type = ParseObjectiveType(arg1);
                    if (obj->type == OBJ_ITEM) {
                        obj->item = ParseItemType(arg2);
                        obj->targetNPC = (parsed >= 3) ? ParseNPCType(arg3) : quest->startNPC;
                    } else if (obj->type == OBJ_TALK_TO) {
                        obj->item = ITEM_NONE;
                        obj->targetNPC = ParseNPCType(arg2);
                    }
                } else {
                    // Legacy format: objective <item_name> (uses startNPC)
                    obj->type = OBJ_ITEM;
                    obj->item = ParseItemType(arg1);
                    obj->targetNPC = quest->startNPC;
                }
                quest->objectiveCount++;
            }
        }
        else if (strcmp(directive, "reward_gil") == 0) {
            sscanf(line, "reward_gil %d", &quest->rewardGil);
        }
        else if (strcmp(directive, "reward_quest_points") == 0) {
            sscanf(line, "reward_quest_points %d", &quest->rewardQuestPoints);
        }
        else if (strcmp(directive, "reward_item") == 0) {
            char itemName[32];
            if (sscanf(line, "reward_item %31s", itemName) == 1) {
                if (quest->rewardItemCount < MAX_QUEST_REWARD_ITEMS) {
                    quest->rewardItems[quest->rewardItemCount++] = ParseItemType(itemName);
                }
            }
        }
        else if (strcmp(directive, "dialogue_start") == 0) {
            quest->dialogueStart = ParseDialogueBlock(file, &quest->dialogueStartCount);
        }
        else if (strncmp(directive, "dialogue_stage_", 15) == 0) {
            int stageNum = 0;
            if (sscanf(directive, "dialogue_stage_%d", &stageNum) == 1) {
                if (stageNum >= 1 && stageNum <= MAX_QUEST_OBJECTIVES) {
                    quest->dialogueStage[stageNum - 1] = ParseDialogueBlock(file, &quest->dialogueStageCount[stageNum - 1]);
                }
            }
        }
        else if (strncmp(directive, "dialogue_turnin_", 16) == 0) {
            int stageNum = 0;
            if (sscanf(directive, "dialogue_turnin_%d", &stageNum) == 1) {
                if (stageNum >= 1 && stageNum <= MAX_QUEST_OBJECTIVES) {
                    quest->dialogueTurnin[stageNum - 1] = ParseDialogueBlock(file, &quest->dialogueTurninCount[stageNum - 1]);
                }
            }
        }
        else if (strcmp(directive, "dialogue_complete") == 0) {
            quest->dialogueComplete = ParseDialogueBlock(file, &quest->dialogueCompleteCount);
        }
    }

    fclose(file);
    quest->loaded = true;

    printf("Loaded quest: %s (%s) with %d objectives, %d NPCs\n",
           quest->name, quest->id, quest->objectiveCount, quest->npcCount);
    return true;
}

int LoadAllQuests(Quest* quests, int maxQuests) {
    int count = 0;

    DIR* dir = opendir("quests");
    if (!dir) {
        printf("No quests directory found\n");
        return 0;
    }

    // Collect all quest filenames first
    char filenames[MAX_QUESTS][64];
    int fileCount = 0;

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL && fileCount < MAX_QUESTS) {
        const char* name = entry->d_name;
        size_t len = strlen(name);
        if (len > 6 && len < 64 && strcmp(name + len - 6, ".quest") == 0) {
            strncpy(filenames[fileCount], name, 63);
            filenames[fileCount][63] = '\0';
            fileCount++;
        }
    }
    closedir(dir);

    // Sort filenames alphabetically for consistent quest indices
    // Simple bubble sort (small number of quests)
    for (int i = 0; i < fileCount - 1; i++) {
        for (int j = 0; j < fileCount - i - 1; j++) {
            if (strcmp(filenames[j], filenames[j + 1]) > 0) {
                char temp[64];
                strcpy(temp, filenames[j]);
                strcpy(filenames[j], filenames[j + 1]);
                strcpy(filenames[j + 1], temp);
            }
        }
    }

    // Load quests in sorted order
    for (int i = 0; i < fileCount && count < maxQuests; i++) {
        char filepath[256];
        snprintf(filepath, sizeof(filepath), "quests/%s", filenames[i]);

        if (LoadQuest(filepath, &quests[count])) {
            count++;
        }
    }

    printf("Loaded %d quests\n", count);
    return count;
}

void FreeQuest(Quest* quest) {
    if (quest->dialogueStart) {
        for (int i = 0; i < quest->dialogueStartCount; i++) {
            free(quest->dialogueStart[i]);
        }
        free(quest->dialogueStart);
    }

    for (int obj = 0; obj < MAX_QUEST_OBJECTIVES; obj++) {
        if (quest->dialogueStage[obj]) {
            for (int i = 0; i < quest->dialogueStageCount[obj]; i++) {
                free(quest->dialogueStage[obj][i]);
            }
            free(quest->dialogueStage[obj]);
        }
        if (quest->dialogueTurnin[obj]) {
            for (int i = 0; i < quest->dialogueTurninCount[obj]; i++) {
                free(quest->dialogueTurnin[obj][i]);
            }
            free(quest->dialogueTurnin[obj]);
        }
    }

    if (quest->dialogueComplete) {
        for (int i = 0; i < quest->dialogueCompleteCount; i++) {
            free(quest->dialogueComplete[i]);
        }
        free(quest->dialogueComplete);
    }

    memset(quest, 0, sizeof(Quest));
}

int FindQuestByNPC(const Quest* quests, int questCount, NPCType npc) {
    for (int i = 0; i < questCount; i++) {
        if (!quests[i].loaded) continue;

        // Check if this NPC is involved in the quest (multi-NPC support)
        for (int j = 0; j < quests[i].npcCount; j++) {
            if (quests[i].npcs[j] == npc) {
                return i;
            }
        }
    }
    return -1;
}

int FindQuestById(const Quest* quests, int questCount, const char* id) {
    for (int i = 0; i < questCount; i++) {
        if (quests[i].loaded && strcmp(quests[i].id, id) == 0) {
            return i;
        }
    }
    return -1;
}

const char** GetQuestDialogue(const Quest* quest, const QuestProgress* progress,
                               const PlayerState* state, NPCType talkingTo,
                               int* lineCount, bool* showAcceptPrompt) {
    *showAcceptPrompt = false;

    if (progress->state == QUEST_NOT_STARTED) {
        // Only the start NPC can give the intro
        if (talkingTo != quest->startNPC) {
            *lineCount = 0;
            return nullptr;
        }
        *lineCount = quest->dialogueStartCount;
        *showAcceptPrompt = true;  // Show accept/decline after intro
        return (const char**)quest->dialogueStart;
    }
    else if (progress->state == QUEST_IN_PROGRESS) {
        int currentObj = progress->currentObjective;

        // First check if this NPC is the target of the CURRENT objective
        if (currentObj < quest->objectiveCount) {
            const QuestObjective* objective = &quest->objectives[currentObj];

            if (talkingTo == objective->targetNPC) {
                // This is the NPC for the current objective
                if (objective->type == OBJ_TALK_TO) {
                    // "Talk to" objective - show turnin dialogue when talking to target
                    *lineCount = quest->dialogueTurninCount[currentObj];
                    return (const char**)quest->dialogueTurnin[currentObj];
                }
                else if (objective->type == OBJ_ITEM) {
                    if (HasItem(state, objective->item)) {
                        // Player has the item - show turnin dialogue
                        *lineCount = quest->dialogueTurninCount[currentObj];
                        return (const char**)quest->dialogueTurnin[currentObj];
                    } else {
                        // Player doesn't have item - show "go get it" dialogue
                        *lineCount = quest->dialogueStageCount[currentObj];
                        return (const char**)quest->dialogueStage[currentObj];
                    }
                }
            }
        }

        // Not the current objective's NPC - check if this NPC was a PREVIOUS objective's target
        // If so, repeat their turnin dialogue (so they don't revert to default dialogue)
        for (int obj = currentObj - 1; obj >= 0; obj--) {
            if (quest->objectives[obj].targetNPC == talkingTo) {
                // This NPC was involved in a previous objective - repeat their turnin
                if (quest->dialogueTurnin[obj] && quest->dialogueTurninCount[obj] > 0) {
                    *lineCount = quest->dialogueTurninCount[obj];
                    return (const char**)quest->dialogueTurnin[obj];
                }
            }
        }

        // NPC is part of quest but hasn't been reached yet - no dialogue
        *lineCount = 0;
        return nullptr;
    }
    else if (progress->state == QUEST_COMPLETE) {
        // Any involved NPC can show completion dialogue
        *lineCount = quest->dialogueCompleteCount;
        return (const char**)quest->dialogueComplete;
    }

    *lineCount = 0;
    return nullptr;
}

bool CanAdvanceQuest(const Quest* quest, const QuestProgress* progress,
                     const PlayerState* state, NPCType talkingTo) {
    if (progress->state == QUEST_NOT_STARTED) {
        // Only the start NPC can give the quest
        return talkingTo == quest->startNPC;
    }
    else if (progress->state == QUEST_IN_PROGRESS) {
        int obj = progress->currentObjective;
        if (obj < quest->objectiveCount) {
            const QuestObjective* objective = &quest->objectives[obj];

            // Must be talking to the right NPC
            if (talkingTo != objective->targetNPC) {
                return false;
            }

            if (objective->type == OBJ_TALK_TO) {
                return true;  // Just talking to them is enough
            }
            else if (objective->type == OBJ_ITEM) {
                return HasItem(state, objective->item);
            }
        }
    }
    return false;
}

bool AdvanceQuest(const Quest* quest, QuestProgress* progress,
                  PlayerState* state, NPCType talkingTo) {
    if (progress->state == QUEST_NOT_STARTED) {
        // Start the quest
        progress->state = QUEST_IN_PROGRESS;
        progress->currentObjective = 0;
        return false;
    }
    else if (progress->state == QUEST_IN_PROGRESS) {
        int obj = progress->currentObjective;
        if (obj < quest->objectiveCount) {
            const QuestObjective* objective = &quest->objectives[obj];

            // Verify we're talking to the right NPC
            if (talkingTo != objective->targetNPC) {
                return false;
            }

            if (objective->type == OBJ_ITEM) {
                if (HasItem(state, objective->item)) {
                    // Remove the item
                    RemoveItem(state, objective->item);
                }
            }
            // For OBJ_TALK_TO, no item to remove - just talking is the action

            // Advance to next objective
            progress->currentObjective++;

            // Check if quest is complete
            if (progress->currentObjective >= quest->objectiveCount) {
                progress->state = QUEST_COMPLETE;

                // Give rewards
                if (quest->rewardGil > 0) {
                    AddGil(state, quest->rewardGil);
                }
                state->questPoints += quest->rewardQuestPoints;

                // Give reward items
                for (int i = 0; i < quest->rewardItemCount; i++) {
                    AddToInventory(state, quest->rewardItems[i]);
                }

                return true;  // Quest completed!
            }
        }
    }
    return false;
}

void InitQuestProgress(QuestProgress* progress, int count) {
    for (int i = 0; i < count; i++) {
        progress[i].state = QUEST_NOT_STARTED;
        progress[i].currentObjective = 0;
    }
}
