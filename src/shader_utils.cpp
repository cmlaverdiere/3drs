#include "shader_utils.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

// Read entire file into dynamically allocated string
static char* ReadFileContents(const char* fileName) {
    FILE* file = fopen(fileName, "r");
    if (!file) {
        TraceLog(LOG_WARNING, "SHADER: Failed to open file: %s", fileName);
        return nullptr;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* contents = (char*)malloc(size + 1);
    if (!contents) {
        fclose(file);
        return nullptr;
    }

    size_t read = fread(contents, 1, size, file);
    contents[read] = '\0';
    fclose(file);

    return contents;
}

// Get directory from file path (returns "shaders/" for "shaders/grass.fs")
static void GetDirectory(const char* filePath, char* dir, int maxLen) {
    const char* lastSlash = strrchr(filePath, '/');
    if (lastSlash) {
        int len = (int)(lastSlash - filePath + 1);
        if (len >= maxLen) len = maxLen - 1;
        strncpy(dir, filePath, len);
        dir[len] = '\0';
    } else {
        dir[0] = '\0';
    }
}

char* PreprocessShaderSource(const char* fileName) {
    char* source = ReadFileContents(fileName);
    if (!source) return nullptr;

    // Get base directory for relative includes
    char baseDir[256];
    GetDirectory(fileName, baseDir, sizeof(baseDir));

    // Allocate result buffer (start with 2x source size for includes)
    size_t resultSize = strlen(source) * 4 + 1;
    char* result = (char*)malloc(resultSize);
    if (!result) {
        free(source);
        return nullptr;
    }
    result[0] = '\0';
    size_t resultLen = 0;

    // Process line by line
    char* line = source;
    while (line && *line) {
        char* nextLine = strchr(line, '\n');
        size_t lineLen = nextLine ? (size_t)(nextLine - line) : strlen(line);

        // Check for #include directive
        if (strncmp(line, "#include", 8) == 0) {
            // Parse include path
            const char* quote1 = strchr(line, '"');
            const char* quote2 = quote1 ? strchr(quote1 + 1, '"') : nullptr;

            if (quote1 && quote2) {
                // Extract include path
                int pathLen = (int)(quote2 - quote1 - 1);
                char includePath[256];
                snprintf(includePath, sizeof(includePath), "%s%.*s", baseDir, pathLen, quote1 + 1);

                // Read and insert include file
                char* includeContents = ReadFileContents(includePath);
                if (includeContents) {
                    size_t includeLen = strlen(includeContents);

                    // Grow result buffer if needed
                    while (resultLen + includeLen + 2 >= resultSize) {
                        resultSize *= 2;
                        result = (char*)realloc(result, resultSize);
                    }

                    // Add include contents with newline
                    strcat(result, includeContents);
                    strcat(result, "\n");
                    resultLen += includeLen + 1;

                    free(includeContents);
                    TraceLog(LOG_INFO, "SHADER: Included %s", includePath);
                } else {
                    TraceLog(LOG_WARNING, "SHADER: Failed to include %s", includePath);
                }
            }
        } else {
            // Regular line - copy to result
            while (resultLen + lineLen + 2 >= resultSize) {
                resultSize *= 2;
                result = (char*)realloc(result, resultSize);
            }

            strncat(result, line, lineLen);
            strcat(result, "\n");
            resultLen += lineLen + 1;
        }

        // Move to next line
        line = nextLine ? nextLine + 1 : nullptr;
    }

    free(source);
    return result;
}

Shader LoadShaderWithIncludes(const char* vsFileName, const char* fsFileName) {
    char* vsSource = nullptr;
    char* fsSource = nullptr;

    if (vsFileName) {
        vsSource = PreprocessShaderSource(vsFileName);
    }
    if (fsFileName) {
        fsSource = PreprocessShaderSource(fsFileName);
    }

    Shader shader = LoadShaderFromMemory(vsSource, fsSource);

    if (vsSource) free(vsSource);
    if (fsSource) free(fsSource);

    return shader;
}
