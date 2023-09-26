#ifndef SHADER_HPP
#define SHADER_HPP
#include "../glad/include/glad/glad.h" 
#include <GLFW/glfw3.h>

#include <iostream>

// Use a Lehmer RNG as PRNG
// https://en.wikipedia.org/wiki/Lehmer_random_number_generator
#define PNRG_a 75
#define PRNG_m 8388617
#define prng(x) ((PNRG_a * x) % PRNG_m)

// Return current CPU cycle
uint64_t get_time(void);

// Create an object program with a vertex shader and a fragment shader
// Code taken from opengl-tutorial (https://github.com/opengl-tutorials/ogl)
unsigned int LoadShaders(const char * vertex_file_path,const char * fragment_file_path);

void setInt(int ID, const std::string &name, int value);

// Process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow *window);

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

#endif
