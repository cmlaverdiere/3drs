

#ifndef RAYLIB_H
#define RAYLIB_H

#include <stdarg.h>

#define RAYLIB_VERSION_MAJOR 5
#define RAYLIB_VERSION_MINOR 6
#define RAYLIB_VERSION_PATCH 0
#define RAYLIB_VERSION  "5.6-dev"

#if defined(_WIN32)
    #if defined(__TINYC__)
        #define __declspec(x) __attribute__((x))
    #endif
    #if defined(BUILD_LIBTYPE_SHARED)
        #define RLAPI __declspec(dllexport)
    #elif defined(USE_LIBTYPE_SHARED)
        #define RLAPI __declspec(dllimport)
    #endif
#else
    #if defined(BUILD_LIBTYPE_SHARED)
        #define RLAPI __attribute__((visibility("default")))
    #endif
#endif

#ifndef RLAPI
    #define RLAPI
#endif

#ifndef PI
    #define PI 3.14159265358979323846f
#endif
#ifndef DEG2RAD
    #define DEG2RAD (PI/180.0f)
#endif
#ifndef RAD2DEG
    #define RAD2DEG (180.0f/PI)
#endif

#ifndef RL_MALLOC
    #define RL_MALLOC(sz)       malloc(sz)
#endif
#ifndef RL_CALLOC
    #define RL_CALLOC(n,sz)     calloc(n,sz)
#endif
#ifndef RL_REALLOC
    #define RL_REALLOC(ptr,sz)  realloc(ptr,sz)
#endif
#ifndef RL_FREE
    #define RL_FREE(ptr)        free(ptr)
#endif

#if defined(__cplusplus)
    #define CLITERAL(type)      type
#else
    #define CLITERAL(type)      (type)
#endif

#if !defined(_MSC_VER) && (defined(__cplusplus) && __cplusplus < 201103L)
    #error "C++11 or later is required. Add -std=c++11"
#endif

#define RL_COLOR_TYPE
#define RL_RECTANGLE_TYPE
#define RL_VECTOR2_TYPE
#define RL_VECTOR3_TYPE
#define RL_VECTOR4_TYPE
#define RL_QUATERNION_TYPE
#define RL_MATRIX_TYPE

#define LIGHTGRAY  CLITERAL(Color){ 200, 200, 200, 255 }
#define GRAY       CLITERAL(Color){ 130, 130, 130, 255 }
#define DARKGRAY   CLITERAL(Color){ 80, 80, 80, 255 }
#define YELLOW     CLITERAL(Color){ 253, 249, 0, 255 }
#define GOLD       CLITERAL(Color){ 255, 203, 0, 255 }
#define ORANGE     CLITERAL(Color){ 255, 161, 0, 255 }
#define PINK       CLITERAL(Color){ 255, 109, 194, 255 }
#define RED        CLITERAL(Color){ 230, 41, 55, 255 }
#define MAROON     CLITERAL(Color){ 190, 33, 55, 255 }
#define GREEN      CLITERAL(Color){ 0, 228, 48, 255 }
#define LIME       CLITERAL(Color){ 0, 158, 47, 255 }
#define DARKGREEN  CLITERAL(Color){ 0, 117, 44, 255 }
#define SKYBLUE    CLITERAL(Color){ 102, 191, 255, 255 }
#define BLUE       CLITERAL(Color){ 0, 121, 241, 255 }
#define DARKBLUE   CLITERAL(Color){ 0, 82, 172, 255 }
#define PURPLE     CLITERAL(Color){ 200, 122, 255, 255 }
#define VIOLET     CLITERAL(Color){ 135, 60, 190, 255 }
#define DARKPURPLE CLITERAL(Color){ 112, 31, 126, 255 }
#define BEIGE      CLITERAL(Color){ 211, 176, 131, 255 }
#define BROWN      CLITERAL(Color){ 127, 106, 79, 255 }
#define DARKBROWN  CLITERAL(Color){ 76, 63, 47, 255 }

#define WHITE      CLITERAL(Color){ 255, 255, 255, 255 }
#define BLACK      CLITERAL(Color){ 0, 0, 0, 255 }
#define BLANK      CLITERAL(Color){ 0, 0, 0, 0 }
#define MAGENTA    CLITERAL(Color){ 255, 0, 255, 255 }
#define RAYWHITE   CLITERAL(Color){ 245, 245, 245, 255 }

#if (defined(__STDC__) && __STDC_VERSION__ >= 199901L) || (defined(_MSC_VER) && _MSC_VER >= 1800)
    #include <stdbool.h>
#elif !defined(__cplusplus) && !defined(bool)
    typedef enum bool { false = 0, true = !false } bool;
    #define RL_BOOL_TYPE
#endif

typedef struct Vector2 {
    float x;
    float y;
} Vector2;

typedef struct Vector3 {
    float x;
    float y;
    float z;
} Vector3;

typedef struct Vector4 {
    float x;
    float y;
    float z;
    float w;
} Vector4;

typedef Vector4 Quaternion;

typedef struct Matrix {
    float m0, m4, m8, m12;
    float m1, m5, m9, m13;
    float m2, m6, m10, m14;
    float m3, m7, m11, m15;
} Matrix;

typedef struct Color {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} Color;

typedef struct Rectangle {
    float x;
    float y;
    float width;
    float height;
} Rectangle;

typedef struct Image {
    void *data;
    int width;
    int height;
    int mipmaps;
    int format;
} Image;

typedef struct Texture {
    unsigned int id;
    int width;
    int height;
    int mipmaps;
    int format;
} Texture;

typedef Texture Texture2D;

typedef Texture TextureCubemap;

typedef struct RenderTexture {
    unsigned int id;
    Texture texture;
    Texture depth;
} RenderTexture;

typedef RenderTexture RenderTexture2D;

typedef struct NPatchInfo {
    Rectangle source;
    int left;
    int top;
    int right;
    int bottom;
    int layout;
} NPatchInfo;

typedef struct GlyphInfo {
    int value;
    int offsetX;
    int offsetY;
    int advanceX;
    Image image;
} GlyphInfo;

typedef struct Font {
    int baseSize;
    int glyphCount;
    int glyphPadding;
    Texture2D texture;
    Rectangle *recs;
    GlyphInfo *glyphs;
} Font;

typedef struct Camera3D {
    Vector3 position;
    Vector3 target;
    Vector3 up;
    float fovy;
    int projection;
} Camera3D;

typedef Camera3D Camera;

typedef struct Camera2D {
    Vector2 offset;
    Vector2 target;
	float rotation;
    float zoom;
} Camera2D;

typedef struct Mesh {
    int vertexCount;
    int triangleCount;


    float *vertices;
    float *texcoords;
    float *texcoords2;
    float *normals;
    float *tangents;
    unsigned char *colors;
    unsigned short *indices;


    float *animVertices;
    float *animNormals;
    unsigned char *boneIds;
    float *boneWeights;
    Matrix *boneMatrices;
    int boneCount;


    unsigned int vaoId;
    unsigned int *vboId;
} Mesh;

typedef struct Shader {
    unsigned int id;
    int *locs;
} Shader;

typedef struct MaterialMap {
    Texture2D texture;
    Color color;
    float value;
} MaterialMap;

typedef struct Material {
    Shader shader;
    MaterialMap *maps;
    float params[4];
} Material;

typedef struct Transform {
    Vector3 translation;
    Quaternion rotation;
    Vector3 scale;
} Transform;

typedef struct BoneInfo {
    char name[32];
    int parent;
} BoneInfo;

typedef struct Model {
    Matrix transform;

    int meshCount;
    int materialCount;
    Mesh *meshes;
    Material *materials;
    int *meshMaterial;


    int boneCount;
    BoneInfo *bones;
    Transform *bindPose;
} Model;

typedef struct ModelAnimation {
    int boneCount;
    int frameCount;
    BoneInfo *bones;
    Transform **framePoses;
    char name[32];
} ModelAnimation;

typedef struct Ray {
    Vector3 position;
    Vector3 direction;
} Ray;

typedef struct RayCollision {
    bool hit;
    float distance;
    Vector3 point;
    Vector3 normal;
} RayCollision;

typedef struct BoundingBox {
    Vector3 min;
    Vector3 max;
} BoundingBox;

typedef struct Wave {
    unsigned int frameCount;
    unsigned int sampleRate;
    unsigned int sampleSize;
    unsigned int channels;
    void *data;
} Wave;

typedef struct rAudioBuffer rAudioBuffer;
typedef struct rAudioProcessor rAudioProcessor;

typedef struct AudioStream {
    rAudioBuffer *buffer;
    rAudioProcessor *processor;

    unsigned int sampleRate;
    unsigned int sampleSize;
    unsigned int channels;
} AudioStream;

typedef struct Sound {
    AudioStream stream;
    unsigned int frameCount;
} Sound;

typedef struct Music {
    AudioStream stream;
    unsigned int frameCount;
    bool looping;

    int ctxType;
    void *ctxData;
} Music;

typedef struct VrDeviceInfo {
    int hResolution;
    int vResolution;
    float hScreenSize;
    float vScreenSize;
    float eyeToScreenDistance;
    float lensSeparationDistance;
    float interpupillaryDistance;
    float lensDistortionValues[4];
    float chromaAbCorrection[4];
} VrDeviceInfo;

typedef struct VrStereoConfig {
    Matrix projection[2];
    Matrix viewOffset[2];
    float leftLensCenter[2];
    float rightLensCenter[2];
    float leftScreenCenter[2];
    float rightScreenCenter[2];
    float scale[2];
    float scaleIn[2];
} VrStereoConfig;

typedef struct FilePathList {
    unsigned int capacity;
    unsigned int count;
    char **paths;
} FilePathList;

typedef struct AutomationEvent {
    unsigned int frame;
    unsigned int type;
    int params[4];
} AutomationEvent;

typedef struct AutomationEventList {
    unsigned int capacity;
    unsigned int count;
    AutomationEvent *events;
} AutomationEventList;

typedef enum {
    FLAG_VSYNC_HINT         = 0x00000040,
    FLAG_FULLSCREEN_MODE    = 0x00000002,
    FLAG_WINDOW_RESIZABLE   = 0x00000004,
    FLAG_WINDOW_UNDECORATED = 0x00000008,
    FLAG_WINDOW_HIDDEN      = 0x00000080,
    FLAG_WINDOW_MINIMIZED   = 0x00000200,
    FLAG_WINDOW_MAXIMIZED   = 0x00000400,
    FLAG_WINDOW_UNFOCUSED   = 0x00000800,
    FLAG_WINDOW_TOPMOST     = 0x00001000,
    FLAG_WINDOW_ALWAYS_RUN  = 0x00000100,
    FLAG_WINDOW_TRANSPARENT = 0x00000010,
    FLAG_WINDOW_HIGHDPI     = 0x00002000,
    FLAG_WINDOW_MOUSE_PASSTHROUGH = 0x00004000,
    FLAG_BORDERLESS_WINDOWED_MODE = 0x00008000,
    FLAG_MSAA_4X_HINT       = 0x00000020,
    FLAG_INTERLACED_HINT    = 0x00010000
} ConfigFlags;

typedef enum {
    LOG_ALL = 0,
    LOG_TRACE,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_FATAL,
    LOG_NONE
} TraceLogLevel;

typedef enum {
    KEY_NULL            = 0,

    KEY_APOSTROPHE      = 39,
    KEY_COMMA           = 44,
    KEY_MINUS           = 45,
    KEY_PERIOD          = 46,
    KEY_SLASH           = 47,
    KEY_ZERO            = 48,
    KEY_ONE             = 49,
    KEY_TWO             = 50,
    KEY_THREE           = 51,
    KEY_FOUR            = 52,
    KEY_FIVE            = 53,
    KEY_SIX             = 54,
    KEY_SEVEN           = 55,
    KEY_EIGHT           = 56,
    KEY_NINE            = 57,
    KEY_SEMICOLON       = 59,
    KEY_EQUAL           = 61,
    KEY_A               = 65,
    KEY_B               = 66,
    KEY_C               = 67,
    KEY_D               = 68,
    KEY_E               = 69,
    KEY_F               = 70,
    KEY_G               = 71,
    KEY_H               = 72,
    KEY_I               = 73,
    KEY_J               = 74,
    KEY_K               = 75,
    KEY_L               = 76,
    KEY_M               = 77,
    KEY_N               = 78,
    KEY_O               = 79,
    KEY_P               = 80,
    KEY_Q               = 81,
    KEY_R               = 82,
    KEY_S               = 83,
    KEY_T               = 84,
    KEY_U               = 85,
    KEY_V               = 86,
    KEY_W               = 87,
    KEY_X               = 88,
    KEY_Y               = 89,
    KEY_Z               = 90,
    KEY_LEFT_BRACKET    = 91,
    KEY_BACKSLASH       = 92,
    KEY_RIGHT_BRACKET   = 93,
    KEY_GRAVE           = 96,

    KEY_SPACE           = 32,
    KEY_ESCAPE          = 256,
    KEY_ENTER           = 257,
    KEY_TAB             = 258,
    KEY_BACKSPACE       = 259,
    KEY_INSERT          = 260,
    KEY_DELETE          = 261,
    KEY_RIGHT           = 262,
    KEY_LEFT            = 263,
    KEY_DOWN            = 264,
    KEY_UP              = 265,
    KEY_PAGE_UP         = 266,
    KEY_PAGE_DOWN       = 267,
    KEY_HOME            = 268,
    KEY_END             = 269,
    KEY_CAPS_LOCK       = 280,
    KEY_SCROLL_LOCK     = 281,
    KEY_NUM_LOCK        = 282,
    KEY_PRINT_SCREEN    = 283,
    KEY_PAUSE           = 284,
    KEY_F1              = 290,
    KEY_F2              = 291,
    KEY_F3              = 292,
    KEY_F4              = 293,
    KEY_F5              = 294,
    KEY_F6              = 295,
    KEY_F7              = 296,
    KEY_F8              = 297,
    KEY_F9              = 298,
    KEY_F10             = 299,
    KEY_F11             = 300,
    KEY_F12             = 301,
    KEY_LEFT_SHIFT      = 340,
    KEY_LEFT_CONTROL    = 341,
    KEY_LEFT_ALT        = 342,
    KEY_LEFT_SUPER      = 343,
    KEY_RIGHT_SHIFT     = 344,
    KEY_RIGHT_CONTROL   = 345,
    KEY_RIGHT_ALT       = 346,
    KEY_RIGHT_SUPER     = 347,
    KEY_KB_MENU         = 348,

    KEY_KP_0            = 320,
    KEY_KP_1            = 321,
    KEY_KP_2            = 322,
    KEY_KP_3            = 323,
    KEY_KP_4            = 324,
    KEY_KP_5            = 325,
    KEY_KP_6            = 326,
    KEY_KP_7            = 327,
    KEY_KP_8            = 328,
    KEY_KP_9            = 329,
    KEY_KP_DECIMAL      = 330,
    KEY_KP_DIVIDE       = 331,
    KEY_KP_MULTIPLY     = 332,
    KEY_KP_SUBTRACT     = 333,
    KEY_KP_ADD          = 334,
    KEY_KP_ENTER        = 335,
    KEY_KP_EQUAL        = 336,

    KEY_BACK            = 4,
    KEY_MENU            = 5,
    KEY_VOLUME_UP       = 24,
    KEY_VOLUME_DOWN     = 25
} KeyboardKey;

#define MOUSE_LEFT_BUTTON   MOUSE_BUTTON_LEFT
#define MOUSE_RIGHT_BUTTON  MOUSE_BUTTON_RIGHT
#define MOUSE_MIDDLE_BUTTON MOUSE_BUTTON_MIDDLE

typedef enum {
    MOUSE_BUTTON_LEFT    = 0,
    MOUSE_BUTTON_RIGHT   = 1,
    MOUSE_BUTTON_MIDDLE  = 2,
    MOUSE_BUTTON_SIDE    = 3,
    MOUSE_BUTTON_EXTRA   = 4,
    MOUSE_BUTTON_FORWARD = 5,
    MOUSE_BUTTON_BACK    = 6,
} MouseButton;

typedef enum {
    MOUSE_CURSOR_DEFAULT       = 0,
    MOUSE_CURSOR_ARROW         = 1,
    MOUSE_CURSOR_IBEAM         = 2,
    MOUSE_CURSOR_CROSSHAIR     = 3,
    MOUSE_CURSOR_POINTING_HAND = 4,
    MOUSE_CURSOR_RESIZE_EW     = 5,
    MOUSE_CURSOR_RESIZE_NS     = 6,
    MOUSE_CURSOR_RESIZE_NWSE   = 7,
    MOUSE_CURSOR_RESIZE_NESW   = 8,
    MOUSE_CURSOR_RESIZE_ALL    = 9,
    MOUSE_CURSOR_NOT_ALLOWED   = 10
} MouseCursor;

typedef enum {
    GAMEPAD_BUTTON_UNKNOWN = 0,
    GAMEPAD_BUTTON_LEFT_FACE_UP,
    GAMEPAD_BUTTON_LEFT_FACE_RIGHT,
    GAMEPAD_BUTTON_LEFT_FACE_DOWN,
    GAMEPAD_BUTTON_LEFT_FACE_LEFT,
    GAMEPAD_BUTTON_RIGHT_FACE_UP,
    GAMEPAD_BUTTON_RIGHT_FACE_RIGHT,
    GAMEPAD_BUTTON_RIGHT_FACE_DOWN,
    GAMEPAD_BUTTON_RIGHT_FACE_LEFT,
    GAMEPAD_BUTTON_LEFT_TRIGGER_1,
    GAMEPAD_BUTTON_LEFT_TRIGGER_2,
    GAMEPAD_BUTTON_RIGHT_TRIGGER_1,
    GAMEPAD_BUTTON_RIGHT_TRIGGER_2,
    GAMEPAD_BUTTON_MIDDLE_LEFT,
    GAMEPAD_BUTTON_MIDDLE,
    GAMEPAD_BUTTON_MIDDLE_RIGHT,
    GAMEPAD_BUTTON_LEFT_THUMB,
    GAMEPAD_BUTTON_RIGHT_THUMB
} GamepadButton;

typedef enum {
    GAMEPAD_AXIS_LEFT_X        = 0,
    GAMEPAD_AXIS_LEFT_Y        = 1,
    GAMEPAD_AXIS_RIGHT_X       = 2,
    GAMEPAD_AXIS_RIGHT_Y       = 3,
    GAMEPAD_AXIS_LEFT_TRIGGER  = 4,
    GAMEPAD_AXIS_RIGHT_TRIGGER = 5
} GamepadAxis;

typedef enum {
    MATERIAL_MAP_ALBEDO = 0,
    MATERIAL_MAP_METALNESS,
    MATERIAL_MAP_NORMAL,
    MATERIAL_MAP_ROUGHNESS,
    MATERIAL_MAP_OCCLUSION,
    MATERIAL_MAP_EMISSION,
    MATERIAL_MAP_HEIGHT,
    MATERIAL_MAP_CUBEMAP,
    MATERIAL_MAP_IRRADIANCE,
    MATERIAL_MAP_PREFILTER,
    MATERIAL_MAP_BRDF
} MaterialMapIndex;

#define MATERIAL_MAP_DIFFUSE      MATERIAL_MAP_ALBEDO
#define MATERIAL_MAP_SPECULAR     MATERIAL_MAP_METALNESS

typedef enum {
    SHADER_LOC_VERTEX_POSITION = 0,
    SHADER_LOC_VERTEX_TEXCOORD01,
    SHADER_LOC_VERTEX_TEXCOORD02,
    SHADER_LOC_VERTEX_NORMAL,
    SHADER_LOC_VERTEX_TANGENT,
    SHADER_LOC_VERTEX_COLOR,
    SHADER_LOC_MATRIX_MVP,
    SHADER_LOC_MATRIX_VIEW,
    SHADER_LOC_MATRIX_PROJECTION,
    SHADER_LOC_MATRIX_MODEL,
    SHADER_LOC_MATRIX_NORMAL,
    SHADER_LOC_VECTOR_VIEW,
    SHADER_LOC_COLOR_DIFFUSE,
    SHADER_LOC_COLOR_SPECULAR,
    SHADER_LOC_COLOR_AMBIENT,
    SHADER_LOC_MAP_ALBEDO,
    SHADER_LOC_MAP_METALNESS,
    SHADER_LOC_MAP_NORMAL,
    SHADER_LOC_MAP_ROUGHNESS,
    SHADER_LOC_MAP_OCCLUSION,
    SHADER_LOC_MAP_EMISSION,
    SHADER_LOC_MAP_HEIGHT,
    SHADER_LOC_MAP_CUBEMAP,
    SHADER_LOC_MAP_IRRADIANCE,
    SHADER_LOC_MAP_PREFILTER,
    SHADER_LOC_MAP_BRDF,
    SHADER_LOC_VERTEX_BONEIDS,
    SHADER_LOC_VERTEX_BONEWEIGHTS,
    SHADER_LOC_BONE_MATRICES,
    SHADER_LOC_VERTEX_INSTANCE_TX
} ShaderLocationIndex;

#define SHADER_LOC_MAP_DIFFUSE      SHADER_LOC_MAP_ALBEDO
#define SHADER_LOC_MAP_SPECULAR     SHADER_LOC_MAP_METALNESS

typedef enum {
    SHADER_UNIFORM_FLOAT = 0,
    SHADER_UNIFORM_VEC2,
    SHADER_UNIFORM_VEC3,
    SHADER_UNIFORM_VEC4,
    SHADER_UNIFORM_INT,
    SHADER_UNIFORM_IVEC2,
    SHADER_UNIFORM_IVEC3,
    SHADER_UNIFORM_IVEC4,
    SHADER_UNIFORM_UINT,
    SHADER_UNIFORM_UIVEC2,
    SHADER_UNIFORM_UIVEC3,
    SHADER_UNIFORM_UIVEC4,
    SHADER_UNIFORM_SAMPLER2D
} ShaderUniformDataType;

typedef enum {
    SHADER_ATTRIB_FLOAT = 0,
    SHADER_ATTRIB_VEC2,
    SHADER_ATTRIB_VEC3,
    SHADER_ATTRIB_VEC4
} ShaderAttributeDataType;

typedef enum {
    PIXELFORMAT_UNCOMPRESSED_GRAYSCALE = 1,
    PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA,
    PIXELFORMAT_UNCOMPRESSED_R5G6B5,
    PIXELFORMAT_UNCOMPRESSED_R8G8B8,
    PIXELFORMAT_UNCOMPRESSED_R5G5B5A1,
    PIXELFORMAT_UNCOMPRESSED_R4G4B4A4,
    PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
    PIXELFORMAT_UNCOMPRESSED_R32,
    PIXELFORMAT_UNCOMPRESSED_R32G32B32,
    PIXELFORMAT_UNCOMPRESSED_R32G32B32A32,
    PIXELFORMAT_UNCOMPRESSED_R16,
    PIXELFORMAT_UNCOMPRESSED_R16G16B16,
    PIXELFORMAT_UNCOMPRESSED_R16G16B16A16,
    PIXELFORMAT_COMPRESSED_DXT1_RGB,
    PIXELFORMAT_COMPRESSED_DXT1_RGBA,
    PIXELFORMAT_COMPRESSED_DXT3_RGBA,
    PIXELFORMAT_COMPRESSED_DXT5_RGBA,
    PIXELFORMAT_COMPRESSED_ETC1_RGB,
    PIXELFORMAT_COMPRESSED_ETC2_RGB,
    PIXELFORMAT_COMPRESSED_ETC2_EAC_RGBA,
    PIXELFORMAT_COMPRESSED_PVRT_RGB,
    PIXELFORMAT_COMPRESSED_PVRT_RGBA,
    PIXELFORMAT_COMPRESSED_ASTC_4x4_RGBA,
    PIXELFORMAT_COMPRESSED_ASTC_8x8_RGBA
} PixelFormat;

typedef enum {
    TEXTURE_FILTER_POINT = 0,
    TEXTURE_FILTER_BILINEAR,
    TEXTURE_FILTER_TRILINEAR,
    TEXTURE_FILTER_ANISOTROPIC_4X,
    TEXTURE_FILTER_ANISOTROPIC_8X,
    TEXTURE_FILTER_ANISOTROPIC_16X,
} TextureFilter;

typedef enum {
    TEXTURE_WRAP_REPEAT = 0,
    TEXTURE_WRAP_CLAMP,
    TEXTURE_WRAP_MIRROR_REPEAT,
    TEXTURE_WRAP_MIRROR_CLAMP
} TextureWrap;

typedef enum {
    CUBEMAP_LAYOUT_AUTO_DETECT = 0,
    CUBEMAP_LAYOUT_LINE_VERTICAL,
    CUBEMAP_LAYOUT_LINE_HORIZONTAL,
    CUBEMAP_LAYOUT_CROSS_THREE_BY_FOUR,
    CUBEMAP_LAYOUT_CROSS_FOUR_BY_THREE
} CubemapLayout;

typedef enum {
    FONT_DEFAULT = 0,
    FONT_BITMAP,
    FONT_SDF
} FontType;

typedef enum {
    BLEND_ALPHA = 0,
    BLEND_ADDITIVE,
    BLEND_MULTIPLIED,
    BLEND_ADD_COLORS,
    BLEND_SUBTRACT_COLORS,
    BLEND_ALPHA_PREMULTIPLY,
    BLEND_CUSTOM,
    BLEND_CUSTOM_SEPARATE
} BlendMode;

typedef enum {
    GESTURE_NONE        = 0,
    GESTURE_TAP         = 1,
    GESTURE_DOUBLETAP   = 2,
    GESTURE_HOLD        = 4,
    GESTURE_DRAG        = 8,
    GESTURE_SWIPE_RIGHT = 16,
    GESTURE_SWIPE_LEFT  = 32,
    GESTURE_SWIPE_UP    = 64,
    GESTURE_SWIPE_DOWN  = 128,
    GESTURE_PINCH_IN    = 256,
    GESTURE_PINCH_OUT   = 512
} Gesture;

typedef enum {
    CAMERA_CUSTOM = 0,
    CAMERA_FREE,
    CAMERA_ORBITAL,
    CAMERA_FIRST_PERSON,
    CAMERA_THIRD_PERSON
} CameraMode;

typedef enum {
    CAMERA_PERSPECTIVE = 0,
    CAMERA_ORTHOGRAPHIC
} CameraProjection;

typedef enum {
    NPATCH_NINE_PATCH = 0,
    NPATCH_THREE_PATCH_VERTICAL,
    NPATCH_THREE_PATCH_HORIZONTAL
} NPatchLayout;

typedef void (*TraceLogCallback)(int logLevel, const char *text, va_list args);
typedef unsigned char *(*LoadFileDataCallback)(const char *fileName, int *dataSize);
typedef bool (*SaveFileDataCallback)(const char *fileName, void *data, int dataSize);
typedef char *(*LoadFileTextCallback)(const char *fileName);
typedef bool (*SaveFileTextCallback)(const char *fileName, const char *text);

#if defined(__cplusplus)
extern "C" {
#endif

RLAPI void InitWindow(int width, int height, const char *title);
RLAPI void CloseWindow(void);
RLAPI bool WindowShouldClose(void);
RLAPI bool IsWindowReady(void);
RLAPI bool IsWindowFullscreen(void);
RLAPI bool IsWindowHidden(void);
RLAPI bool IsWindowMinimized(void);
RLAPI bool IsWindowMaximized(void);
RLAPI bool IsWindowFocused(void);
RLAPI bool IsWindowResized(void);
RLAPI bool IsWindowState(unsigned int flag);
RLAPI void SetWindowState(unsigned int flags);
RLAPI void ClearWindowState(unsigned int flags);
RLAPI void ToggleFullscreen(void);
RLAPI void ToggleBorderlessWindowed(void);
RLAPI void MaximizeWindow(void);
RLAPI void MinimizeWindow(void);
RLAPI void RestoreWindow(void);
RLAPI void SetWindowIcon(Image image);
RLAPI void SetWindowIcons(Image *images, int count);
RLAPI void SetWindowTitle(const char *title);
RLAPI void SetWindowPosition(int x, int y);
RLAPI void SetWindowMonitor(int monitor);
RLAPI void SetWindowMinSize(int width, int height);
RLAPI void SetWindowMaxSize(int width, int height);
RLAPI void SetWindowSize(int width, int height);
RLAPI void SetWindowOpacity(float opacity);
RLAPI void SetWindowFocused(void);
RLAPI void *GetWindowHandle(void);
RLAPI int GetScreenWidth(void);
RLAPI int GetScreenHeight(void);
RLAPI int GetRenderWidth(void);
RLAPI int GetRenderHeight(void);
RLAPI int GetMonitorCount(void);
RLAPI int GetCurrentMonitor(void);
RLAPI Vector2 GetMonitorPosition(int monitor);
RLAPI int GetMonitorWidth(int monitor);
RLAPI int GetMonitorHeight(int monitor);
RLAPI int GetMonitorPhysicalWidth(int monitor);
RLAPI int GetMonitorPhysicalHeight(int monitor);
RLAPI int GetMonitorRefreshRate(int monitor);
RLAPI Vector2 GetWindowPosition(void);
RLAPI Vector2 GetWindowScaleDPI(void);
RLAPI const char *GetMonitorName(int monitor);
RLAPI void SetClipboardText(const char *text);
RLAPI const char *GetClipboardText(void);
RLAPI Image GetClipboardImage(void);
RLAPI void EnableEventWaiting(void);
RLAPI void DisableEventWaiting(void);

RLAPI void ShowCursor(void);
RLAPI void HideCursor(void);
RLAPI bool IsCursorHidden(void);
RLAPI void EnableCursor(void);
RLAPI void DisableCursor(void);
RLAPI bool IsCursorOnScreen(void);

RLAPI void ClearBackground(Color color);
RLAPI void BeginDrawing(void);
RLAPI void EndDrawing(void);
RLAPI void BeginMode2D(Camera2D camera);
RLAPI void EndMode2D(void);
RLAPI void BeginMode3D(Camera3D camera);
RLAPI void EndMode3D(void);
RLAPI void BeginTextureMode(RenderTexture2D target);
RLAPI void EndTextureMode(void);
RLAPI void BeginShaderMode(Shader shader);
RLAPI void EndShaderMode(void);
RLAPI void BeginBlendMode(int mode);
RLAPI void EndBlendMode(void);
RLAPI void BeginScissorMode(int x, int y, int width, int height);
RLAPI void EndScissorMode(void);
RLAPI void BeginVrStereoMode(VrStereoConfig config);
RLAPI void EndVrStereoMode(void);

RLAPI VrStereoConfig LoadVrStereoConfig(VrDeviceInfo device);
RLAPI void UnloadVrStereoConfig(VrStereoConfig config);

RLAPI Shader LoadShader(const char *vsFileName, const char *fsFileName);
RLAPI Shader LoadShaderFromMemory(const char *vsCode, const char *fsCode);
RLAPI bool IsShaderValid(Shader shader);
RLAPI int GetShaderLocation(Shader shader, const char *uniformName);
RLAPI int GetShaderLocationAttrib(Shader shader, const char *attribName);
RLAPI void SetShaderValue(Shader shader, int locIndex, const void *value, int uniformType);
RLAPI void SetShaderValueV(Shader shader, int locIndex, const void *value, int uniformType, int count);
RLAPI void SetShaderValueMatrix(Shader shader, int locIndex, Matrix mat);
RLAPI void SetShaderValueTexture(Shader shader, int locIndex, Texture2D texture);
RLAPI void UnloadShader(Shader shader);

#define GetMouseRay GetScreenToWorldRay
RLAPI Ray GetScreenToWorldRay(Vector2 position, Camera camera);
RLAPI Ray GetScreenToWorldRayEx(Vector2 position, Camera camera, int width, int height);
RLAPI Vector2 GetWorldToScreen(Vector3 position, Camera camera);
RLAPI Vector2 GetWorldToScreenEx(Vector3 position, Camera camera, int width, int height);
RLAPI Vector2 GetWorldToScreen2D(Vector2 position, Camera2D camera);
RLAPI Vector2 GetScreenToWorld2D(Vector2 position, Camera2D camera);
RLAPI Matrix GetCameraMatrix(Camera camera);
RLAPI Matrix GetCameraMatrix2D(Camera2D camera);

RLAPI void SetTargetFPS(int fps);
RLAPI float GetFrameTime(void);
RLAPI double GetTime(void);
RLAPI int GetFPS(void);

RLAPI void SwapScreenBuffer(void);
RLAPI void PollInputEvents(void);
RLAPI void WaitTime(double seconds);

RLAPI void SetRandomSeed(unsigned int seed);
RLAPI int GetRandomValue(int min, int max);
RLAPI int *LoadRandomSequence(unsigned int count, int min, int max);
RLAPI void UnloadRandomSequence(int *sequence);

RLAPI void TakeScreenshot(const char *fileName);
RLAPI void SetConfigFlags(unsigned int flags);
RLAPI void OpenURL(const char *url);

RLAPI void TraceLog(int logLevel, const char *text, ...);
RLAPI void SetTraceLogLevel(int logLevel);
RLAPI void *MemAlloc(unsigned int size);
RLAPI void *MemRealloc(void *ptr, unsigned int size);
RLAPI void MemFree(void *ptr);

RLAPI void SetTraceLogCallback(TraceLogCallback callback);
RLAPI void SetLoadFileDataCallback(LoadFileDataCallback callback);
RLAPI void SetSaveFileDataCallback(SaveFileDataCallback callback);
RLAPI void SetLoadFileTextCallback(LoadFileTextCallback callback);
RLAPI void SetSaveFileTextCallback(SaveFileTextCallback callback);

RLAPI unsigned char *LoadFileData(const char *fileName, int *dataSize);
RLAPI void UnloadFileData(unsigned char *data);
RLAPI bool SaveFileData(const char *fileName, void *data, int dataSize);
RLAPI bool ExportDataAsCode(const unsigned char *data, int dataSize, const char *fileName);
RLAPI char *LoadFileText(const char *fileName);
RLAPI void UnloadFileText(char *text);
RLAPI bool SaveFileText(const char *fileName, const char *text);

RLAPI int FileRename(const char *fileName, const char *fileRename);
RLAPI int FileRemove(const char *fileName);
RLAPI int FileCopy(const char *srcPath, const char *dstPath);
RLAPI int FileMove(const char *srcPath, const char *dstPath);
RLAPI int FileTextReplace(const char *fileName, const char *search, const char *replacement);
RLAPI int FileTextFindIndex(const char *fileName, const char *search);
RLAPI bool FileExists(const char *fileName);
RLAPI bool DirectoryExists(const char *dirPath);
RLAPI bool IsFileExtension(const char *fileName, const char *ext);
RLAPI int GetFileLength(const char *fileName);
RLAPI long GetFileModTime(const char *fileName);
RLAPI const char *GetFileExtension(const char *fileName);
RLAPI const char *GetFileName(const char *filePath);
RLAPI const char *GetFileNameWithoutExt(const char *filePath);
RLAPI const char *GetDirectoryPath(const char *filePath);
RLAPI const char *GetPrevDirectoryPath(const char *dirPath);
RLAPI const char *GetWorkingDirectory(void);
RLAPI const char *GetApplicationDirectory(void);
RLAPI int MakeDirectory(const char *dirPath);
RLAPI bool ChangeDirectory(const char *dirPath);
RLAPI bool IsPathFile(const char *path);
RLAPI bool IsFileNameValid(const char *fileName);
RLAPI FilePathList LoadDirectoryFiles(const char *dirPath);
RLAPI FilePathList LoadDirectoryFilesEx(const char *basePath, const char *filter, bool scanSubdirs);
RLAPI void UnloadDirectoryFiles(FilePathList files);
RLAPI bool IsFileDropped(void);
RLAPI FilePathList LoadDroppedFiles(void);
RLAPI void UnloadDroppedFiles(FilePathList files);

RLAPI unsigned char *CompressData(const unsigned char *data, int dataSize, int *compDataSize);
RLAPI unsigned char *DecompressData(const unsigned char *compData, int compDataSize, int *dataSize);
RLAPI char *EncodeDataBase64(const unsigned char *data, int dataSize, int *outputSize);
RLAPI unsigned char *DecodeDataBase64(const char *text, int *outputSize);
RLAPI unsigned int ComputeCRC32(unsigned char *data, int dataSize);
RLAPI unsigned int *ComputeMD5(unsigned char *data, int dataSize);
RLAPI unsigned int *ComputeSHA1(unsigned char *data, int dataSize);
RLAPI unsigned int *ComputeSHA256(unsigned char *data, int dataSize);

RLAPI AutomationEventList LoadAutomationEventList(const char *fileName);
RLAPI void UnloadAutomationEventList(AutomationEventList list);
RLAPI bool ExportAutomationEventList(AutomationEventList list, const char *fileName);
RLAPI void SetAutomationEventList(AutomationEventList *list);
RLAPI void SetAutomationEventBaseFrame(int frame);
RLAPI void StartAutomationEventRecording(void);
RLAPI void StopAutomationEventRecording(void);
RLAPI void PlayAutomationEvent(AutomationEvent event);

RLAPI bool IsKeyPressed(int key);
RLAPI bool IsKeyPressedRepeat(int key);
RLAPI bool IsKeyDown(int key);
RLAPI bool IsKeyReleased(int key);
RLAPI bool IsKeyUp(int key);
RLAPI int GetKeyPressed(void);
RLAPI int GetCharPressed(void);
RLAPI const char *GetKeyName(int key);
RLAPI void SetExitKey(int key);

RLAPI bool IsGamepadAvailable(int gamepad);
RLAPI const char *GetGamepadName(int gamepad);
RLAPI bool IsGamepadButtonPressed(int gamepad, int button);
RLAPI bool IsGamepadButtonDown(int gamepad, int button);
RLAPI bool IsGamepadButtonReleased(int gamepad, int button);
RLAPI bool IsGamepadButtonUp(int gamepad, int button);
RLAPI int GetGamepadButtonPressed(void);
RLAPI int GetGamepadAxisCount(int gamepad);
RLAPI float GetGamepadAxisMovement(int gamepad, int axis);
RLAPI int SetGamepadMappings(const char *mappings);
RLAPI void SetGamepadVibration(int gamepad, float leftMotor, float rightMotor, float duration);

RLAPI bool IsMouseButtonPressed(int button);
RLAPI bool IsMouseButtonDown(int button);
RLAPI bool IsMouseButtonReleased(int button);
RLAPI bool IsMouseButtonUp(int button);
RLAPI int GetMouseX(void);
RLAPI int GetMouseY(void);
RLAPI Vector2 GetMousePosition(void);
RLAPI Vector2 GetMouseDelta(void);
RLAPI void SetMousePosition(int x, int y);
RLAPI void SetMouseOffset(int offsetX, int offsetY);
RLAPI void SetMouseScale(float scaleX, float scaleY);
RLAPI float GetMouseWheelMove(void);
RLAPI Vector2 GetMouseWheelMoveV(void);
RLAPI void SetMouseCursor(int cursor);

RLAPI int GetTouchX(void);
RLAPI int GetTouchY(void);
RLAPI Vector2 GetTouchPosition(int index);
RLAPI int GetTouchPointId(int index);
RLAPI int GetTouchPointCount(void);

RLAPI void SetGesturesEnabled(unsigned int flags);
RLAPI bool IsGestureDetected(unsigned int gesture);
RLAPI int GetGestureDetected(void);
RLAPI float GetGestureHoldDuration(void);
RLAPI Vector2 GetGestureDragVector(void);
RLAPI float GetGestureDragAngle(void);
RLAPI Vector2 GetGesturePinchVector(void);
RLAPI float GetGesturePinchAngle(void);

RLAPI void UpdateCamera(Camera *camera, int mode);
RLAPI void UpdateCameraPro(Camera *camera, Vector3 movement, Vector3 rotation, float zoom);

RLAPI void SetShapesTexture(Texture2D texture, Rectangle source);
RLAPI Texture2D GetShapesTexture(void);
RLAPI Rectangle GetShapesTextureRectangle(void);

RLAPI void DrawPixel(int posX, int posY, Color color);
RLAPI void DrawPixelV(Vector2 position, Color color);
RLAPI void DrawLine(int startPosX, int startPosY, int endPosX, int endPosY, Color color);
RLAPI void DrawLineV(Vector2 startPos, Vector2 endPos, Color color);
RLAPI void DrawLineEx(Vector2 startPos, Vector2 endPos, float thick, Color color);
RLAPI void DrawLineStrip(const Vector2 *points, int pointCount, Color color);
RLAPI void DrawLineBezier(Vector2 startPos, Vector2 endPos, float thick, Color color);
RLAPI void DrawLineDashed(Vector2 startPos, Vector2 endPos, int dashSize, int spaceSize, Color color);
RLAPI void DrawCircle(int centerX, int centerY, float radius, Color color);
RLAPI void DrawCircleSector(Vector2 center, float radius, float startAngle, float endAngle, int segments, Color color);
RLAPI void DrawCircleSectorLines(Vector2 center, float radius, float startAngle, float endAngle, int segments, Color color);
RLAPI void DrawCircleGradient(int centerX, int centerY, float radius, Color inner, Color outer);
RLAPI void DrawCircleV(Vector2 center, float radius, Color color);
RLAPI void DrawCircleLines(int centerX, int centerY, float radius, Color color);
RLAPI void DrawCircleLinesV(Vector2 center, float radius, Color color);
RLAPI void DrawEllipse(int centerX, int centerY, float radiusH, float radiusV, Color color);
RLAPI void DrawEllipseV(Vector2 center, float radiusH, float radiusV, Color color);
RLAPI void DrawEllipseLines(int centerX, int centerY, float radiusH, float radiusV, Color color);
RLAPI void DrawEllipseLinesV(Vector2 center, float radiusH, float radiusV, Color color);
RLAPI void DrawRing(Vector2 center, float innerRadius, float outerRadius, float startAngle, float endAngle, int segments, Color color);
RLAPI void DrawRingLines(Vector2 center, float innerRadius, float outerRadius, float startAngle, float endAngle, int segments, Color color);
RLAPI void DrawRectangle(int posX, int posY, int width, int height, Color color);
RLAPI void DrawRectangleV(Vector2 position, Vector2 size, Color color);
RLAPI void DrawRectangleRec(Rectangle rec, Color color);
RLAPI void DrawRectanglePro(Rectangle rec, Vector2 origin, float rotation, Color color);
RLAPI void DrawRectangleGradientV(int posX, int posY, int width, int height, Color top, Color bottom);
RLAPI void DrawRectangleGradientH(int posX, int posY, int width, int height, Color left, Color right);
RLAPI void DrawRectangleGradientEx(Rectangle rec, Color topLeft, Color bottomLeft, Color bottomRight, Color topRight);
RLAPI void DrawRectangleLines(int posX, int posY, int width, int height, Color color);
RLAPI void DrawRectangleLinesEx(Rectangle rec, float lineThick, Color color);
RLAPI void DrawRectangleRounded(Rectangle rec, float roundness, int segments, Color color);
RLAPI void DrawRectangleRoundedLines(Rectangle rec, float roundness, int segments, Color color);
RLAPI void DrawRectangleRoundedLinesEx(Rectangle rec, float roundness, int segments, float lineThick, Color color);
RLAPI void DrawTriangle(Vector2 v1, Vector2 v2, Vector2 v3, Color color);
RLAPI void DrawTriangleLines(Vector2 v1, Vector2 v2, Vector2 v3, Color color);
RLAPI void DrawTriangleFan(const Vector2 *points, int pointCount, Color color);
RLAPI void DrawTriangleStrip(const Vector2 *points, int pointCount, Color color);
RLAPI void DrawPoly(Vector2 center, int sides, float radius, float rotation, Color color);
RLAPI void DrawPolyLines(Vector2 center, int sides, float radius, float rotation, Color color);
RLAPI void DrawPolyLinesEx(Vector2 center, int sides, float radius, float rotation, float lineThick, Color color);

RLAPI void DrawSplineLinear(const Vector2 *points, int pointCount, float thick, Color color);
RLAPI void DrawSplineBasis(const Vector2 *points, int pointCount, float thick, Color color);
RLAPI void DrawSplineCatmullRom(const Vector2 *points, int pointCount, float thick, Color color);
RLAPI void DrawSplineBezierQuadratic(const Vector2 *points, int pointCount, float thick, Color color);
RLAPI void DrawSplineBezierCubic(const Vector2 *points, int pointCount, float thick, Color color);
RLAPI void DrawSplineSegmentLinear(Vector2 p1, Vector2 p2, float thick, Color color);
RLAPI void DrawSplineSegmentBasis(Vector2 p1, Vector2 p2, Vector2 p3, Vector2 p4, float thick, Color color);
RLAPI void DrawSplineSegmentCatmullRom(Vector2 p1, Vector2 p2, Vector2 p3, Vector2 p4, float thick, Color color);
RLAPI void DrawSplineSegmentBezierQuadratic(Vector2 p1, Vector2 c2, Vector2 p3, float thick, Color color);
RLAPI void DrawSplineSegmentBezierCubic(Vector2 p1, Vector2 c2, Vector2 c3, Vector2 p4, float thick, Color color);

RLAPI Vector2 GetSplinePointLinear(Vector2 startPos, Vector2 endPos, float t);
RLAPI Vector2 GetSplinePointBasis(Vector2 p1, Vector2 p2, Vector2 p3, Vector2 p4, float t);
RLAPI Vector2 GetSplinePointCatmullRom(Vector2 p1, Vector2 p2, Vector2 p3, Vector2 p4, float t);
RLAPI Vector2 GetSplinePointBezierQuad(Vector2 p1, Vector2 c2, Vector2 p3, float t);
RLAPI Vector2 GetSplinePointBezierCubic(Vector2 p1, Vector2 c2, Vector2 c3, Vector2 p4, float t);

RLAPI bool CheckCollisionRecs(Rectangle rec1, Rectangle rec2);
RLAPI bool CheckCollisionCircles(Vector2 center1, float radius1, Vector2 center2, float radius2);
RLAPI bool CheckCollisionCircleRec(Vector2 center, float radius, Rectangle rec);
RLAPI bool CheckCollisionCircleLine(Vector2 center, float radius, Vector2 p1, Vector2 p2);
RLAPI bool CheckCollisionPointRec(Vector2 point, Rectangle rec);
RLAPI bool CheckCollisionPointCircle(Vector2 point, Vector2 center, float radius);
RLAPI bool CheckCollisionPointTriangle(Vector2 point, Vector2 p1, Vector2 p2, Vector2 p3);
RLAPI bool CheckCollisionPointLine(Vector2 point, Vector2 p1, Vector2 p2, int threshold);
RLAPI bool CheckCollisionPointPoly(Vector2 point, const Vector2 *points, int pointCount);
RLAPI bool CheckCollisionLines(Vector2 startPos1, Vector2 endPos1, Vector2 startPos2, Vector2 endPos2, Vector2 *collisionPoint);
RLAPI Rectangle GetCollisionRec(Rectangle rec1, Rectangle rec2);

RLAPI Image LoadImage(const char *fileName);
RLAPI Image LoadImageRaw(const char *fileName, int width, int height, int format, int headerSize);
RLAPI Image LoadImageAnim(const char *fileName, int *frames);
RLAPI Image LoadImageAnimFromMemory(const char *fileType, const unsigned char *fileData, int dataSize, int *frames);
RLAPI Image LoadImageFromMemory(const char *fileType, const unsigned char *fileData, int dataSize);
RLAPI Image LoadImageFromTexture(Texture2D texture);
RLAPI Image LoadImageFromScreen(void);
RLAPI bool IsImageValid(Image image);
RLAPI void UnloadImage(Image image);
RLAPI bool ExportImage(Image image, const char *fileName);
RLAPI unsigned char *ExportImageToMemory(Image image, const char *fileType, int *fileSize);
RLAPI bool ExportImageAsCode(Image image, const char *fileName);

RLAPI Image GenImageColor(int width, int height, Color color);
RLAPI Image GenImageGradientLinear(int width, int height, int direction, Color start, Color end);
RLAPI Image GenImageGradientRadial(int width, int height, float density, Color inner, Color outer);
RLAPI Image GenImageGradientSquare(int width, int height, float density, Color inner, Color outer);
RLAPI Image GenImageChecked(int width, int height, int checksX, int checksY, Color col1, Color col2);
RLAPI Image GenImageWhiteNoise(int width, int height, float factor);
RLAPI Image GenImagePerlinNoise(int width, int height, int offsetX, int offsetY, float scale);
RLAPI Image GenImageCellular(int width, int height, int tileSize);
RLAPI Image GenImageText(int width, int height, const char *text);

RLAPI Image ImageCopy(Image image);
RLAPI Image ImageFromImage(Image image, Rectangle rec);
RLAPI Image ImageFromChannel(Image image, int selectedChannel);
RLAPI Image ImageText(const char *text, int fontSize, Color color);
RLAPI Image ImageTextEx(Font font, const char *text, float fontSize, float spacing, Color tint);
RLAPI void ImageFormat(Image *image, int newFormat);
RLAPI void ImageToPOT(Image *image, Color fill);
RLAPI void ImageCrop(Image *image, Rectangle crop);
RLAPI void ImageAlphaCrop(Image *image, float threshold);
RLAPI void ImageAlphaClear(Image *image, Color color, float threshold);
RLAPI void ImageAlphaMask(Image *image, Image alphaMask);
RLAPI void ImageAlphaPremultiply(Image *image);
RLAPI void ImageBlurGaussian(Image *image, int blurSize);
RLAPI void ImageKernelConvolution(Image *image, const float *kernel, int kernelSize);
RLAPI void ImageResize(Image *image, int newWidth, int newHeight);
RLAPI void ImageResizeNN(Image *image, int newWidth, int newHeight);
RLAPI void ImageResizeCanvas(Image *image, int newWidth, int newHeight, int offsetX, int offsetY, Color fill);
RLAPI void ImageMipmaps(Image *image);
RLAPI void ImageDither(Image *image, int rBpp, int gBpp, int bBpp, int aBpp);
RLAPI void ImageFlipVertical(Image *image);
RLAPI void ImageFlipHorizontal(Image *image);
RLAPI void ImageRotate(Image *image, int degrees);
RLAPI void ImageRotateCW(Image *image);
RLAPI void ImageRotateCCW(Image *image);
RLAPI void ImageColorTint(Image *image, Color color);
RLAPI void ImageColorInvert(Image *image);
RLAPI void ImageColorGrayscale(Image *image);
RLAPI void ImageColorContrast(Image *image, float contrast);
RLAPI void ImageColorBrightness(Image *image, int brightness);
RLAPI void ImageColorReplace(Image *image, Color color, Color replace);
RLAPI Color *LoadImageColors(Image image);
RLAPI Color *LoadImagePalette(Image image, int maxPaletteSize, int *colorCount);
RLAPI void UnloadImageColors(Color *colors);
RLAPI void UnloadImagePalette(Color *colors);
RLAPI Rectangle GetImageAlphaBorder(Image image, float threshold);
RLAPI Color GetImageColor(Image image, int x, int y);

RLAPI void ImageClearBackground(Image *dst, Color color);
RLAPI void ImageDrawPixel(Image *dst, int posX, int posY, Color color);
RLAPI void ImageDrawPixelV(Image *dst, Vector2 position, Color color);
RLAPI void ImageDrawLine(Image *dst, int startPosX, int startPosY, int endPosX, int endPosY, Color color);
RLAPI void ImageDrawLineV(Image *dst, Vector2 start, Vector2 end, Color color);
RLAPI void ImageDrawLineEx(Image *dst, Vector2 start, Vector2 end, int thick, Color color);
RLAPI void ImageDrawCircle(Image *dst, int centerX, int centerY, int radius, Color color);
RLAPI void ImageDrawCircleV(Image *dst, Vector2 center, int radius, Color color);
RLAPI void ImageDrawCircleLines(Image *dst, int centerX, int centerY, int radius, Color color);
RLAPI void ImageDrawCircleLinesV(Image *dst, Vector2 center, int radius, Color color);
RLAPI void ImageDrawRectangle(Image *dst, int posX, int posY, int width, int height, Color color);
RLAPI void ImageDrawRectangleV(Image *dst, Vector2 position, Vector2 size, Color color);
RLAPI void ImageDrawRectangleRec(Image *dst, Rectangle rec, Color color);
RLAPI void ImageDrawRectangleLines(Image *dst, Rectangle rec, int thick, Color color);
RLAPI void ImageDrawTriangle(Image *dst, Vector2 v1, Vector2 v2, Vector2 v3, Color color);
RLAPI void ImageDrawTriangleEx(Image *dst, Vector2 v1, Vector2 v2, Vector2 v3, Color c1, Color c2, Color c3);
RLAPI void ImageDrawTriangleLines(Image *dst, Vector2 v1, Vector2 v2, Vector2 v3, Color color);
RLAPI void ImageDrawTriangleFan(Image *dst, const Vector2 *points, int pointCount, Color color);
RLAPI void ImageDrawTriangleStrip(Image *dst, const Vector2 *points, int pointCount, Color color);
RLAPI void ImageDraw(Image *dst, Image src, Rectangle srcRec, Rectangle dstRec, Color tint);
RLAPI void ImageDrawText(Image *dst, const char *text, int posX, int posY, int fontSize, Color color);
RLAPI void ImageDrawTextEx(Image *dst, Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint);

RLAPI Texture2D LoadTexture(const char *fileName);
RLAPI Texture2D LoadTextureFromImage(Image image);
RLAPI TextureCubemap LoadTextureCubemap(Image image, int layout);
RLAPI RenderTexture2D LoadRenderTexture(int width, int height);
RLAPI bool IsTextureValid(Texture2D texture);
RLAPI void UnloadTexture(Texture2D texture);
RLAPI bool IsRenderTextureValid(RenderTexture2D target);
RLAPI void UnloadRenderTexture(RenderTexture2D target);
RLAPI void UpdateTexture(Texture2D texture, const void *pixels);
RLAPI void UpdateTextureRec(Texture2D texture, Rectangle rec, const void *pixels);

RLAPI void GenTextureMipmaps(Texture2D *texture);
RLAPI void SetTextureFilter(Texture2D texture, int filter);
RLAPI void SetTextureWrap(Texture2D texture, int wrap);

RLAPI void DrawTexture(Texture2D texture, int posX, int posY, Color tint);
RLAPI void DrawTextureV(Texture2D texture, Vector2 position, Color tint);
RLAPI void DrawTextureEx(Texture2D texture, Vector2 position, float rotation, float scale, Color tint);
RLAPI void DrawTextureRec(Texture2D texture, Rectangle source, Vector2 position, Color tint);
RLAPI void DrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint);
RLAPI void DrawTextureNPatch(Texture2D texture, NPatchInfo nPatchInfo, Rectangle dest, Vector2 origin, float rotation, Color tint);

RLAPI bool ColorIsEqual(Color col1, Color col2);
RLAPI Color Fade(Color color, float alpha);
RLAPI int ColorToInt(Color color);
RLAPI Vector4 ColorNormalize(Color color);
RLAPI Color ColorFromNormalized(Vector4 normalized);
RLAPI Vector3 ColorToHSV(Color color);
RLAPI Color ColorFromHSV(float hue, float saturation, float value);
RLAPI Color ColorTint(Color color, Color tint);
RLAPI Color ColorBrightness(Color color, float factor);
RLAPI Color ColorContrast(Color color, float contrast);
RLAPI Color ColorAlpha(Color color, float alpha);
RLAPI Color ColorAlphaBlend(Color dst, Color src, Color tint);
RLAPI Color ColorLerp(Color color1, Color color2, float factor);
RLAPI Color GetColor(unsigned int hexValue);
RLAPI Color GetPixelColor(void *srcPtr, int format);
RLAPI void SetPixelColor(void *dstPtr, Color color, int format);
RLAPI int GetPixelDataSize(int width, int height, int format);

RLAPI Font GetFontDefault(void);
RLAPI Font LoadFont(const char *fileName);
RLAPI Font LoadFontEx(const char *fileName, int fontSize, const int *codepoints, int codepointCount);
RLAPI Font LoadFontFromImage(Image image, Color key, int firstChar);
RLAPI Font LoadFontFromMemory(const char *fileType, const unsigned char *fileData, int dataSize, int fontSize, const int *codepoints, int codepointCount);
RLAPI bool IsFontValid(Font font);
RLAPI GlyphInfo *LoadFontData(const unsigned char *fileData, int dataSize, int fontSize, const int *codepoints, int codepointCount, int type, int *glyphCount);
RLAPI Image GenImageFontAtlas(const GlyphInfo *glyphs, Rectangle **glyphRecs, int glyphCount, int fontSize, int padding, int packMethod);
RLAPI void UnloadFontData(GlyphInfo *glyphs, int glyphCount);
RLAPI void UnloadFont(Font font);
RLAPI bool ExportFontAsCode(Font font, const char *fileName);

RLAPI void DrawFPS(int posX, int posY);
RLAPI void DrawText(const char *text, int posX, int posY, int fontSize, Color color);
RLAPI void DrawTextEx(Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint);
RLAPI void DrawTextPro(Font font, const char *text, Vector2 position, Vector2 origin, float rotation, float fontSize, float spacing, Color tint);
RLAPI void DrawTextCodepoint(Font font, int codepoint, Vector2 position, float fontSize, Color tint);
RLAPI void DrawTextCodepoints(Font font, const int *codepoints, int codepointCount, Vector2 position, float fontSize, float spacing, Color tint);

RLAPI void SetTextLineSpacing(int spacing);
RLAPI int MeasureText(const char *text, int fontSize);
RLAPI Vector2 MeasureTextEx(Font font, const char *text, float fontSize, float spacing);
RLAPI int GetGlyphIndex(Font font, int codepoint);
RLAPI GlyphInfo GetGlyphInfo(Font font, int codepoint);
RLAPI Rectangle GetGlyphAtlasRec(Font font, int codepoint);

RLAPI char *LoadUTF8(const int *codepoints, int length);
RLAPI void UnloadUTF8(char *text);
RLAPI int *LoadCodepoints(const char *text, int *count);
RLAPI void UnloadCodepoints(int *codepoints);
RLAPI int GetCodepointCount(const char *text);
RLAPI int GetCodepoint(const char *text, int *codepointSize);
RLAPI int GetCodepointNext(const char *text, int *codepointSize);
RLAPI int GetCodepointPrevious(const char *text, int *codepointSize);
RLAPI const char *CodepointToUTF8(int codepoint, int *utf8Size);

RLAPI char **LoadTextLines(const char *text, int *count);
RLAPI void UnloadTextLines(char **text, int lineCount);
RLAPI int TextCopy(char *dst, const char *src);
RLAPI bool TextIsEqual(const char *text1, const char *text2);
RLAPI unsigned int TextLength(const char *text);
RLAPI const char *TextFormat(const char *text, ...);
RLAPI const char *TextSubtext(const char *text, int position, int length);
RLAPI const char *TextRemoveSpaces(const char *text);
RLAPI char *GetTextBetween(const char *text, const char *begin, const char *end);
RLAPI char *TextReplace(const char *text, const char *search, const char *replacement);
RLAPI char *TextReplaceBetween(const char *text, const char *begin, const char *end, const char *replacement);
RLAPI char *TextInsert(const char *text, const char *insert, int position);
RLAPI char *TextJoin(char **textList, int count, const char *delimiter);
RLAPI char **TextSplit(const char *text, char delimiter, int *count);
RLAPI void TextAppend(char *text, const char *append, int *position);
RLAPI int TextFindIndex(const char *text, const char *search);
RLAPI char *TextToUpper(const char *text);
RLAPI char *TextToLower(const char *text);
RLAPI char *TextToPascal(const char *text);
RLAPI char *TextToSnake(const char *text);
RLAPI char *TextToCamel(const char *text);
RLAPI int TextToInteger(const char *text);
RLAPI float TextToFloat(const char *text);

RLAPI void DrawLine3D(Vector3 startPos, Vector3 endPos, Color color);
RLAPI void DrawPoint3D(Vector3 position, Color color);
RLAPI void DrawCircle3D(Vector3 center, float radius, Vector3 rotationAxis, float rotationAngle, Color color);
RLAPI void DrawTriangle3D(Vector3 v1, Vector3 v2, Vector3 v3, Color color);
RLAPI void DrawTriangleStrip3D(const Vector3 *points, int pointCount, Color color);
RLAPI void DrawCube(Vector3 position, float width, float height, float length, Color color);
RLAPI void DrawCubeV(Vector3 position, Vector3 size, Color color);
RLAPI void DrawCubeWires(Vector3 position, float width, float height, float length, Color color);
RLAPI void DrawCubeWiresV(Vector3 position, Vector3 size, Color color);
RLAPI void DrawSphere(Vector3 centerPos, float radius, Color color);
RLAPI void DrawSphereEx(Vector3 centerPos, float radius, int rings, int slices, Color color);
RLAPI void DrawSphereWires(Vector3 centerPos, float radius, int rings, int slices, Color color);
RLAPI void DrawCylinder(Vector3 position, float radiusTop, float radiusBottom, float height, int slices, Color color);
RLAPI void DrawCylinderEx(Vector3 startPos, Vector3 endPos, float startRadius, float endRadius, int sides, Color color);
RLAPI void DrawCylinderWires(Vector3 position, float radiusTop, float radiusBottom, float height, int slices, Color color);
RLAPI void DrawCylinderWiresEx(Vector3 startPos, Vector3 endPos, float startRadius, float endRadius, int sides, Color color);
RLAPI void DrawCapsule(Vector3 startPos, Vector3 endPos, float radius, int slices, int rings, Color color);
RLAPI void DrawCapsuleWires(Vector3 startPos, Vector3 endPos, float radius, int slices, int rings, Color color);
RLAPI void DrawPlane(Vector3 centerPos, Vector2 size, Color color);
RLAPI void DrawRay(Ray ray, Color color);
RLAPI void DrawGrid(int slices, float spacing);

RLAPI Model LoadModel(const char *fileName);
RLAPI Model LoadModelFromMesh(Mesh mesh);
RLAPI bool IsModelValid(Model model);
RLAPI void UnloadModel(Model model);
RLAPI BoundingBox GetModelBoundingBox(Model model);

RLAPI void DrawModel(Model model, Vector3 position, float scale, Color tint);
RLAPI void DrawModelEx(Model model, Vector3 position, Vector3 rotationAxis, float rotationAngle, Vector3 scale, Color tint);
RLAPI void DrawModelWires(Model model, Vector3 position, float scale, Color tint);
RLAPI void DrawModelWiresEx(Model model, Vector3 position, Vector3 rotationAxis, float rotationAngle, Vector3 scale, Color tint);
RLAPI void DrawModelPoints(Model model, Vector3 position, float scale, Color tint);
RLAPI void DrawModelPointsEx(Model model, Vector3 position, Vector3 rotationAxis, float rotationAngle, Vector3 scale, Color tint);
RLAPI void DrawBoundingBox(BoundingBox box, Color color);
RLAPI void DrawBillboard(Camera camera, Texture2D texture, Vector3 position, float scale, Color tint);
RLAPI void DrawBillboardRec(Camera camera, Texture2D texture, Rectangle source, Vector3 position, Vector2 size, Color tint);
RLAPI void DrawBillboardPro(Camera camera, Texture2D texture, Rectangle source, Vector3 position, Vector3 up, Vector2 size, Vector2 origin, float rotation, Color tint);

RLAPI void UploadMesh(Mesh *mesh, bool dynamic);
RLAPI void UpdateMeshBuffer(Mesh mesh, int index, const void *data, int dataSize, int offset);
RLAPI void UnloadMesh(Mesh mesh);
RLAPI void DrawMesh(Mesh mesh, Material material, Matrix transform);
RLAPI void DrawMeshInstanced(Mesh mesh, Material material, const Matrix *transforms, int instances);
RLAPI BoundingBox GetMeshBoundingBox(Mesh mesh);
RLAPI void GenMeshTangents(Mesh *mesh);
RLAPI bool ExportMesh(Mesh mesh, const char *fileName);
RLAPI bool ExportMeshAsCode(Mesh mesh, const char *fileName);

RLAPI Mesh GenMeshPoly(int sides, float radius);
RLAPI Mesh GenMeshPlane(float width, float length, int resX, int resZ);
RLAPI Mesh GenMeshCube(float width, float height, float length);
RLAPI Mesh GenMeshSphere(float radius, int rings, int slices);
RLAPI Mesh GenMeshHemiSphere(float radius, int rings, int slices);
RLAPI Mesh GenMeshCylinder(float radius, float height, int slices);
RLAPI Mesh GenMeshCone(float radius, float height, int slices);
RLAPI Mesh GenMeshTorus(float radius, float size, int radSeg, int sides);
RLAPI Mesh GenMeshKnot(float radius, float size, int radSeg, int sides);
RLAPI Mesh GenMeshHeightmap(Image heightmap, Vector3 size);
RLAPI Mesh GenMeshCubicmap(Image cubicmap, Vector3 cubeSize);

RLAPI Material *LoadMaterials(const char *fileName, int *materialCount);
RLAPI Material LoadMaterialDefault(void);
RLAPI bool IsMaterialValid(Material material);
RLAPI void UnloadMaterial(Material material);
RLAPI void SetMaterialTexture(Material *material, int mapType, Texture2D texture);
RLAPI void SetModelMeshMaterial(Model *model, int meshId, int materialId);

RLAPI ModelAnimation *LoadModelAnimations(const char *fileName, int *animCount);
RLAPI void UpdateModelAnimation(Model model, ModelAnimation anim, int frame);
RLAPI void UpdateModelAnimationBones(Model model, ModelAnimation anim, int frame);
RLAPI void UnloadModelAnimation(ModelAnimation anim);
RLAPI void UnloadModelAnimations(ModelAnimation *animations, int animCount);
RLAPI bool IsModelAnimationValid(Model model, ModelAnimation anim);

RLAPI bool CheckCollisionSpheres(Vector3 center1, float radius1, Vector3 center2, float radius2);
RLAPI bool CheckCollisionBoxes(BoundingBox box1, BoundingBox box2);
RLAPI bool CheckCollisionBoxSphere(BoundingBox box, Vector3 center, float radius);
RLAPI RayCollision GetRayCollisionSphere(Ray ray, Vector3 center, float radius);
RLAPI RayCollision GetRayCollisionBox(Ray ray, BoundingBox box);
RLAPI RayCollision GetRayCollisionMesh(Ray ray, Mesh mesh, Matrix transform);
RLAPI RayCollision GetRayCollisionTriangle(Ray ray, Vector3 p1, Vector3 p2, Vector3 p3);
RLAPI RayCollision GetRayCollisionQuad(Ray ray, Vector3 p1, Vector3 p2, Vector3 p3, Vector3 p4);

typedef void (*AudioCallback)(void *bufferData, unsigned int frames);

RLAPI void InitAudioDevice(void);
RLAPI void CloseAudioDevice(void);
RLAPI bool IsAudioDeviceReady(void);
RLAPI void SetMasterVolume(float volume);
RLAPI float GetMasterVolume(void);

RLAPI Wave LoadWave(const char *fileName);
RLAPI Wave LoadWaveFromMemory(const char *fileType, const unsigned char *fileData, int dataSize);
RLAPI bool IsWaveValid(Wave wave);
RLAPI Sound LoadSound(const char *fileName);
RLAPI Sound LoadSoundFromWave(Wave wave);
RLAPI Sound LoadSoundAlias(Sound source);
RLAPI bool IsSoundValid(Sound sound);
RLAPI void UpdateSound(Sound sound, const void *data, int sampleCount);
RLAPI void UnloadWave(Wave wave);
RLAPI void UnloadSound(Sound sound);
RLAPI void UnloadSoundAlias(Sound alias);
RLAPI bool ExportWave(Wave wave, const char *fileName);
RLAPI bool ExportWaveAsCode(Wave wave, const char *fileName);

RLAPI void PlaySound(Sound sound);
RLAPI void StopSound(Sound sound);
RLAPI void PauseSound(Sound sound);
RLAPI void ResumeSound(Sound sound);
RLAPI bool IsSoundPlaying(Sound sound);
RLAPI void SetSoundVolume(Sound sound, float volume);
RLAPI void SetSoundPitch(Sound sound, float pitch);
RLAPI void SetSoundPan(Sound sound, float pan);
RLAPI Wave WaveCopy(Wave wave);
RLAPI void WaveCrop(Wave *wave, int initFrame, int finalFrame);
RLAPI void WaveFormat(Wave *wave, int sampleRate, int sampleSize, int channels);
RLAPI float *LoadWaveSamples(Wave wave);
RLAPI void UnloadWaveSamples(float *samples);

RLAPI Music LoadMusicStream(const char *fileName);
RLAPI Music LoadMusicStreamFromMemory(const char *fileType, const unsigned char *data, int dataSize);
RLAPI bool IsMusicValid(Music music);
RLAPI void UnloadMusicStream(Music music);
RLAPI void PlayMusicStream(Music music);
RLAPI bool IsMusicStreamPlaying(Music music);
RLAPI void UpdateMusicStream(Music music);
RLAPI void StopMusicStream(Music music);
RLAPI void PauseMusicStream(Music music);
RLAPI void ResumeMusicStream(Music music);
RLAPI void SeekMusicStream(Music music, float position);
RLAPI void SetMusicVolume(Music music, float volume);
RLAPI void SetMusicPitch(Music music, float pitch);
RLAPI void SetMusicPan(Music music, float pan);
RLAPI float GetMusicTimeLength(Music music);
RLAPI float GetMusicTimePlayed(Music music);

RLAPI AudioStream LoadAudioStream(unsigned int sampleRate, unsigned int sampleSize, unsigned int channels);
RLAPI bool IsAudioStreamValid(AudioStream stream);
RLAPI void UnloadAudioStream(AudioStream stream);
RLAPI void UpdateAudioStream(AudioStream stream, const void *data, int frameCount);
RLAPI bool IsAudioStreamProcessed(AudioStream stream);
RLAPI void PlayAudioStream(AudioStream stream);
RLAPI void PauseAudioStream(AudioStream stream);
RLAPI void ResumeAudioStream(AudioStream stream);
RLAPI bool IsAudioStreamPlaying(AudioStream stream);
RLAPI void StopAudioStream(AudioStream stream);
RLAPI void SetAudioStreamVolume(AudioStream stream, float volume);
RLAPI void SetAudioStreamPitch(AudioStream stream, float pitch);
RLAPI void SetAudioStreamPan(AudioStream stream, float pan);
RLAPI void SetAudioStreamBufferSizeDefault(int size);
RLAPI void SetAudioStreamCallback(AudioStream stream, AudioCallback callback);

RLAPI void AttachAudioStreamProcessor(AudioStream stream, AudioCallback processor);
RLAPI void DetachAudioStreamProcessor(AudioStream stream, AudioCallback processor);

RLAPI void AttachAudioMixedProcessor(AudioCallback processor);
RLAPI void DetachAudioMixedProcessor(AudioCallback processor);

#if defined(__cplusplus)
}
#endif

#endif
