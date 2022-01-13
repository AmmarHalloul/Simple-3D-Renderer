#include "renderer.h"
#include "windows.h"
#include "stdarg.h"
#include "glad/glad.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "common.h" 

#include "lodepng.h"
#define GLFW_DLL
#include "GLFW/glfw3.h"
#define PAR_SHAPES_IMPLEMENTATION
#include "par_shapes.h"
#define CGLTF_IMPLEMENTATION
#include "cgltf.h"


static const char vertex_shader_code[] = R"(

    #version 460 core

    layout (location = 0) in vec3 vertex ;
    layout (location = 1) in vec3 normal ;
    layout (location = 2) in vec2 texture_coordinates;

    layout (location = 0) out float diffuse_out;
    layout (location = 1) out vec2 texture_coordinates_out;

    layout (location = 0) uniform mat4 model_matrix ;
    layout (location = 1) uniform mat4 normal_matrix;
    layout (location = 2) uniform mat4 projection_matrix ;
    layout (location = 3) uniform vec3 light_source;


    void main() 
    {
        vec4 world_vertex_vec4 = model_matrix * vec4(vertex, 1.0);
        gl_Position = projection_matrix * world_vertex_vec4;

        vec3 world_vertex = world_vertex_vec4.xyz / world_vertex_vec4.w;
        vec3 new_normal = normalize(vec3(normal_matrix * vec4(normal, 0.0)));
        vec3 light_direction = normalize(light_source - world_vertex);

        diffuse_out = max(dot(new_normal, light_direction), 0.0);

        texture_coordinates_out = texture_coordinates;
    }
)" ;


static const char fragment_shader_code[] = R"(

    #version 460 core

    layout (location = 0) in float diffuse; 
    layout (location = 1) in vec2 texture_coordinates;

    layout (location = 0) out vec4 pixel_out;

    layout (location = 10) uniform vec3 color;
    layout (location = 11) uniform int mode;

    layout (binding = 0) uniform sampler2D my_texture;

    const int color_mode = 0;
    const int textured_mode = 1;
    const int unlit_color_mode = 2;

    const float ambient = 0.2;

    void main() 
    {
        switch(mode)
        {
            case textured_mode :
                pixel_out = vec4(texture(my_texture, texture_coordinates).rgb * min(diffuse + ambient, 1.0), 1.0);
            break;

            case unlit_color_mode :
                pixel_out = vec4(color, 1.0);
            break;

            case color_mode :
                pixel_out = vec4(color * min(diffuse + ambient, 1.0), 1.0);
            break;
        }

    }
)" ;


Mesh LoadHelmetMesh()
{
    cgltf_options options = {};
    cgltf_data* data = nullptr;
    cgltf_result result = cgltf_parse_file(&options, "DamagedHelmet.gltf", &data);
    Assert(result == cgltf_result_success);
    result = cgltf_load_buffers(&options, data, "DamagedHelmet.gltf");
    Assert(result == cgltf_result_success);

    const u32 index_count = 46356;
    const u32 vertex_count = 14556;

    void* indices = data->buffers[0].data;
    void* vertices = ((byte*)data->buffers[0].data) + 92712;
    void* normals = ((byte*)data->buffers[0].data) + 267384;
    void* tex_coords = ((byte*)data->buffers[0].data) + 442056;

    byte* image = 0;
    u32 width, height;

    auto error = lodepng_decode32_file(&image, &width, &height, "Default_albedo.png");
    if(error) 
    {
        printf("png error : %s\n", lodepng_error_text(error));
        abort();
    }


    Mesh m = {};
    glCreateBuffers(1, &m.vertices);
    glCreateBuffers(1, &m.indices);
    glCreateBuffers(1, &m.normals);
    glCreateBuffers(1, &m.texture_coordinates);

    m.vertex_count = vertex_count;
    m.index_count = index_count;

    glNamedBufferStorage(m.indices, m.index_count * sizeof(u16), indices, 0);
    glNamedBufferStorage(m.vertices, m.vertex_count * sizeof(glm::vec3), vertices, 0);
    glNamedBufferStorage(m.normals, m.vertex_count * sizeof(glm::vec3), normals, 0);
    glNamedBufferStorage(m.texture_coordinates, m.vertex_count * sizeof(glm::vec2), tex_coords, 0);

    glCreateTextures(GL_TEXTURE_2D, 1, &m.texture);
    glTextureStorage2D(m.texture, 1, GL_RGBA8, width, height);
    glTextureSubImage2D(m.texture, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, image);

    cgltf_free(data);
    free(image);
    return m;
}

static Mesh ToMesh(par_shapes_mesh_s* par_mesh)
{

    glm::vec3* vertices = (glm::vec3*) par_mesh->points;
    u32 vertex_count = par_mesh->npoints; 
    glm::vec3 centroid = {};


    par_shapes_compute_normals(par_mesh);

    // center of gravity approximation 
    ForCount(i, vertex_count)
        centroid += vertices[i];

    centroid /= vertex_count;

    // center mesh
    ForCount(i, vertex_count)
        vertices[i] -= centroid;

    Mesh m = {};
    glCreateBuffers(1, &m.vertices);
    glCreateBuffers(1, &m.indices);
    glCreateBuffers(1, &m.normals);

    m.vertex_count = par_mesh->npoints;
    m.index_count = par_mesh->ntriangles * 3;

    glNamedBufferStorage(m.vertices, m.vertex_count * sizeof(glm::vec3), par_mesh->points, 0);
    glNamedBufferStorage(m.normals, m.vertex_count * sizeof(glm::vec3), par_mesh->normals, 0);
    glNamedBufferStorage(m.indices, m.index_count * sizeof(u16), par_mesh->triangles, 0);

    par_shapes_free_mesh(par_mesh);
    return m;
}


static void CompileShaderStage(u32 shader) 
{
    i32 compiled;
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE)
    {
        i32 log_length = 0;
        char message[1024];
        glGetShaderInfoLog(shader, ArraySize(message), &log_length, message);
        printf("shader compiling failed : %s\n", message);
    }
};

static u32 CompileShader(const char* vert_code, const char* frag_code)  
{
    u32 id = glCreateProgram();

    auto vert = glCreateShader(GL_VERTEX_SHADER) ;
    glShaderSource(vert, 1, &vert_code, 0);

    auto frag = glCreateShader(GL_FRAGMENT_SHADER) ;
    glShaderSource(frag, 1, &frag_code, 0);
    

    CompileShaderStage(vert);
    CompileShaderStage(frag);


    glAttachShader(id, vert);
    glAttachShader(id, frag);

    glLinkProgram(id);

    glDetachShader(id, vert);
    glDetachShader(id, frag);

    glDeleteShader(vert);
    glDeleteShader(frag);


    int linked ;
    glGetProgramiv(id, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE)
    {
        i32 log_length = 0; 
        char message[1024];
        glGetProgramInfoLog(id, ArraySize(message), &log_length, message);
        printf("shader linking failed : %s\n", message) ;
        glDeleteProgram(id) ;
        return 0;
    }

    return id;
}



static void OpenglDebugCallback(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char *message, const void *userParam)
{
    if(id == 131169 || id == 131185 || id == 131218 || id == 131204) return;
    printf("openg error : %s\n", message);
    abort();
}

static void SetDataAsWD()
{
    char cd[1000];
    *cd = 0;
    GetModuleFileNameA(GetModuleHandle(0), cd, ArraySize(cd));
    strcat_s(cd, ArraySize(cd), "\\..\\..\\data");
    auto cd_error = SetCurrentDirectory(cd);
    if (!cd_error) abort();
}

Renderer InitRenderer()
{
    Renderer r = {};
    SetDataAsWD();

    glfwInit();

    glfwWindowHint(GLFW_RESIZABLE, false);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true); 
    r.window_extent = {1280, 720};
    r.window = glfwCreateWindow(r.window_extent.x, r.window_extent.y, "3D Graphics Test", nullptr, nullptr);
    Assert(r.window);
    glfwSwapInterval(1);

    glfwMakeContextCurrent(r.window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glViewport(0, 0, r.window_extent.x, r.window_extent.y);
    glEnable(GL_DEPTH_TEST) ;
    glEnable(GL_BLEND) ;
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA) ;

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); 
    glDebugMessageCallback(OpenglDebugCallback, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, 0, GL_TRUE);
    // glClearColor(0.3f, 0.4f, 0.4f, 1.0f);

    r.projection_matrix = glm::perspective(glm::radians(45.0f), r.window_extent.x/r.window_extent.y, 0.1f, 100.0f);
    r.view_matrix = glm::mat4(1.0);
    r.view_matrix = glm::translate(r.view_matrix, {0.f, 0.f, -6.5f});
    
    r.shapes.cube = ToMesh(par_shapes_create_cube());
    // r.shapes.cone = ToMesh(par_shapes_create_cone(10, 10));
    // r.shapes.cylinder = ToMesh(par_shapes_create_cylinder(10, 10));
    r.shapes.sphere = ToMesh(par_shapes_create_subdivided_sphere(3));

    r.shader = CompileShader(vertex_shader_code, fragment_shader_code);
    r.light_source = {0.f, 2.5f, -5.f};

    return r;
}



void Terminate(Renderer* ren)
{
    glfwDestroyWindow(ren->window);
    glfwTerminate();
    *ren = {};
}

static void SetUniform(u32 shader, const char* uniform_name, glm::vec3 v)
{
    i32 loc = glGetUniformLocation(shader, uniform_name);
    Assert(loc != -1);
    glProgramUniform3fv(shader, loc, 1, &v.x);
}

static void SetUniform(u32 shader, const char* uniform_name, glm::mat4 v)
{
    i32 loc = glGetUniformLocation(shader, uniform_name);
    Assert(loc != -1);
    glProgramUniformMatrix4fv(shader, loc, 1, false, &v[0].x);
}

static void SetUniform(u32 shader, const char* uniform_name, i32 value)
{
    i32 loc = glGetUniformLocation(shader, uniform_name);
    Assert(loc != -1);
    glProgramUniform1i(shader, loc, value);
}


void DrawMesh(Renderer* ren, Mesh mesh, glm::vec3 position, f32 scale, f32 angle, glm::vec3 axis, DrawMode draw_mode, glm::vec3 color)
{   
    glm::mat4 model_matrix(1.f);
    model_matrix = glm::translate(model_matrix, position);
    if (axis != glm::vec3(0.f))
        model_matrix = glm::rotate(model_matrix, glm::radians(angle), axis);
    model_matrix = glm::scale(model_matrix, glm::vec3(scale));

    glUseProgram(ren->shader);
    SetUniform(ren->shader, "model_matrix", model_matrix);
    SetUniform(ren->shader, "normal_matrix", glm::transpose(glm::inverse(model_matrix)));
    SetUniform(ren->shader, "projection_matrix", ren->projection_matrix * ren->view_matrix);
    SetUniform(ren->shader, "mode", draw_mode);
    SetUniform(ren->shader, "light_source", ren->light_source);

    if (draw_mode != textured_mode)
    {
        SetUniform(ren->shader, "color", color);
    }


    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indices);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vertices);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.normals);
    glVertexAttribPointer(1, 3, GL_FLOAT, false, 0, 0);

    if (draw_mode == textured_mode)
    {
        glBindTextureUnit(0, mesh.texture);
        glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, mesh.texture_coordinates);
        glVertexAttribPointer(2, 2, GL_FLOAT, false, 0, 0);        
    }

    glDrawElements(GL_TRIANGLES, mesh.index_count, GL_UNSIGNED_SHORT, 0);
}


void DrawCube(Renderer* ren, glm::vec3 color, glm::vec3 position, f32 scale, f32 angle, glm::vec3 axis)
{
    DrawMesh(ren, ren->shapes.cube, position, scale, angle, axis, color_mode, color);
}

void DrawSphere(Renderer* ren, glm::vec3 color, glm::vec3 position, f32 scale)
{
    DrawMesh(ren, ren->shapes.sphere, position, scale, 0.f, glm::vec3(1.f), color_mode, color);
}

void DrawSphereUnlit(Renderer* ren, glm::vec3 color, glm::vec3 position, f32 scale)
{
    DrawMesh(ren, ren->shapes.sphere, position, scale, 0.f, glm::vec3(1.f), unlit_color_mode, color);
}

void BeginDrawing(Renderer* ren)
{
    glfwPollEvents();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    DrawSphereUnlit(ren, white_color, ren->light_source, .3f);
}

void EndDrawing(Renderer* ren)
{
    glfwSwapBuffers(ren->window);
}

bool WindowShouldClose(Renderer* ren)
{
    return glfwWindowShouldClose(ren->window);
}


