# Disable built-in rules and variables
MAKEFLAGS += --no-builtin-rules
MAKEFLAGS += --no-builtin-variables


#
# The only part the must be modified by the user
#
EXECUTABLE := decimator
SRCS_APP := src/main.cpp 

LIB := meshlib.a
SRCS_LIB := src/log.cpp src/mesh.cpp src/camera.cpp src/arena.cpp src/shaders.cpp libs/glad/src/glad.c

SRCS_TEST := test/test_decimator.cpp

#
# Sets include directories and builds flags
#
INC_FLAGS := -Ilibs/glad/include -Isrc
CPPFLAGS := -MMD -MP
DEBUG ?= 0
ifeq ($(DEBUG), 1)
	BUILD_DIR := ./build/debug
	CXXFLAGS := -g -Wall -Wpedantic -Wextra -Wno-unused-parameter -Wdouble-promotion
	LDFLAGS := -lglfw
	# SANITIZER := -fsanitize=address
	SANITIZER := 
else
	BUILD_DIR := ./build/release
	CXXFLAGS := -O3 -DNDEBUG -DLOGOFF
	LDFLAGS := -lglfw
endif

CXX := g++
CC := cc

#
# Rules
# 

OBJS_APP = $(SRCS_APP:%=$(BUILD_DIR)/%.o)
OBJS_LIB = $(SRCS_LIB:%=$(BUILD_DIR)/%.o)
DEPS = $(OBJS_APP:.o=.d) 
DEPS = $(OBJS_LIB:.o=.d)

.PHONY: all clean test

all: $(BUILD_DIR)/$(EXECUTABLE)

$(BUILD_DIR)/$(EXECUTABLE): $(OBJS_APP) $(BUILD_DIR)/$(LIB)
	$(CXX) $^ -o $@ $(LDFLAGS) $(SANITIZER)

$(BUILD_DIR)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(INC_FLAGS) $(SANITIZER) -c $< -o $@

# Library rule
$(BUILD_DIR)/$(LIB): $(OBJS_LIB)
	ar rcs $@ $^

# Building GLAD no need to have debug symbols at all
$(BUILD_DIR)/libs/glad/src/glad.c.o: libs/glad/src/glad.c
	@mkdir -p $(dir $@)
	$(CC) -O3 -DNDEBUG -Ilibs/glad/include -c $< -o $@

# Test setup. We create one target per each test file and link it against the lib
OBJS_TEST = $(SRCS_TEST:%=$(BUILD_DIR)/%.o)
TARGETS_TEST = $(foreach var, $(SRCS_TEST), $(BUILD_DIR)/$(basename $(notdir $(var)))) 
DEPS += $(OBJS_TEST:.o=.d)

test: $(TARGETS_TEST)
	for executable in $^; do ./$$executable; done

$(TARGETS_TEST): $(OBJS_TEST) $(BUILD_DIR)/$(LIB)
	$(CXX) $^ -o $@ $(LDFLAGS) $(SANITIZER) -Isrc
	# ./$@

clean:
	-rm -r $(BUILD_DIR)

# Include the .d makefiles. The - at the front suppresses the errors of missing makefiles during the first run
-include $(DEPS)

