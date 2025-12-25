#ifndef SHADER_UTILS_H
#define SHADER_UTILS_H

#include "raylib.h"

// Load shader with #include directive support
// Processes lines like: #include "common/lighting.glsl"
Shader LoadShaderWithIncludes(const char* vsFileName, const char* fsFileName);

// Process a shader source file, expanding #include directives
// Returns dynamically allocated string (caller must free)
char* PreprocessShaderSource(const char* fileName);

#endif
