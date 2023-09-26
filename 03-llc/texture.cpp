// Code inspired by LearnOpenGL (Joey de Vries, twitter: https://twitter.com/JoeyDeVriez) https://github.com/JoeyDeVries/LearnOpenGL
// Used under CC BY-NC 4.0 license (https://creativecommons.org/licenses/by-nc/4.0/).
// Modifications made are integrating the original code with additional functionality.

#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include "shader.hpp"
#include <cstdlib>
#include <inttypes.h>
#include <immintrin.h>
#include <iostream>
#include <fstream>
#include <sys/time.h>
#include <unistd.h>
#include <filesystem>
#include "pputil.c"

int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        fprintf(stderr, "Wrong Input! Enter: %s <color> <sample_counter> <size> <filename> \n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Read in args
    float color;
    // color: 0 Black, 1 Random
    // color: Any number bigger than 0 creates either Skew (color does not divide SCR_WIDTH) or Gradient (color divides SCR_WIDTH)
    // For every pixel on the screen: pixel.R = pixel.G = pixel.B = pixel.x%color (pixel.x is the coordinate of this pixel in the x direction)
    sscanf(argv[1], "%f", &color);

    // The number of LLC walk sample
    uint64_t sample_counter;
    sscanf(argv[2], "%lu", &sample_counter);

    // SCR_WIDTH and SCR_HEIGHT define the dimension of the screen
    // By default, they are the same (the screen is a square)
    unsigned int SCR_WIDTH = 0;
    unsigned int SCR_HEIGHT = 0;
    sscanf(argv[3], "%u", &SCR_WIDTH);
    sscanf(argv[3], "%u", &SCR_HEIGHT);

    // Output file
    char filename[80];
    sscanf(argv[4], "%s", filename);

    // Decide the LLC walk time when every cacheline are brought into the cache
    uint64_t cache_hit = 0;
    for (int i = 0; i < 1000; i++)
    {
        uint64_t llc_reading = measureOnce();
        if (i > 500)
        { // All Cacheline should be in already
            cache_hit = cache_hit + llc_reading;
        }
    }
    cache_hit = cache_hit / 500;
    std::cout << "Hit: " << cache_hit << std::endl;

    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // glfw window creation
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LLC", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Use std::filesystem to extract the relative path from the current directory to the executable directory
    std::filesystem::path executablePath = argv[0];
    std::filesystem::path executableDir = executablePath.parent_path();
    std::filesystem::path currentDir = std::filesystem::current_path();
    std::filesystem::path relativePath = std::filesystem::relative(executableDir, currentDir);

    std::string v1 = relativePath.string() + std::string("/../../shader/vertex.glsl");
    std::string f1 = relativePath.string() + std::string("/../../shader/fragment.glsl");

    // Create and compile our GLSL program from the shaders
    unsigned int programID = LoadShaders(v1.c_str(), f1.c_str()); // For off-screen render

    // Set up vertex data (and buffer(s)) and configure vertex attributes (for off-screen rendering)
    // Encode user input "color" as the pixel color of the four vertices
    float vertices[] = {
        // positions          // colors
        1.0f, 1.0f, 0.0f, color, color, color,   // top right
        1.0f, -1.0f, 0.0f, color, color, color,  // bottom right
        -1.0f, -1.0f, 0.0f, color, color, color, // bottom left
        -1.0f, 1.0f, 0.0f, color, color, color   // top left
    };
    unsigned int indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Framebuffer configuration for off-screen render
    unsigned int framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    // create a color attachment texture
    unsigned int textureColorbuffer;
    glGenTextures(1, &textureColorbuffer);
    // Bind the named texture textureColorbuffer to texturing target GL_TEXTURE_2D
    glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Off-screen render
    // Update texture entirely on GPU, very fast
    // Refer to this answer for more details: https://stackoverflow.com/a/10702468/1065190
    glUseProgram(programID);
    setInt(programID, "SCR_WIDTH", SCR_WIDTH);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // For logging LLC walk time
    uint64_t *reading_array = (uint64_t *)malloc(sizeof(uint64_t) * sample_counter);
    uint64_t reading_counter = 0;

    // Render loop
    std::cout << "start rendering" << std::endl;
    while (!glfwWindowShouldClose(window))
    {
        if (reading_counter >= sample_counter)
            break;

        // input
        processInput(window);

        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureColorbuffer);

        glUseProgram(programID);

        // Make sure every cacheline is brought into the cache
        while (1)
        {
            uint64_t llc_reading = measureOnce();
            if (abs((int)(llc_reading - cache_hit)) < 1000)
            {
                break;
            }
        }

        // Do off-screen rendering
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glFinish();

        // Measure LLC walk time
        uint64_t llc_reading = measureOnce();
        reading_array[reading_counter] = llc_reading;

        glBindTexture(GL_TEXTURE_2D, 0);

        // glBindFramebuffer(GL_FRAMEBUFFER, 0);
        //  glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();

        reading_counter++;
    }

    // Deallocate all resources once they've outlived their purpose:
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteFramebuffers(1, &framebuffer);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    glfwTerminate();

    // Save LLC walk times to a file
    reading_counter = 0;
    std::ofstream outputfile(filename);
    if (outputfile.is_open())
    {
        while (reading_counter < sample_counter)
        {
            outputfile << reading_array[reading_counter] << std::endl;
            reading_counter++;
        }
        outputfile.close();
    }
    free(reading_array);

    return 0;
}
