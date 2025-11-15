#include "mesh.cpp"
#include "test.hpp"

#include <cmath>

static Arena s_arena_scratch = arenaMake(MEGABYTES(8));

struct ArenaFixture : public Test
{
    ~ArenaFixture() { arenaFree(s_arena_scratch); };
};

TEST_WITH_FIXTURE(RemoveDuplicates, ArenaFixture)
{
    Array<Vec3<int>> plane_indices {};
    arraySetCapacity(plane_indices, 6, s_arena_scratch);

    arrayEmplaceBack(plane_indices, 1, 2, 3);
    arrayEmplaceBack(plane_indices, 1, 2, 3);
    arrayEmplaceBack(plane_indices, 1, 2, 3);
    arrayEmplaceBack(plane_indices, 1, 2, 2);
    arrayEmplaceBack(plane_indices, 3, 2, 3);
    arrayEmplaceBack(plane_indices, 1, 1, 2);

    arrayRemoveDuplicates(plane_indices, [](auto& a, auto& b) { return (a.x == b.x) & (a.y == b.y) & (a.z == b.z); });
    ASSERT(plane_indices.length == 4);

    int expected_indices[4][3] = {
      {1, 2, 3},
      {1, 2, 2},
      {3, 2, 3},
      {1, 1, 2},
    };
    for ( int idx = 0; idx < plane_indices.length; idx++ )
    {
        ASSERT(plane_indices[idx].x == expected_indices[idx][0]);
        ASSERT(plane_indices[idx].y == expected_indices[idx][1]);
        ASSERT(plane_indices[idx].z == expected_indices[idx][2]);
    }

    // Test list with single element
    plane_indices.length=1;
    arrayRemoveDuplicates(plane_indices, [](auto& a, auto& b) { return (a.x == b.x) & (a.y == b.y) & (a.z == b.z); });
    ASSERT(plane_indices[0].x == 1);
    ASSERT(plane_indices[0].y == 2);
    ASSERT(plane_indices[0].z == 3);

}


TEST(getWeight)
{
    float vector[3] = {3, 2, 2};
    float cost {};

    Quadric quadric_1 {};
    quadric_1.aa = 2;
    quadric_1.bb = 2;
    quadric_1.cc = 2;
    quadric_1.dd = 1;

    cost = getWeight(quadric_1, vector);
    ASSERT((int)cost == 35);

    Quadric quadric_2 {};
    quadric_2.aa = 2;
    quadric_2.ad = 1;
    quadric_2.bb = 2;
    quadric_2.cc = 2;
    quadric_2.dd = 2;

    cost = getWeight(quadric_2, vector);
    ASSERT((int)cost == 42);

    Quadric quadric_3 {};
    quadric_3.aa = 2;
    quadric_3.ad = 1;
    quadric_3.bb = 2;
    quadric_3.bc = 2;
    quadric_3.cc = 2;
    quadric_3.dd = 2;

    cost = getWeight(quadric_3, vector);
    ASSERT((int)cost == 58);

    Quadric quadric_4 {};
    quadric_4.aa = 2;
    quadric_4.ad = 1;
    quadric_4.bb = 2;
    quadric_4.bc = 2;
    quadric_4.cc = 2;
    quadric_4.cd = 3;
    quadric_4.dd = 2;

    cost = getWeight(quadric_4, vector);
    ASSERT((int)cost == 70);
}

TEST(calculateQuadric)
{
    Vec3<float> p1 {3, 4, 5};
    Vec3<float> p2 {4, 4, 5};
    Vec3<float> p3 {7, 11, 9};
    Quadric     quadric = calculatePlaneMatrix(p1, p2, p3);
    ASSERT((int)quadric.aa == 0);
    ASSERT((int)quadric.ab == 0);
    ASSERT((int)quadric.ac == 0);
    ASSERT((int)quadric.ad == 0);
    ASSERT((int)quadric.bb == 16);
    ASSERT((int)quadric.bc == -28);
    ASSERT((int)quadric.bd == 76);
    ASSERT((int)quadric.cc == 49);
    ASSERT((int)quadric.cd == -133);
    ASSERT((int)quadric.dd == 361);
}

TEST_WITH_FIXTURE(calculateQuadricMatrices, ArenaFixture)
{
    Array<Quadric> quadric_matrix_per_plane {};
    Array<Vertex>  vertex_quadrics {};
    Mesh           mesh {};

    // Generate simple grid
    int width_n_elements = 3;
    int triangle_side_size = 2;

    arraySetCapacity(mesh.vertices, 2 * width_n_elements + 2, s_arena_scratch);
    arraySetCapacity(mesh.indices, 2 * width_n_elements, s_arena_scratch);

    // Create a simple triangle strip
    arrayPushBack(mesh.vertices, {0.0f, 0.0f, 0.0f});
    for ( int idx = 0; idx < width_n_elements; idx++ )
    {
        float x_0 = (float)(idx * triangle_side_size);
        float x = (float)((idx + 1) * triangle_side_size);
        arrayPushBack(mesh.vertices, {x_0, 0.0f, (float)triangle_side_size});
        arrayPushBack(mesh.vertices, {x, 0.0f, 0.0f});
    }
    float max_coord = (float)(width_n_elements * triangle_side_size);
    arrayPushBack(mesh.vertices, {max_coord, 0.0f, (float)triangle_side_size});

    for ( int idx = 0; idx < 2 * width_n_elements; idx++ )
    {
        arrayPushBack(mesh.indices, {idx, idx + 1, idx + 2});
    }

    calculateQuadricMatrices(quadric_matrix_per_plane, vertex_quadrics, mesh, s_arena_scratch);

    for ( auto& quadric : quadric_matrix_per_plane )
    {

        ASSERT(std::round(quadric.aa) == 0);
        ASSERT(std::round(quadric.ab) == 0);
        ASSERT(std::round(quadric.ac) == 0);
        ASSERT(std::round(quadric.ad) == 0);
        ASSERT(std::round(quadric.bb) == 16);
        ASSERT(std::round(quadric.bc) == 0);
        ASSERT(std::round(quadric.bd) == 0);
        ASSERT(std::round(quadric.cc) == 0);
        ASSERT(std::round(quadric.cd) == 0);
        ASSERT(std::round(quadric.dd) == 0);
    }

    int idx = 0;
    for ( auto& quadric : vertex_quadrics )
    {

        if ( idx == 0 or idx == 7 )
        {
            ASSERT(quadric.faces.length = 1);
            ASSERT(quadric.quadric.bb == 16);
        }
        else if ( idx == 1 or idx == 6 )
        {
            ASSERT(quadric.faces.length = 2);
            ASSERT(quadric.quadric.bb == 32);
        }
        else
        {
            ASSERT(quadric.faces.length = 3);
            ASSERT(quadric.quadric.bb == 48);
        }
        idx++;
    }
}

TEST(CalculateVectorFromMinPlaneDistance)
{
    Quadric quadric {2, 3, 4, 5, 4, 7, 1, 1, 1, 2};
    float   optimal_vector[3];
    float   det = calculateVectorFromMinPlaneDistance(quadric, optimal_vector);
    LINFO("determinant %f", (double)det);
    LINFO("optimal_vector %f, %f, %f", (double)optimal_vector[0], (double)optimal_vector[1], (double)optimal_vector[2]);
    ASSERT_CLOSE(det, 5.000f, 0.0001f);
    ASSERT_CLOSE(optimal_vector[0], 39.f, 0.0001f);
    ASSERT_CLOSE(optimal_vector[1], -21.8f, 0.0001f);
    ASSERT_CLOSE(optimal_vector[2], -4.4f, 0.0001f);
}
//
// static int test_array(Arena& arena)
// {
//     struct CustomStruct
//     {
//         int a;
//         int b;
//     };
//     Array<CustomStruct> array {};
//     ArrayAlloc(array, 4, arena);
//     ASSERT(array.length == 0);
//     ASSERT(array.capacity == 4);
//
//     array.emplace_back(1, 2);
//     array.emplace_back(3, 4);
//     array.emplace_back(5, 6);
//
//     ASSERT(array.length == 3);
//
//     ASSERT(array[0].a == 1);
//     ASSERT(array[0].b == 2);
//
//     ASSERT(array[1].a == 3);
//     ASSERT(array[1].b == 4);
//
//     ASSERT(array[2].a == 5);
//     ASSERT(array[2].b == 6);
//
//     ArrayResize(array, 5);
//
//     ASSERT(array.capacity == 5);
//     ASSERT(array.length == 3);
//
//     ASSERT(array[0].a == 1);
//     ASSERT(array[0].b == 2);
//
//     ASSERT(array[1].a == 3);
//     ASSERT(array[1].b == 4);
//
//     ASSERT(array[2].a == 5);
//     ASSERT(array[2].b == 6);
//
//     array.emplace_back(1, 1);
//     array.emplace_back(1, 1);
//     array.emplace_back(1, 1);
//
//     ASSERT(array.length == 6);
//     ASSERT(array.capacity == 10);
//
//     ASSERT(array[0].a == 1);
//     ASSERT(array[0].b == 2);
//     ASSERT(array[1].a == 3);
//     ASSERT(array[1].b == 4);
//     ASSERT(array[2].a == 5);
//     ASSERT(array[2].b == 6);
//     ASSERT(array[3].a == 1);
//     ASSERT(array[3].b == 1);
//     ASSERT(array[4].a == 1);
//     ASSERT(array[4].b == 1);
//     ASSERT(array[5].a == 1);
//     ASSERT(array[5].b == 1);
//
//     return 0;
// }

int main(int argc, char** argv)
{
    // runAllTests();
    RUN_ALL_TESTS();
    // RUN_TEST(emplace_back(arena_scratch));
    // RUN_TEST(get_weight(arena_scratch));
    // RUN_TEST(test_array(arena_scratch));
    // arenaFree(s_arena_scratch);
}
