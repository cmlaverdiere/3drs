#include "shader_utils.h"
#include "rlgl.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <string>

static int g_shaderFailures = 0;

static bool ReadFileContents(const std::string& fileName, std::string* out) {
    FILE* file = fopen(fileName.c_str(), "rb");
    if (!file) {
        TraceLog(LOG_WARNING, "SHADER: Failed to open file: %s", fileName.c_str());
        return false;
    }
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    out->resize(size > 0 ? (size_t)size : 0);
    size_t read = size > 0 ? fread(&(*out)[0], 1, (size_t)size, file) : 0;
    out->resize(read);
    fclose(file);
    return true;
}

static std::string DirectoryOf(const std::string& path) {
    size_t slash = path.find_last_of('/');
    return slash == std::string::npos ? std::string() : path.substr(0, slash + 1);
}

// Normalizes "shaders/common/../common/x.glsl" style paths so include-once works.
static std::string NormalizePath(const std::string& path) {
    std::string result;
    size_t start = 0;
    std::string parts[64];
    int count = 0;
    while (start <= path.size()) {
        size_t end = path.find('/', start);
        if (end == std::string::npos) end = path.size();
        std::string part = path.substr(start, end - start);
        if (part == "..") {
            if (count > 0) count--;
        } else if (!part.empty() && part != "." && count < 64) {
            parts[count++] = part;
        }
        start = end + 1;
    }
    for (int i = 0; i < count; i++) {
        if (i) result += '/';
        result += parts[i];
    }
    return result;
}

static bool ExpandIncludes(const std::string& fileName, std::set<std::string>& included,
                           std::string* out, int depth) {
    if (depth > 16) {
        TraceLog(LOG_ERROR, "SHADER: Include depth exceeded at %s", fileName.c_str());
        return false;
    }
    std::string source;
    if (!ReadFileContents(fileName, &source)) return false;
    std::string baseDir = DirectoryOf(fileName);

    size_t pos = 0;
    while (pos < source.size()) {
        size_t eol = source.find('\n', pos);
        if (eol == std::string::npos) eol = source.size();
        std::string line = source.substr(pos, eol - pos);
        pos = eol + 1;

        size_t first = line.find_first_not_of(" \t");
        if (first != std::string::npos && line.compare(first, 8, "#include") == 0) {
            size_t q1 = line.find('"', first);
            size_t q2 = q1 == std::string::npos ? q1 : line.find('"', q1 + 1);
            if (q1 == std::string::npos || q2 == std::string::npos) {
                TraceLog(LOG_ERROR, "SHADER: Malformed include in %s: %s", fileName.c_str(), line.c_str());
                return false;
            }
            std::string includePath = NormalizePath(baseDir + line.substr(q1 + 1, q2 - q1 - 1));
            if (included.count(includePath)) continue;
            included.insert(includePath);
            if (!ExpandIncludes(includePath, included, out, depth + 1)) {
                TraceLog(LOG_ERROR, "SHADER: Failed to include %s from %s", includePath.c_str(), fileName.c_str());
                return false;
            }
            continue;
        }
        *out += line;
        *out += '\n';
    }
    return true;
}

// Defines go directly after #version, which must stay the first directive.
static std::string InjectDefines(const std::string& source, const char* defines) {
    if (!defines || !*defines) return source;
    std::string block;
    const char* p = defines;
    while (*p) {
        while (*p == ' ' || *p == ';') p++;
        const char* end = p;
        while (*end && *end != ';') end++;
        if (end > p) block += "#define " + std::string(p, end - p) + "\n";
        p = end;
    }
    size_t versionLine = source.find("#version");
    if (versionLine == std::string::npos) return block + source;
    size_t eol = source.find('\n', versionLine);
    if (eol == std::string::npos) return source + "\n" + block;
    return source.substr(0, eol + 1) + block + source.substr(eol + 1);
}

char* PreprocessShaderSource(const char* fileName, const char* defines) {
    std::set<std::string> included;
    std::string expanded;
    std::string path = NormalizePath(fileName);
    included.insert(path);
    if (!ExpandIncludes(path, included, &expanded, 0)) return nullptr;
    expanded = InjectDefines(expanded, defines);
    char* result = (char*)malloc(expanded.size() + 1);
    memcpy(result, expanded.c_str(), expanded.size() + 1);
    return result;
}

Shader LoadShaderVariant(const char* vsFileName, const char* fsFileName, const char* defines) {
    char* vsSource = vsFileName ? PreprocessShaderSource(vsFileName, defines) : nullptr;
    char* fsSource = fsFileName ? PreprocessShaderSource(fsFileName, defines) : nullptr;
    bool missing = (vsFileName && !vsSource) || (fsFileName && !fsSource);

    Shader shader = missing ? Shader{} : LoadShaderFromMemory(vsSource, fsSource);
    if (vsSource) free(vsSource);
    if (fsSource) free(fsSource);

    if (missing || shader.id == 0 || shader.id == rlGetShaderIdDefault()) {
        g_shaderFailures++;
        TraceLog(LOG_ERROR, "SHADER FAILED: %s + %s [%s]", vsFileName ? vsFileName : "(default)",
                 fsFileName ? fsFileName : "(default)", defines ? defines : "");
    }
    return shader;
}

Shader LoadShaderWithIncludes(const char* vsFileName, const char* fsFileName) {
    return LoadShaderVariant(vsFileName, fsFileName, nullptr);
}

int GetShaderLoadFailures() {
    return g_shaderFailures;
}
