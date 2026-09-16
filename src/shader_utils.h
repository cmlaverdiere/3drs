#ifndef SHADER_UTILS_H
#define SHADER_UTILS_H

#include "raylib.h"

// Load shader with recursive, include-once #include support
// Processes lines like: #include "common/lighting.glsl"
Shader LoadShaderWithIncludes(const char* vsFileName, const char* fsFileName);

// Same as LoadShaderWithIncludes, with semicolon-separated defines inserted after
// #version, e.g. "INSTANCED;ALPHA_TEST" or "CASCADES 4".
Shader LoadShaderVariant(const char* vsFileName, const char* fsFileName, const char* defines);

// Process a shader source file, expanding #include directives and defines
// Returns dynamically allocated string (caller must free)
char* PreprocessShaderSource(const char* fileName, const char* defines = nullptr);

// Number of shaders that failed to load or compile (raylib falls back silently)
int GetShaderLoadFailures();

#endif
