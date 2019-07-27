# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.15

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/vlab/workspace/autonomous-x-rtsp

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/vlab/workspace/autonomous-x-rtsp/build

# Include any dependencies generated for this target.
include gst_rtsp_lib/CMakeFiles/gst-rtsp-lib.dir/depend.make

# Include the progress variables for this target.
include gst_rtsp_lib/CMakeFiles/gst-rtsp-lib.dir/progress.make

# Include the compile flags for this target's objects.
include gst_rtsp_lib/CMakeFiles/gst-rtsp-lib.dir/flags.make

gst_rtsp_lib/CMakeFiles/gst-rtsp-lib.dir/gst_rtsp.c.o: gst_rtsp_lib/CMakeFiles/gst-rtsp-lib.dir/flags.make
gst_rtsp_lib/CMakeFiles/gst-rtsp-lib.dir/gst_rtsp.c.o: ../gst_rtsp_lib/gst_rtsp.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/vlab/workspace/autonomous-x-rtsp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object gst_rtsp_lib/CMakeFiles/gst-rtsp-lib.dir/gst_rtsp.c.o"
	cd /home/vlab/workspace/autonomous-x-rtsp/build/gst_rtsp_lib && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/gst-rtsp-lib.dir/gst_rtsp.c.o   -c /home/vlab/workspace/autonomous-x-rtsp/gst_rtsp_lib/gst_rtsp.c

gst_rtsp_lib/CMakeFiles/gst-rtsp-lib.dir/gst_rtsp.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/gst-rtsp-lib.dir/gst_rtsp.c.i"
	cd /home/vlab/workspace/autonomous-x-rtsp/build/gst_rtsp_lib && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/vlab/workspace/autonomous-x-rtsp/gst_rtsp_lib/gst_rtsp.c > CMakeFiles/gst-rtsp-lib.dir/gst_rtsp.c.i

gst_rtsp_lib/CMakeFiles/gst-rtsp-lib.dir/gst_rtsp.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/gst-rtsp-lib.dir/gst_rtsp.c.s"
	cd /home/vlab/workspace/autonomous-x-rtsp/build/gst_rtsp_lib && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/vlab/workspace/autonomous-x-rtsp/gst_rtsp_lib/gst_rtsp.c -o CMakeFiles/gst-rtsp-lib.dir/gst_rtsp.c.s

# Object files for target gst-rtsp-lib
gst__rtsp__lib_OBJECTS = \
"CMakeFiles/gst-rtsp-lib.dir/gst_rtsp.c.o"

# External object files for target gst-rtsp-lib
gst__rtsp__lib_EXTERNAL_OBJECTS =

gst_rtsp_lib/libgst-rtsp-lib.a: gst_rtsp_lib/CMakeFiles/gst-rtsp-lib.dir/gst_rtsp.c.o
gst_rtsp_lib/libgst-rtsp-lib.a: gst_rtsp_lib/CMakeFiles/gst-rtsp-lib.dir/build.make
gst_rtsp_lib/libgst-rtsp-lib.a: gst_rtsp_lib/CMakeFiles/gst-rtsp-lib.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/vlab/workspace/autonomous-x-rtsp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C static library libgst-rtsp-lib.a"
	cd /home/vlab/workspace/autonomous-x-rtsp/build/gst_rtsp_lib && $(CMAKE_COMMAND) -P CMakeFiles/gst-rtsp-lib.dir/cmake_clean_target.cmake
	cd /home/vlab/workspace/autonomous-x-rtsp/build/gst_rtsp_lib && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/gst-rtsp-lib.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
gst_rtsp_lib/CMakeFiles/gst-rtsp-lib.dir/build: gst_rtsp_lib/libgst-rtsp-lib.a

.PHONY : gst_rtsp_lib/CMakeFiles/gst-rtsp-lib.dir/build

gst_rtsp_lib/CMakeFiles/gst-rtsp-lib.dir/clean:
	cd /home/vlab/workspace/autonomous-x-rtsp/build/gst_rtsp_lib && $(CMAKE_COMMAND) -P CMakeFiles/gst-rtsp-lib.dir/cmake_clean.cmake
.PHONY : gst_rtsp_lib/CMakeFiles/gst-rtsp-lib.dir/clean

gst_rtsp_lib/CMakeFiles/gst-rtsp-lib.dir/depend:
	cd /home/vlab/workspace/autonomous-x-rtsp/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/vlab/workspace/autonomous-x-rtsp /home/vlab/workspace/autonomous-x-rtsp/gst_rtsp_lib /home/vlab/workspace/autonomous-x-rtsp/build /home/vlab/workspace/autonomous-x-rtsp/build/gst_rtsp_lib /home/vlab/workspace/autonomous-x-rtsp/build/gst_rtsp_lib/CMakeFiles/gst-rtsp-lib.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : gst_rtsp_lib/CMakeFiles/gst-rtsp-lib.dir/depend

