#pragma once

#include "arena.hpp"
#include "array.hpp"

#include <cstddef>
#include <cstring>
#include <glad/glad.h>

const int MAX_BUFFER = 12000;

template<typename T>
struct Vec3
{
    T  x, y, z;
    T& operator[](size_t idx)
    {
        switch ( idx )
        {
        default:
        case 0:
            return x;
        case 1:
            return y;
        case 2:
            return z;
        }
    }

    const T& operator[](size_t idx) const
    {
        switch ( idx )
        {
        default:
        case 0:
            return x;
        case 1:
            return y;
        case 2:
            return z;
        }
    }
};

template<typename T>
struct Vec2
{
    T x, y;

    T& operator[](size_t idx)
    {
        switch ( idx )
        {
        default:
        case 0:
            return x;
        case 1:
            return y;
        }
    }
};

struct Pair
{
    int         v1, v2; // vertex pairs references
    float       cost;
    Vec3<float> optimal;
    int&        operator[](size_t idx)
    {
        switch ( idx )
        {
        default:
        case 0:
            return v1;
        case 1:
            return v2;
        }
    }
};

struct Quadric {
    float aa;
    float ab;
    float ac;
    float ad;
    float bb;
    float bc;
    float bd;
    float cc;
    float cd;
    float dd;
};

// Contains information about the quadric, i.e., sum of all intesecting planes and
// the faces that intersect it
struct Vertex
{
    Quadric    quadric;
    Array<int> faces;
};

struct Mesh
{
    GLuint             vao, vertex_buffer, normal_buffer, index_buffer;
    Array<Vec3<float>> vertices;
    Array<Vec3<int>>   indices;
};

void meshInitFromObjFile(Mesh&, const char* file_name, Arena& arena, Arena& arena_scratch);

void meshBindBuffers(Mesh& mesh, Arena& arena_scratch);

//
//  The q matrix is the sum of all K_p matrices, i.e., that is the matrix formed by P x P^T where P are the vectors
//  containing coefficients of the plane equation for one of the faces intersecting one of the vertices of the models.
//
//  The algorithm will go something like the following.
//
//  1. Calculates the plane equation for all the faces of the model. That results in a 10 dimensional
//  matrix of parameters. Then we find the associated parameters to each vertex, that is the sum of the
//  parameters of each adjacent face. In detail:
//      * Loop over the faces and on each iteration
//          * Pick up the three vectors defining the face
//          * Calculate the plane equation coefficients / normal and store them
//          * Aggregate this contribution to the corresponding vertex that are adjacent to the face in another data structure
//
//  2. Select all valid pairs vertex pairs V1 and V2 to contract
//      * Loop over all of the faces and get all of the pairs. At the end of all the iteration remove the duplicates
//      * Find the optimal contraction target V by solving the linear problem (diagonalization?). If the exact solution
//        does not exists the we find the optimal target in between the pair, end points or mid points.
//      * Find the error V x Q x VT for each of the contraction targets. This is the cost
//
//  3. Sort them by lowest errors
//      3.1 Remove the lowest one, i.e., contract it
//      3.2 Update the costs of all pairs involving the new Target.
//      3.3 Repeat this step from the beginning.
void meshDecimate(Mesh& mesh, Arena& arena, Arena& arena_scratch);

