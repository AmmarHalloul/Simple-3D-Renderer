#pragma once
#include "glm/glm.hpp"
#include "glad/glad.h"
#define GLFW_DLL
#include "GLFW/glfw3.h"
#include "common.h" 

enum DrawMode : i32
{
    color_mode = 0,
    textured_mode,
    unlit_color_mode
};


inline const glm::vec3 vec3_up    = {0.f, 1.f,  0.f};
inline const glm::vec3 vec3_right = {1.f, 0.f,  0.f};
inline const glm::vec3 vec3_front = {0.f, 0.f, -1.f};

inline const glm::vec3 white_color = {1.f, 1.f, 1.f};
inline const glm::vec3 blue_color  = {0.f, 0.f, 1.f};
inline const glm::vec3 green_color = {0.f, 1.f, 0.f};
inline const glm::vec3 red_color   = {1.f, 0.f, 0.f};
inline const glm::vec3 black_color = {0.f, 0.f, 0.f};


struct Mesh
{
    u32 vertices;
    u32 normals; //per vertex
    u32 texture_coordinates;
    u32 vertex_count;
    u32 indices;
    u32 index_count;
    u32 texture;
};

struct Renderer
{
    struct Shapes
    {
        Mesh cube;
        Mesh sphere;
    };

    GLFWwindow* window;
    glm::vec2 window_extent;

    glm::mat4 projection_matrix;
    glm::mat4 view_matrix;

    Shapes shapes;

    u32 shader;

    glm::vec3 light_source;
};


Mesh LoadHelmetMesh();

Renderer InitRenderer();
void Terminate(Renderer* ren);

void BeginDrawing(Renderer* ren);
void EndDrawing(Renderer* ren);
bool WindowShouldClose(Renderer* ren);

void DrawMesh(Renderer* ren, Mesh mesh, glm::vec3 position, f32 scale, f32 angle, glm::vec3 axis, DrawMode draw_mode = textured_mode, glm::vec3 color = {});
void DrawCube(Renderer* ren, glm::vec3 color, glm::vec3 position, f32 scale, f32 angle, glm::vec3 axis);
void DrawSphere(Renderer* ren, glm::vec3 color, glm::vec3 position, f32 scale);
void DrawSphereUnlit(Renderer* ren, glm::vec3 color, glm::vec3 position, f32 scale);


