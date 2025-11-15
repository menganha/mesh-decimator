#define GLFW_INCLUDE_NONE
#include "arena.hpp"
#include "camera.hpp"
#include "log.hpp"
#include "mesh.hpp"
#include "shaders.hpp"

#include <GLFW/glfw3.h>
#include <cmath>
#include <glad/glad.h>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void cursor_pos_callback(GLFWwindow* window, double xoffset, double yoffset);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void InitDefaultCamera(Camera& camera, Mesh& mesh);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// application state
struct AppState
{
    Mesh*          mesh;
    CameraControl* control;
    Arena*         arena;
    Arena*         arena_scratch;
};

int main(int argc, char* argv[])
{

    if ( argc < 2 )
    {
        LERROR("You need to pass a file name argument to the command line");
    }

    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // glfw window creation
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Decimator", nullptr, nullptr);

    if ( not window )
    {
        LERROR("Failed to create GLFW window");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // glad: load all OpenGL function pointers
    if ( !gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) )
    {
        LERROR("Failed to initialize GLAD");
        return -1;
    }
    glEnable(GL_DEPTH_TEST);

    // Set camera and cursor
    CameraControl control = {};

    control.trackball.speed = 1.f;
    control.trackball.is_active = false;

    // Set arenas
    Arena arena = arenaMake(MEGABYTES(64));
    Arena arena_scratch = arenaMake(MEGABYTES(256));

    // Build and compile shaders
    ShaderProgram shader_program = {};
    InitShaders(&shader_program, "shaders/vertex.vert", "shaders/fragment.frag", arena);

    // Read file
    Mesh my_mesh {};
    meshInitFromObjFile(my_mesh, argv[1], arena, arena_scratch);
    meshBindBuffers(my_mesh, arena_scratch);

    // Set Camera
    InitDefaultCamera(control.camera, my_mesh);

    // Set glfw callbacks
    AppState app_state {&my_mesh, &control, &arena, &arena_scratch};
    glfwSetWindowUserPointer(window, &app_state);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    while ( !glfwWindowShouldClose(window) )
    {

        // render
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 camera_transform = glm::lookAt(control.camera.position, control.camera.target, control.camera.up);
        SetMat4(&shader_program, "modelview", camera_transform);

        glm::mat4 projection =
          glm::perspective(glm::radians(control.camera.fov), (float)SCR_WIDTH / (float)SCR_HEIGHT, control.camera.zNear, control.camera.zFar);
        SetMat4(&shader_program, "projection", projection);

        glUseProgram(shader_program.program_id);

        glBindVertexArray(my_mesh.vao);
        glDrawElements(GL_TRIANGLES, 3 * my_mesh.indices.length, GL_UNSIGNED_INT, 0);

        glBindVertexArray(0);

        glfwSwapBuffers(window); // Swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwPollEvents();
    }

    glDeleteProgram(shader_program.program_id);

    glfwTerminate();
    arenaFree(arena);
    arenaFree(arena_scratch);
    return 0;
}

// Executed whenever the window size changed (by OS or user resize).
// makes sure the viewport matches the new window dimensions; note that width and height will be significantly
// larger than specified on retina displays.
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if ( key == GLFW_KEY_R && action == GLFW_PRESS )
    {
        AppState* app_state = static_cast<AppState*>(glfwGetWindowUserPointer(window));
        InitDefaultCamera(app_state->control->camera, *app_state->mesh);
    }
    else if ( key == GLFW_KEY_D && action == GLFW_PRESS )
    {
        AppState* app_state = static_cast<AppState*>(glfwGetWindowUserPointer(window));
        meshDecimate(*app_state->mesh, *app_state->arena, *app_state->arena_scratch);
        meshBindBuffers(*app_state->mesh, *app_state->arena_scratch);
    }
}

void cursor_pos_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    double cursor_pos_x, cursor_pos_y;
    glfwGetCursorPos(window, &cursor_pos_x, &cursor_pos_y);

    AppState* app_state = static_cast<AppState*>(glfwGetWindowUserPointer(window));
    CursorPosCallback(app_state->control, cursor_pos_x, cursor_pos_y, width, height);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    // TODO: (menganha) This functionality seems more appropriate to be on camera module
    AppState* app_state = static_cast<AppState*>(glfwGetWindowUserPointer(window));
    if ( button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE )
        app_state->control->trackball.is_active = false;
    else if ( button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS )
    {
        app_state->control->trackball.is_active = true;
        double x_pos, y_pos;
        glfwGetCursorPos(window, &x_pos, &y_pos);
        app_state->control->trackball.last_cursor_pos.x = x_pos;
        app_state->control->trackball.last_cursor_pos.y = y_pos;
        // Gets the current rotation part of the transform in a quaternion
        // TODO: Maybe there's a smarter way to do this? it seems a little bit too convoluted and it doesn't seem to belong here
        glm::mat4 transform = glm::lookAt(app_state->control->camera.position, app_state->control->camera.target, app_state->control->camera.up);
        app_state->control->trackball.current_rotation = glm::inverse(glm::quat_cast(transform));
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    AppState* app_state = static_cast<AppState*>(glfwGetWindowUserPointer(window));
    app_state->control->camera.position += yoffset * 0.1;
}

void InitDefaultCamera(Camera& camera, Mesh& mesh)
{
    float max_x = 0.f, min_x = 0.f;
    float max_y = 0.f, min_y = 0.f;
    float max_z = 0.f, min_z = 0.f;
    for ( int idx = 0; idx < mesh.vertices.length; idx++ )
    {
        Vec3<float> vertex = mesh.vertices[idx];
        if ( vertex.x > max_x )
            max_x = vertex.x;
        if ( vertex.y > max_y )
            max_y = vertex.y;
        if ( vertex.z > max_z )
            max_z = vertex.z;

        if ( vertex.x < min_x )
            min_x = vertex.x;
        if ( vertex.y < min_y )
            min_y = vertex.y;
        if ( vertex.z < min_z )
            min_z = vertex.z;
    }

    camera.fov = 45.f;
    camera.zNear = 0.1f;
    camera.zFar = 8.f * min_z; // TODO: It can be improved. For now we just choose a big number

    float center_x = (max_x + min_x) / 2;
    float center_y = (max_y + min_y) / 2;
    float center_z = (max_z + min_z) / 2;

    float z_ideal_pos {};
    if ( std::abs(max_x - center_x) > std::abs(max_y - center_y) )
    {
        z_ideal_pos = std::abs(max_x - center_x) / std::tan(camera.fov * 3.1416f / 360) + max_z;
    }
    else
    {
        z_ideal_pos = std::abs(max_y - center_y) / std::tan(camera.fov * 3.1416f / 360) + max_z;
    }

    camera.position = glm::vec3(center_x, center_y, z_ideal_pos);
    camera.target = glm::vec3(center_x, center_y, center_z);
    camera.up = glm::vec3(0.0f, 1.0f, 0.0f);
}
