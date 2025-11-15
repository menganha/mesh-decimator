#pragma once
#include "log.hpp"

struct Test
{
    int   status;
    Test* next;
    virtual ~Test() {};
    virtual void        setUp() {};
    virtual void        tearDown() {};
    virtual const char* name() = 0;
    virtual void        run() = 0;
};

static Test* s_test_head = nullptr;

struct TestRegister
{
    // We cannot call functions outside "main" but we can declare and statically initialize objects.
    // Therefore we register every test in a linked list by simply instantiating this class
    TestRegister(Test* test)
    {
        if ( s_test_head )
        {
            test->next = s_test_head;
            s_test_head = test;
        }
        else
        {
            s_test_head = test;
        }
    }
};

#define ASSERT(condition)                                                                             \
    if( not (condition) ){                                                                            \
        LERROR("=== Test \"%s\" failed!\n\t%s:%i: %s", this->name(), __FILE__, __LINE__, #condition); \
        this->status =  1;                                                                            \
    }

#define ASSERT_CLOSE(expected, actual, tolerance)                                                     \
     if ( not ((actual >= (expected - tolerance)) && (actual <= (expected + tolerance))) ){             \
        LERROR("=== Test \"%s\" failed!\n\t%s:%i: Expected %s +- %s but was actual %s", this->name(), __FILE__, __LINE__, #expected, #tolerance, #actual); \
        this->status =  1;                                                                            \
    }

#define DEFINE_TEST(Name, Fixture)                                                 \
    struct Test ## Name ## Fixture: public Fixture                                 \
    {                                                                              \
        const char* name() { return #Name;}                                        \
        virtual void run();                                                        \
    } static test_ ## Name ## Fixture ##  _instance {};                            \
                                                                                   \
    static TestRegister register ## Name (&test_ ## Name ## Fixture ## _instance); \
                                                                                   \
    void Test ## Name ## Fixture::run()

#define TEST(Name) \
    DEFINE_TEST(Name, Test)

#define TEST_WITH_FIXTURE(Name, Fixture) \
    DEFINE_TEST(Name, Fixture)

#define RUN_ALL_TESTS()                                         \
{                                                               \
    if ( not s_test_head )                                      \
    {                                                           \
        LERROR("=== No test has been registered!");             \
    }                                                           \
    else                                                        \
    {                                                           \
        Test* temp = s_test_head;                               \
        int test_total = 0;                                     \
        int test_passed = 0;                                    \
        while ( temp != nullptr )                               \
        {                                                       \
            LINFO("=== Running test \"%s\"", temp->name());     \
            temp->run();                                        \
            test_total++;                                       \
            if ( temp->status == 0 )                            \
            {                                                   \
                LINFO("=== Test \"%s\" passed!", temp->name()); \
                test_passed++;                                  \
            }                                                   \
            temp = temp->next;                                  \
        }                                                       \
        if (test_total == test_passed){                          \
            LINFO("=== Success! %i/%i tests passed", test_passed, test_total);     \
        }                                                       \
        else{                                                   \
            LERROR("=== Failure! %i/%i tests passed", test_passed, test_total);     \
        }                                                       \
    }                                                           \
}

