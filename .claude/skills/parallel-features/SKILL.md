---
name: parallel-features
description: Implement multiple game features in parallel using jj workspaces and subagents. Use when the user asks to work on multiple features simultaneously, says "in parallel", or lists several features to implement at once.
---

# Parallel Feature Development

Implement multiple features in parallel using jj workspaces and subagents, then merge the results.

## Prerequisites

- jj (Jujutsu) version control
- Workspaces directory exists at `workspaces/`
- Feature ideas documented in `ideas.txt`

## Workflow

### 1. Parse Features

Extract the list of features from the user's request. Check `ideas.txt` for feature descriptions and requirements.

### 2. Create Workspaces

For each feature, create a jj workspace:

```bash
jj workspace add workspaces/<feature-name>
```

### 3. Launch Subagents in Parallel

Launch a Task agent for each feature IN A SINGLE MESSAGE with multiple Task tool calls.

For each feature, use:
- `subagent_type`: "general-purpose"
- `prompt`: Must include all of the following:

```
Feature: <name>
Description: <from ideas.txt>

IMPORTANT: You are working in a jj workspace. Your working directory is:
/Users/chris.laverdiere/dev/3drs/workspaces/<feature-name>

All file edits, reads, and operations must happen within this workspace directory.
The workspace is a full copy of the repo - edit files here, not in the main repo.

Instructions:
1. cd to the workspace directory first
2. Read CLAUDE.md for coding guidelines (especially "Adding New Content" sections)
3. Implement the feature fully
4. Build to verify: cmake -B build && cmake --build build
5. Test: ./build/game --test
6. Commit your changes: jj commit -m "<feature>: <summary>"

<detailed feature requirements here>
```

### 4. Wait for Completion

Use TaskOutput to wait for all subagents to complete.

### 5. Analyze and Merge

Once all subagents finish:

1. **Review each workspace's changes:**
   ```bash
   cd workspaces/<feature> && jj log -r @ && jj diff -r @-
   ```

2. **Identify conflicts** - Common conflict areas:
   - `src/types.h`: Enum additions must be ordered consistently
   - `src/types.cpp`: Arrays must match enum ordering
   - `src/main.cpp`: Input handling and render loop changes
   - `src/hud.cpp`: UI rendering additions

3. **Merge strategy:**
   - Return to main workspace
   - For enums: Combine all additions in consistent order
   - For parallel arrays: Match enum ordering exactly
   - For main.cpp: Interleave input handlers and render calls
   - For new files: Include all

4. **Apply merged changes** to the main workspace by reading diffs and applying edits

5. **Verify:**
   ```bash
   cmake -B build && cmake --build build
   ./build/game --test
   ```

6. **Cleanup workspaces:**
   ```bash
   jj workspace forget <name>
   rm -rf workspaces/<feature>
   ```

### 6. Final Commit

Create a single commit with all merged features:
```bash
jj commit -m "Add <feature1>, <feature2>, <feature3>"
```

## Example

**User:** "Work on banking, minimap, and food features in parallel"

1. Read `ideas.txt` for feature specs
2. Create workspaces:
   ```bash
   jj workspace add workspaces/banking
   jj workspace add workspaces/minimap
   jj workspace add workspaces/food
   ```
3. Launch 3 subagents simultaneously (single message, 3 Task tools), each working in their own workspace directory
4. Wait for all to complete
5. Review diffs from each workspace, resolve conflicts, apply merged changes to main
6. Build and test in main workspace
7. Clean up workspaces
8. Commit and report results

## Notes

- Subagents MUST work within their workspace directory, not the main repo
- Always check `ideas.txt` before starting
- Follow CLAUDE.md "Adding New Content" guidelines
- Prefer smaller, focused features that minimize conflicts
- If merging is complex, do it incrementally (merge 2, verify, merge next)
