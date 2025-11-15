#include "mesh.hpp"

#include "log.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>

static const std::size_t BUFFER_SIZE = KILOBYTES(8);
static const std::size_t MAX_VECTOR_SIZE = 100000;
static const int         MAX_FACES_PER_VERTEX = 50;
static const int         MAX_AFFECTED_PAIRS = 1000; // affected pairs after contraction
static const int         ITERATIONS_PER_STEP = 200;
static const int         MIN_MESH_SIZE = 50;

// Holds intermediate results of the decimation. In order not to recalculate it again and again
struct CacheAux
{
    Array<Pair>    pair_array;
    Array<Vertex>  vertex_data;
    Array<Quadric> quadric_per_face;
    Array<int>     faces_removed;
};

CacheAux g_cache {};

static bool IsCacheInitialized(CacheAux& cache_aux) { return (cache_aux.pair_array.data != nullptr); }

//
// Parses a chunk of text data line by line
//
static void parseChunk(char* buffer, Array<Vec3<float>>& vertices, Array<Vec3<int>>& indices)
{

    char* line = std::strtok(buffer, "\n");
    float x, y, z;
    int   fx, fy, fz;
    int   parsed;

    while ( line )
    {
        size_t idx = std::strspn(line, " ");
        char   first_char = line[idx];
        char   second_char = line[++idx];

        switch ( first_char )
        {
        case ('v'):
            if ( second_char != 't' )
            { // TODO: Improve this, perhaps remove switch statement
                parsed = std::sscanf(line, "%*s %f %f %f", &x, &y, &z);
                if ( parsed == 3 )
                {
                    if ( second_char != 'n' and second_char != 't' )
                    {
                        arrayEmplaceBack(vertices, x, y, z);
                    }
                    else
                        LTRACE("Ignoring vertex normal line: %s:", line);
                }
                else
                    LERROR("Error parsing vertex line: %s", line);
            }
            break;
        case ('f'):
            // TODO: Support input lines without dashes
            parsed = std::sscanf(line, "%*s %d%*2[/]%*d %d%*2[/]%*d %d%*2[/]%*d", &fx, &fy, &fz);
            if ( parsed == 3 )
                arrayEmplaceBack(indices, fx - 1, fy - 1, fz - 1);
            else
            {
                parsed = std::sscanf(line, "%*s %d %d %d", &fx, &fy, &fz);
                if ( parsed == 3 )
                    arrayEmplaceBack(indices, fx - 1, fy - 1, fz - 1);
                else
                    LERROR("Error parsing face index line: %s", line);
            }
            break;
        case ('#'):
            LTRACE("Ignoring comment line %s: ", line);
            break;
        default:
            LWARN("Unhandled line of the .obj file: %s", line);
        }

        line = std::strtok(nullptr, "\n");
    }
}

void meshInitFromObjFile(Mesh& mesh, const char* file_name, Arena& arena, Arena& arena_scratch)
{

    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vertex_buffer);
    glGenBuffers(1, &mesh.normal_buffer);
    glGenBuffers(1, &mesh.index_buffer);

    FILE* file_handler = fopen(file_name, "rb");
    if ( file_handler == nullptr )
    {
        LERROR("Error opening the input file %s", file_name);
        // Return a simple stub triangle
        arraySetCapacity(mesh.vertices, 3, arena);
        arrayPushBack(mesh.vertices, Vec3<float> {-0.5f, -0.5f, 0.0f});
        arrayPushBack(mesh.vertices, Vec3<float> {0.5f, -0.5f, 0.0f});
        arrayPushBack(mesh.vertices, Vec3<float> {0.0f, 0.5f, 0.0f});

        arraySetCapacity(mesh.indices, 1, arena);
        arrayPushBack(mesh.indices, Vec3<int> {0, 1, 2});

        return;
    }

    // TODO: Set the max size dynamically after doing a single pass to the file
    arraySetCapacity(mesh.vertices, MAX_VECTOR_SIZE, arena);
    arraySetCapacity(mesh.indices, MAX_VECTOR_SIZE, arena);

    //
    // Parses file by chunks
    //
    char* buffer = arenaAlloc<char>(arena_scratch, 2 * BUFFER_SIZE);
    char* ptr_start = buffer;
    char* ptr_end;
    char* ptr_last_new_line;

    for ( ;; )
    {
        size_t read = std::fread(ptr_start, 1, BUFFER_SIZE, file_handler);
        ptr_end = ptr_start + read;

        if ( read == BUFFER_SIZE ) // Process the file until the last new line and continue looping
        {
            // Finds the last new line
            ptr_last_new_line = ptr_end;
            while ( ptr_last_new_line > buffer )
            {
                ptr_last_new_line--;
                if ( *ptr_last_new_line == '\n' )
                    break;
            }
            *ptr_last_new_line = '\0';
            ptr_last_new_line++;
            parseChunk(buffer, mesh.vertices, mesh.indices);

            // Copies the overflow at the start of the buffer for the next iteration
            std::size_t overflow_bytes = static_cast<std::size_t>(ptr_end - ptr_last_new_line);
            std::memmove(buffer, ptr_last_new_line, overflow_bytes);
            ptr_start = buffer + overflow_bytes;
        }
        else
        {
            if ( std::feof(file_handler) ) // Either we have read the entire file or reached the final chunk.
            {
                *ptr_end = '\0';           // Sets the null terminator at the end to properly signal an end of a line to the parser
                parseChunk(buffer, mesh.vertices, mesh.indices);
                LDEBUG("End of the file reached");
                break;
            }
            else if ( int err_code = std::ferror(file_handler) )
            {
                // Some error happened log it and break
                LDEBUG("File Error: %i", err_code);
                break;
            }
        }
    }
    fclose(file_handler);
    arenaReset(arena_scratch);
}

//
//  Sends the data to the GPU
//
void meshBindBuffers(Mesh& mesh, Arena& arena_scratch)
{
    // Bind vertex array object
    glBindVertexArray(mesh.vao);

    // Bind vertices to layout location 0
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(mesh.vertices[0]) * mesh.vertices.length, &mesh.vertices[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); // This allows usage of layout location 0 in the vertex shader
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);

    // Bind elements
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.index_buffer);
    if ( !IsCacheInitialized(g_cache) )
    {
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(mesh.indices[0]) * mesh.indices.length, &mesh.indices[0], GL_STATIC_DRAW);
    }
    else
    {
        Array<Vec3<int>> mesh_indices_copy {};
        arraySetCapacity(mesh_indices_copy, mesh.indices.length, arena_scratch);

        int idx_counter = 0;
        for ( Vec3<int>& indices : mesh.indices )
        {
            int* it = arrayFind(g_cache.faces_removed, idx_counter);
            if ( it == g_cache.faces_removed.end() )
            {
                arrayPushBack(mesh_indices_copy, indices);
            }
            idx_counter++;
        }
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(mesh_indices_copy[0]) * mesh_indices_copy.length, &mesh_indices_copy[0], GL_STATIC_DRAW);
    }
    arenaReset(arena_scratch);

    // Unbind everything before exiting
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

// Brute force calculation of the determinant for 3x3 matrix
static inline float determinant3(const float mat3x3[3][3])
{
    float det =
      (mat3x3[0][0] * (mat3x3[1][1] * mat3x3[2][2] - mat3x3[2][1] * mat3x3[1][2])) -
      (mat3x3[0][1] * (mat3x3[1][0] * mat3x3[2][2] - mat3x3[2][0] * mat3x3[1][2])) +
      (mat3x3[0][2] * (mat3x3[1][0] * mat3x3[2][1] - mat3x3[2][0] * mat3x3[1][1]));
    return det;
}

//
//  Gets the vector that will minimize the distance between all of the planes surrounding the pair.
//  Uses the fact that some values of the matrix are outright zero or one for this specific problem
//  so that it reduces the amount of operations. See paper Eq(1)
//
static inline float calculateVectorFromMinPlaneDistance(const Quadric& quadric, float optimal_vector[3])
{

    float A2312 = quadric.bc * quadric.cd - quadric.bd * quadric.cc;
    float A1312 = quadric.bb * quadric.cd - quadric.bd * quadric.bc;
    float A1212 = quadric.bb * quadric.cc - quadric.bc * quadric.bc;
    float A0312 = quadric.ab * quadric.cd - quadric.bd * quadric.ac;
    float A0212 = quadric.ab * quadric.cc - quadric.bc * quadric.ac;
    float A0112 = quadric.ab * quadric.bc - quadric.bb * quadric.ac;

    float det = quadric.aa * A1212 - quadric.ab * A0212 + quadric.ac * A0112;

    float inv_det = 1 / det;

    optimal_vector[0] = inv_det * (quadric.ac * A1312 - quadric.ab * A2312 - quadric.ad * A1212);
    optimal_vector[1] = inv_det * (quadric.aa * A2312 - quadric.ac * A0312 + quadric.ad * A0212);
    optimal_vector[2] = inv_det * (quadric.ab * A0312 - quadric.aa * A1312 - quadric.ad * A0112);

    return det;
}

//
//  Find the contraction between vector^Transposed * matrix * vector using the symmetries of the
//  problem, i.e, the vector having the last component equal to 1 and the matrix being symmetric
//
static float getWeight(const Quadric& quadric, const float optimal_vector[3])
{
    float weight =
      quadric.aa * optimal_vector[0] * optimal_vector[0] +
      quadric.bb * optimal_vector[1] * optimal_vector[1] +
      quadric.cc * optimal_vector[2] * optimal_vector[2] +
      quadric.dd +
      2.f * quadric.ab * optimal_vector[0] * optimal_vector[1] +
      2.f * quadric.ac * optimal_vector[0] * optimal_vector[2] +
      2.f * quadric.bc * optimal_vector[1] * optimal_vector[2] +
      2.f * quadric.ad * optimal_vector[0] +
      2.f * quadric.bd * optimal_vector[1] +
      2.f * quadric.cd * optimal_vector[2];
    return weight;
}

static inline Vec3<int> sortVec(const Vec3<int>& vector)
{
    int x = vector.x;
    int y = vector.y;
    int z = vector.z;
    int tmp = 0;

    if ( x > z )
    {
        tmp = x;
        x = z;
        z = tmp;
    }
    if ( x > y )
    {
        tmp = x;
        x = y;
        y = tmp;
    }
    if ( y > z )
    {
        tmp = y;
        y = z;
        z = tmp;
    }
    return Vec3<int> {x, y, z};
}

//
//  Calculates the plane coefficients for a face. Then stores the 10 unique coefficients
//  of the corresponding symetric quadric matrix.
//
//  The equation of a plane determined by three points that can be calculated with the equation
//  det ( [[x, y, z, 1], [a1, a2, a3, 1], [b1, b2, b3, 1], [c1, c2, c3, 1] ) = 0
//
//  or if we derive
//
//  x * det ( [a2, a3, 1],  [b2, b3, 1],  [c2, c3, 1] ) +
//  y * det ( [a1, a3, 1],  [b1, b3, 1],  [c1, c3, 1] ) +
//  z * det ( [a1, a2, 1],  [b1, b2, 1],  [c1, c2, 1] ) +
//  1 * det ( [a1, a2, a3], [b1, b2, b3], [c1, c2, b3] )  = 0
//
//  Only the determinants values are needed to determine the 4 coefficient of the plane equation
//
static inline Quadric calculatePlaneMatrix(const Vec3<float>& vertex_a, const Vec3<float>& vertex_b, const Vec3<float>& vertex_c)
{
    float mat_x[3][3] {
      {vertex_a.y, vertex_a.z, 1.0f},
      {vertex_b.y, vertex_b.z, 1.0f},
      {vertex_c.y, vertex_c.z, 1.0f}
    };
    float coeff_a = determinant3(mat_x);

    float mat_y[3][3] {
      {vertex_a.x, vertex_a.z, 1.0f},
      {vertex_b.x, vertex_b.z, 1.0f},
      {vertex_c.x, vertex_c.z, 1.0f}
    };
    float coeff_b = -determinant3(mat_y);

    float mat_z[3][3] {
      {vertex_a.x, vertex_a.y, 1.0f},
      {vertex_b.x, vertex_b.y, 1.0f},
      {vertex_c.x, vertex_c.y, 1.0f}
    };
    float coeff_c = determinant3(mat_z);

    float mat_d[3][3] {
      {vertex_a.x, vertex_a.y, vertex_a.z},
      {vertex_b.x, vertex_b.y, vertex_b.z},
      {vertex_c.x, vertex_c.y, vertex_c.z}
    };
    float coeff_d = -determinant3(mat_d);

    return Quadric {
      coeff_a * coeff_a,
      coeff_a * coeff_b,
      coeff_a * coeff_c,
      coeff_a * coeff_d,
      coeff_b * coeff_b,
      coeff_b * coeff_c,
      coeff_b * coeff_d,
      coeff_c * coeff_c,
      coeff_c * coeff_d,
      coeff_d * coeff_d};
}

static void calculateQuadricMatrices(Array<Quadric>& quadrics_per_plane, Array<Vertex>& vertex_data, const Mesh& mesh, Arena& arena)
{

    arraySetCapacity(quadrics_per_plane, mesh.indices.length, arena);
    arraySetCapacity(vertex_data, mesh.vertices.length, arena);

    // No need to push back anything to the vertex_data as all elements are already initialized to zero just as we want
    vertex_data.length = mesh.vertices.length;

    for ( auto& vertex : vertex_data )
        arraySetCapacity(vertex.faces, MAX_FACES_PER_VERTEX, arena);

    int idx_plane = 0;
    for ( const Vec3<int>& vertex_indices : mesh.indices )
    {
        // = quadric_per_plane[idx_plane];
        Quadric quadric = calculatePlaneMatrix(mesh.vertices[vertex_indices.x], mesh.vertices[vertex_indices.y], mesh.vertices[vertex_indices.z]);
        arrayPushBack(quadrics_per_plane, quadric);

        // Sums the contribution to each vertex of the plane and assings it to the corresponding vertex quadric element.
        // In other words, this will become the sum of all the quadrics of the intersecting planes to that vertex to be used later

        // Loop over the vertex of the plane and then assing to each of this vertices the contribution of the same plane
        for ( int idx_vtx = 0; idx_vtx < 3; idx_vtx++ )
        {
            int idx_vertex = vertex_indices[idx_vtx];
            arrayEmplaceBack(vertex_data[idx_vertex].faces, idx_plane);
            Quadric& vertex_quadric = vertex_data[idx_vertex].quadric;
            vertex_quadric.aa += quadric.aa;
            vertex_quadric.ab += quadric.ab;
            vertex_quadric.ac += quadric.ac;
            vertex_quadric.ad += quadric.ad;
            vertex_quadric.bb += quadric.bb;
            vertex_quadric.bc += quadric.bc;
            vertex_quadric.bd += quadric.bd;
            vertex_quadric.cc += quadric.cc;
            vertex_quadric.cd += quadric.cd;
            vertex_quadric.dd += quadric.dd;
        }
        idx_plane++;
    }
}

//
// Recalculate a quadric at the given vertex index
//
static void recalculateQuadric(const Array<Quadric>& quadric_per_plane, Array<Vertex>& vertex_data, int vertex_index)
{
    Quadric& vertex_quadric = vertex_data[vertex_index].quadric;
    vertex_quadric.aa = 0.f;
    vertex_quadric.ab = 0.f;
    vertex_quadric.ac = 0.f;
    vertex_quadric.ad = 0.f;
    vertex_quadric.bb = 0.f;
    vertex_quadric.bc = 0.f;
    vertex_quadric.bd = 0.f;
    vertex_quadric.cc = 0.f;
    vertex_quadric.cd = 0.f;
    vertex_quadric.dd = 0.f;

    for ( int face_idx : vertex_data[vertex_index].faces )
    {
        const Quadric& quadric = quadric_per_plane[face_idx];
        vertex_quadric.aa += quadric.aa;
        vertex_quadric.ab += quadric.ab;
        vertex_quadric.ac += quadric.ac;
        vertex_quadric.ad += quadric.ad;
        vertex_quadric.bb += quadric.bb;
        vertex_quadric.bc += quadric.bc;
        vertex_quadric.bd += quadric.bd;
        vertex_quadric.cc += quadric.cc;
        vertex_quadric.cd += quadric.cd;
        vertex_quadric.dd += quadric.dd;
    }
}

static void calculateCosts(Pair& pair, const Array<Vertex>& vertex_data, const Mesh& mesh)
{
    const Quadric& quadric_v1 = vertex_data[pair.v1].quadric;
    const Quadric& quadric_v2 = vertex_data[pair.v2].quadric;

    Quadric quadric_sum {quadric_v1.aa + quadric_v2.aa, quadric_v1.ab + quadric_v2.ab, quadric_v1.ac + quadric_v2.ac, quadric_v1.ad + quadric_v2.ad, quadric_v1.bb + quadric_v2.bb, quadric_v1.bc + quadric_v2.bc, quadric_v1.bd + quadric_v2.bd, quadric_v1.cc + quadric_v2.cc, quadric_v1.cd + quadric_v2.cd, quadric_v1.dd + quadric_v2.dd};

    // Find the optimal distance between the two vertices via minimizing the cost equation (see Eq. 1).
    // If this doesn't work numerically, then choose between the minimal distance between the endpoints
    // or the middle point. NOTE: Only the middle point alternative is at the moment implemented.
    float optimal_vector[3] {};
    float det = calculateVectorFromMinPlaneDistance(quadric_sum, optimal_vector);
    if ( std::abs(det) < 1e-5f )
    {
        LTRACE("Determinant for pair (%i,%i) is close to zero: %e", pair.v1, pair.v2, (double)det);
        const Vec3<float>& vertex_a = mesh.vertices[pair.v1];
        const Vec3<float>& vertex_b = mesh.vertices[pair.v2];
        optimal_vector[0] = vertex_a[0] / 2 + vertex_b[0] / 2;
        optimal_vector[1] = vertex_a[1] / 2 + vertex_b[1] / 2;
        optimal_vector[2] = vertex_a[2] / 2 + vertex_b[2] / 2;
    }
    pair.optimal.x = optimal_vector[0];
    pair.optimal.y = optimal_vector[1];
    pair.optimal.z = optimal_vector[2];
    auto cost = getWeight(quadric_sum, optimal_vector);
    pair.cost = cost;
}

//
// Takes care of initializing the variables used thorought the decimation process and caches the intermediate results
//
static void InitDecimation(Mesh& mesh, Arena& arena)
{

    if ( IsCacheInitialized(g_cache) )
    {
        return;
    }
    //
    //  Creates all potential contracting pairs. We create three per vertex and after we remove duplicates
    //  Loop over all contracting pairs V1 and V2. The pair selection is for the moment, just connected
    //  pairs, i.e., the edges of the mesh.
    //
    //  For triangular meshes manifolds, we know that each face has three incident "half" edges, therefore:
    //
    //  2* N_Edges = 2 * N_Pairs =  3 * N_Faces
    //  N_Edges = N_Pairs = 3/2 * N_Faces
    //
    //  Howeveer models are not usually manifolds so we simply take them to be 3 edges per face, and resize
    //  if needed
    //
    arraySetCapacity(g_cache.pair_array, mesh.indices.length * 3, arena);
    for ( Vec3<int>& plane_indices : mesh.indices )
    {
        // Set the smaller index always first in the pair as to avoid adding the equivalent flipped pairs
        Vec3<int> sorted_plane_indices = sortVec(plane_indices);
        int       v1_idx, v2_idx;
        int       vertex_indices[3][2] = {
          {sorted_plane_indices[0], sorted_plane_indices[1]},
          {sorted_plane_indices[1], sorted_plane_indices[2]},
          {sorted_plane_indices[0], sorted_plane_indices[2]},
        };

        for ( auto& v_idx : vertex_indices )
        {
            v1_idx = v_idx[0];
            v2_idx = v_idx[1];
            Pair new_pair {
              v1_idx, v2_idx, 0.f, {0.f, 0.f, 0.f}
            };

            Pair* pair = arrayFind(g_cache.pair_array, new_pair, [](auto& p_a, auto& p_b) { return (p_a.v1 == p_b.v1) and (p_a.v2 == p_b.v2); });
            if ( pair == g_cache.pair_array.end() )
            {

                arrayPushBack(g_cache.pair_array, Pair {
                                                    v1_idx, v2_idx, 0.f, {0.f, 0.f, 0.f}
                });
            }
        }
    }

    // Calculate quadric matrices
    arraySetCapacity(g_cache.quadric_per_face, mesh.indices.length, arena);
    arraySetCapacity(g_cache.vertex_data, mesh.vertices.length, arena);

    calculateQuadricMatrices(g_cache.quadric_per_face, g_cache.vertex_data, mesh, arena);

    // Caclulate the initial cost of each vertex contraction
    for ( Pair& pair : g_cache.pair_array )
    {
        calculateCosts(pair, g_cache.vertex_data, mesh);
    }

    // Allocate space for removed faces from the mesh
    arraySetCapacity(g_cache.faces_removed, mesh.indices.length, arena);
}

void meshDecimate(Mesh& mesh, Arena& arena, Arena& arena_scratch)
{
    InitDecimation(mesh, arena);

    // Aid array to to keep count of the faces removed for each iteration
    Array<int> iter_faces_to_remove {};
    arraySetCapacity(iter_faces_to_remove, MAX_AFFECTED_PAIRS, arena_scratch);

    // Aid array. first elemnt tracks the index on the pair array and the other two the indices of the vertex in the vertex array
    Array<int> pairs_to_be_removed {};
    arraySetCapacity(pairs_to_be_removed, MAX_AFFECTED_PAIRS, arena_scratch);

    // Aid array. first elemnt tracks the index on the pair array and the other two the indices of the vertex in the vertex array
    Array<Vec3<int>> affected_pairs {};
    arraySetCapacity(affected_pairs, MAX_AFFECTED_PAIRS, arena_scratch);

    // Aid array. first elemnt tracks the index on the pair array and the other two the indices of the vertex in the vertex array
    Array<int> affected_pairs_indices {};
    arraySetCapacity(affected_pairs_indices, MAX_AFFECTED_PAIRS, arena_scratch);

    // Iteration
    // TODO: Iteration limit should be a function of some global error function
    for ( int iter_step = 0; iter_step < ITERATIONS_PER_STEP; iter_step++ )
    {
        if ( g_cache.pair_array.length < MIN_MESH_SIZE )
        {
            LWARN("Model already at the lowest available resolution. Cannot continue decimating");
            return;
        }

        // Finds minimum
        // TODO: Keep a stack of ordered elements and rearange the new one instead of calculatig the minimum on each frame
        float min = g_cache.pair_array[0].cost;
        int   min_idx {};
        for ( int idx = 1; idx < g_cache.pair_array.length; idx++ )
        {
            if ( g_cache.pair_array[idx].cost < min )
            {
                min = g_cache.pair_array[idx].cost;
                min_idx = idx;
            }
        }

        // Contract pairs
        Pair pair_to_contract = g_cache.pair_array[min_idx]; // Copy the pair
        int  idx_to_remove = pair_to_contract.v2;
        int  idx_to_keep = pair_to_contract.v1;

        Array<int>& faces_to_remove = g_cache.vertex_data[idx_to_remove].faces;
        Array<int>& faces_to_keep = g_cache.vertex_data[idx_to_keep].faces;

        iter_faces_to_remove.length = 0;
        for ( int face : faces_to_remove )
        {
            int* it = arrayFind(faces_to_keep, face);
            if ( it == faces_to_keep.end() )
            {
                // Add faces that are not the vertex to keep as we are merging the vertices
                arrayPushBack(faces_to_keep, face);
            }
            else
            {
                // If found it means is one of the degenerate ones.
                // Add the face to the ones to be removed from the mesh and remove them from the vertex_data
                arrayPushBack(iter_faces_to_remove, *it);
                *it = faces_to_keep.back();
                faces_to_keep.length--;
            }
        }

        // Then update the mesh vertices and mesh indices
        mesh.vertices[idx_to_keep] = pair_to_contract.optimal;
        for ( int face_idx : faces_to_keep )
        {
            Vec3<int>& vec = mesh.indices[face_idx];
            for ( int idx = 0; idx < 3; idx++ )
            {
                if ( vec[idx] == idx_to_remove )
                    vec[idx] = idx_to_keep;
            }
        }

        // Recalculate quadric for the vertex to keep after the contraction
        recalculateQuadric(g_cache.quadric_per_face, g_cache.vertex_data, idx_to_keep);

        // Remove the pair to contract from the pairs list and replaces vertex idx on the rest of the pairs.
        // In the same loop we can check which pairs have been affected by the contraction
        arrayPopAt(g_cache.pair_array, min_idx);
        affected_pairs_indices.length = 0;
        pairs_to_be_removed.length = 0;

        int pair_idx = 0;
        for ( Pair& pair : g_cache.pair_array )
        {
            int vertex_to_mod = -1;
            if ( pair.v1 == idx_to_remove )
            {
                pair.v1 = idx_to_keep;
            }
            else if ( pair.v2 == idx_to_remove )
            {
                pair.v2 = idx_to_keep;
            }

            if ( pair.v1 == idx_to_keep )
            {
                vertex_to_mod = pair.v2;
            }
            else if ( pair.v2 == idx_to_keep )
            {
                vertex_to_mod = pair.v1;
            }
            if ( vertex_to_mod != -1 )
            {
                arrayEmplaceBack(affected_pairs_indices, pair_idx);
                // We need to recalculate the quadrics for these and remove the degenerate face.
                Array<int>& faces = g_cache.vertex_data[vertex_to_mod].faces;
                for ( int face : iter_faces_to_remove )
                {
                    int* it = arrayFind(faces, face);
                    if ( it != faces.end() )
                    {
                        // If found it means is one of the degenerate ones.Remove them from the vertex_data
                        *it = faces.back();
                        faces.length--;
                    }
                }
                recalculateQuadric(g_cache.quadric_per_face, g_cache.vertex_data, vertex_to_mod);
            }
            pair_idx++;
        }

        affected_pairs.length = 0; // reset length of array
        for ( int idx : affected_pairs_indices )
        {
            Vec3<int>  pair_data {idx, g_cache.pair_array[idx].v1, g_cache.pair_array[idx].v2};
            Vec3<int>* it = arrayFind(affected_pairs, pair_data, [](auto& a, auto& b) { return ((a.y == b.y) and (a.z == b.z)) or ((a.y == b.z) and (a.z == b.y)); });
            if ( it == affected_pairs.end() )
            {
                arrayEmplaceBack(affected_pairs, pair_data);
                calculateCosts(g_cache.pair_array[idx], g_cache.vertex_data, mesh);
            }
            else
            {
                arrayEmplaceBack(pairs_to_be_removed, idx);
            }
        }

        // LASSERT(pairs_to_be_removed.length >= iter_faces_to_remove.length, "Number of faces to remove is larger than the number of duplicate pairs after contraction");

        // Remove duplicate pairs in inverse order as to pop correctly the series
        for ( int* p_idx = pairs_to_be_removed.end(); p_idx-- != pairs_to_be_removed.begin(); )
        {
            arrayPopAt(g_cache.pair_array, *p_idx);
        }

        for ( int face : iter_faces_to_remove )
            arrayPushBack(g_cache.faces_removed, face);
    }

    arenaReset(arena_scratch);
}

