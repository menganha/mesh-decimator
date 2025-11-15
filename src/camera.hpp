#pragma once

#include <GLFW/glfw3.h>
#include <glm/gtc/quaternion.hpp>
#include <glm/glm.hpp>

struct Camera
{
    float     fov;
    float     zNear;
    float     zFar;
    glm::vec3 position;
    glm::vec3 target;
    glm::vec3 up;
};

struct TrackBall
{
    glm::vec2 last_cursor_pos; // Click point on the previous frame
    glm::quat current_rotation;
    float     speed;
    bool      is_active;
};

struct CameraControl
{
    Camera    camera;
    TrackBall trackball;
};

void CursorPosCallback(CameraControl* camera_control, double cursor_pos_x, double cursor_pos_y, int win_size_x, int win_size_y);

