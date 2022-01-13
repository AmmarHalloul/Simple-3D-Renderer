#include "renderer.h"

int main()
{
    Renderer ren = InitRenderer();
    ren.light_source = {0.f, 2.f, 1.f};

    Mesh helmet = LoadHelmetMesh();

    float rotation_angle = 0.f;

    while (!WindowShouldClose(&ren))
    {
        rotation_angle += 1.f;
        BeginDrawing(&ren);

            DrawCube(&ren, red_color, {-2, 0, 0}, 1.f, rotation_angle, vec3_up + vec3_right);
            DrawMesh(&ren, helmet, {2, 0, 0}, 1.f, rotation_angle, vec3_right + vec3_up);

        EndDrawing(&ren);
    }

    Terminate(&ren);
    return 0; 
}


