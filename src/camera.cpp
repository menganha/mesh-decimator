#include "camera.hpp"

#include <glm/glm.hpp>
#include <math.h>

static void calculate_trackball_vector(float x_pos, float y_pos, int win_width, int win_height, glm::vec3* quaternion)
{

    // TODO: Why do we invert the sign of y?
    float x = (2.f * x_pos - static_cast<float>(win_width)) / static_cast<float>(win_width);
    float y = (static_cast<float>(win_height) - 2.f * y_pos) / static_cast<float>(win_height);

    float length2 = x * x + y * y;

    float z;
    if ( length2 <= 0.5f )
        z = sqrt(1.0f - length2);
    else
        z = 0.5f / sqrt(length2);

    float norm = 1.0f / sqrt(length2 + z * z);

    quaternion->x = x * norm;
    quaternion->y = y * norm;
    quaternion->z = z * norm;
}

/*
   Based on partly the https://www.opengl.org/wiki/Object_Mouse_Trackball
   We need to basically:
    1. Find the rotation parameters, i.e., angle-axis of the movements between the mouse displacement on the screen over a
       hypothetical sphere/hypherbolic sheet
    2. Accumulate this rotations whenever we have the right mouse clicked and update the camera
*/
void CursorPosCallback(CameraControl* control, double cursor_pos_x, double cursor_pos_y, int win_width, int win_height)
{
    // camera->fov -= (float)yoffset;
    // if (camera->fov < 1.0f)
    //     camera->fov = 1.0f;
    // if (camera->fov > 45.0f)
    //     camera->fov = 45.0f;

    if ( control->trackball.is_active )
    {

        glm::vec3 start_vec = {};
        calculate_trackball_vector(control->trackball.last_cursor_pos.x, control->trackball.last_cursor_pos.y, win_width, win_height, &start_vec);

        glm::vec3 end_vector = {};
        calculate_trackball_vector(cursor_pos_x, cursor_pos_y, win_width, win_height, &end_vector);

        control->trackball.last_cursor_pos.x = cursor_pos_x;
        control->trackball.last_cursor_pos.y = cursor_pos_y;

        // Computes axis-angle rotation vector/quaternion
        float              cos_theta = glm::dot(start_vec, end_vector);
        glm::vec3          rot_axis = {};
        glm::quat          angle_axis_q = {};
        static const float EPSILON = 1.0e-5f;

        // If they are closely parallel and on opposite directions, try to derive explicitly a rotatin axis.
        if ( cos_theta < -1.0f + EPSILON )
        {
            rot_axis = glm::cross(glm::vec3(0.f, 0.f, 1.f), start_vec);
            double length2 = (double)(rot_axis.x * rot_axis.x + rot_axis.y * rot_axis.y + rot_axis.z * rot_axis.z);
            if ( length2 < 0.01 )
            {
                // TODO: I'm not sure this extra condition is needed. on a fist glance it seems we can
                //       always use the Z axis as a rotation axis for these cases. We need to debug to see when this
                //       condition is even reached.
                rot_axis = glm::cross(glm::vec3(1.f, 0.f, 0.f), start_vec);
            }
            rot_axis = glm::normalize(rot_axis);
            angle_axis_q = glm::angleAxis(180.0f, rot_axis);
        }
        // If parallel and same direction then no rotation
        else if ( cos_theta > 1.0f - EPSILON )
        {
            angle_axis_q = glm::quat(1, 0, 0, 0);
        }
        // Regular case.
        else
        {
            float theta = acos(cos_theta);
            rot_axis = glm::cross(start_vec, end_vector);
            rot_axis = glm::normalize(rot_axis);
            angle_axis_q = glm::angleAxis(theta * control->trackball.speed, rot_axis);
        }

        // Accumulate the rotation
        control->trackball.current_rotation *= glm::inverse(angle_axis_q);
        // control->trackball.current_rotation = glm::normalize(control->trackball.current_rotation * start_q * stop_q);

        // Updates position and up vector of the camera
        glm::vec3 orientation = control->trackball.current_rotation * glm::vec3(0.f, 0.f, 1.f);
        float     translate_length = glm::length(control->camera.position - control->camera.target);
        control->camera.position = translate_length * orientation + control->camera.target;
        control->camera.up = glm::normalize(control->trackball.current_rotation * glm::vec3(0.f, 1.f, 0.f));
    }
}

