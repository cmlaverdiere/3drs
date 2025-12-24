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
    return ITEM_NONE;
}

// Map NPC name string to NPCType
static NPCType ParseNPCType(const char* name) {
    if (strcmp(name, "hans") == 0) return NPC_HANS;
    if (strcmp(name, "shopkeeper") == 0) return NPC_SHOPKEEPER;
    if (strcmp(name, "guard") == 0) return NPC_GUARD;
    if (strcmp(name, "cook") == 0) return NPC_COOK;
    return NPC_HANS;  // Default
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
    quest->npc = NPC_HANS;

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
        else if (strcmp(directive, "npc") == 0) {
            char npcName[32];
            if (sscanf(line, "npc %31s", npcName) == 1) {
                quest->npc = ParseNPCType(npcName);
            }
        }
        else if (strcmp(directive, "objective") == 0) {
            char itemName[32];
            if (sscanf(line, "objective %31s", itemName) == 1) {
                if (quest->objectiveCount < MAX_QUEST_OBJECTIVES) {
                    quest->objectives[quest->objectiveCount] = ParseItemType(itemName);
                    quest->objectiveCount++;
                }
            }
        }
        else if (strcmp(directive, "reward_gil") == 0) {
            sscanf(line, "reward_gil %d", &quest->rewardGil);
        }
        else if (strcmp(directive, "reward_quest_points") == 0) {
            sscanf(line, "reward_quest_points %d", &quest->rewardQuestPoints);
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

    printf("Loaded quest: %s (%s) with %d objectives\n", quest->name, quest->id, quest->objectiveCount);
    return true;
}

int LoadAllQuests(Quest* quests, int maxQuests) {
    int count = 0;

    DIR* dir = opendir("quests");
    if (!dir) {
        printf("No quests directory found\n");
        return 0;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL && count < maxQuests) {
        // Check for .quest extension
        const char* name = entry->d_name;
        size_t len = strlen(name);
        if (len > 6 && strcmp(name + len - 6, ".quest") == 0) {
            char filepath[256];
            snprintf(filepath, sizeof(filepath), "quests/%s", name);

            if (LoadQuest(filepath, &quests[count])) {
                count++;
            }
        }
    }

    closedir(dir);
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
        if (quests[i].loaded && quests[i].npc == npc) {
            return i;
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
                               const PlayerState* state, int* lineCount,
                               bool* showAcceptPrompt) {
    *showAcceptPrompt = false;

    if (progress->state == QUEST_NOT_STARTED) {
        *lineCount = quest->dialogueStartCount;
        *showAcceptPrompt = true;  // Show accept/decline after intro
        return (const char**)quest->dialogueStart;
    }
    else if (progress->state == QUEST_IN_PROGRESS) {
        int obj = progress->currentObjective;
        if (obj < quest->objectiveCount) {
            ItemType needed = quest->objectives[obj];

            if (HasItem(state, needed)) {
                // Player has the item - show turnin dialogue
                *lineCount = quest->dialogueTurninCount[obj];
                return (const char**)quest->dialogueTurnin[obj];
            } else {
                // Player doesn't have item - show "go get it" dialogue
                *lineCount = quest->dialogueStageCount[obj];
                return (const char**)quest->dialogueStage[obj];
            }
        }
    }
    else if (progress->state == QUEST_COMPLETE) {
        *lineCount = quest->dialogueCompleteCount;
        return (const char**)quest->dialogueComplete;
    }

    *lineCount = 0;
    return nullptr;
}

bool CanAdvanceQuest(const Quest* quest, const QuestProgress* progress,
                     const PlayerState* state) {
    if (progress->state == QUEST_NOT_STARTED) {
        return true;  // Can always accept a new quest
    }
    else if (progress->state == QUEST_IN_PROGRESS) {
        int obj = progress->currentObjective;
        if (obj < quest->objectiveCount) {
            return HasItem(state, quest->objectives[obj]);
        }
    }
    return false;
}

bool AdvanceQuest(const Quest* quest, QuestProgress* progress, PlayerState* state) {
    if (progress->state == QUEST_NOT_STARTED) {
        // Start the quest
        progress->state = QUEST_IN_PROGRESS;
        progress->currentObjective = 0;
        return false;
    }
    else if (progress->state == QUEST_IN_PROGRESS) {
        int obj = progress->currentObjective;
        if (obj < quest->objectiveCount) {
            ItemType needed = quest->objectives[obj];

            if (HasItem(state, needed)) {
                // Remove the item
                RemoveItem(state, needed);

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

                    return true;  // Quest completed!
                }
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
