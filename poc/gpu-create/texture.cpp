// Code inspired by LearnOpenGL (Joey de Vries, twitter: https://twitter.com/JoeyDeVriez) https://github.com/JoeyDeVries/LearnOpenGL
// Used under CC BY-NC 4.0 license (https://creativecommons.org/licenses/by-nc/4.0/).
// Modifications made are integrating the original code with additional functionality.

#include "glad/glad.h"
#include "shader.hpp"
#include <GLFW/glfw3.h>
#include <filesystem>
#include <iostream>
#include <sys/time.h>
#include <inttypes.h>

int main(int argc, char *argv[])
{
    if (argc != 6)
    {
        fprintf(stderr, "Wrong Input! Enter: %s <color> <iter> <size> <read_write> <print-time> \n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // color: 0 Black, 1 Random
    // color: Any number bigger than 0 creates either Skew (color does not divide SCR_WIDTH) or Gradient (color divides SCR_WIDTH)
    // For every pixel on the screen: pixel.R = pixel.G = pixel.B = pixel.x%color (pixel.x is the coordinate of this pixel in the x direction)
    float color;
    sscanf(argv[1], "%f", &color);

    // The workload complexity (See Algorithm 1 in the paper)
    int iterations;
    sscanf(argv[2], "%d", &iterations);

    // SCR_WIDTH and SCR_HEIGHT define the dimension of the screen
    // By default, they are the same (the screen is a square)
    unsigned int SCR_WIDTH = 0;
    unsigned int SCR_HEIGHT = 0;
    sscanf(argv[3], "%d", &SCR_WIDTH);
    sscanf(argv[3], "%d", &SCR_HEIGHT);

    // 0: We ask the GPU to create a texture. We render the texture to screen for iterations in a loop
    // 1: We ask the GPU to create a texture for iterations in a loop
    // 2: We ask the GPU to create a texture and render the texture to screen for iterations in a loop
    int read_write;
    sscanf(argv[4], "%d", &read_write);

    // 0 does not print "Rendering time per frame"
    // 1 prints "Rendering time per frame" for "print_time" counts
    int print_time;
    sscanf(argv[5], "%d", &print_time);

    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // glfw window creation
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "GPU-CREATE", NULL, NULL);
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

    std::string v1 = relativePath.string() + std::string("/../../../shader/vertex.glsl");
    std::string f1 = relativePath.string() + std::string("/../../../shader/fragment.glsl");
    std::string v2 = relativePath.string() + std::string("/../../../shader/vertex_screen.glsl");
    std::string f2 = relativePath.string() + std::string("/../../../shader/fragment_screen.glsl");

    // Create and compile our GLSL program from the shaders
    unsigned int programID = LoadShaders(v1.c_str(), f1.c_str());        // For off-screen render
    unsigned int screen_programID = LoadShaders(v2.c_str(), f2.c_str()); // For render to screen

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

    // Vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates (for rendering to screen)
    float quadVertices[] = {
        // positions        // texCoords
        1.0f, 1.0f, 0.0f, 1.0f, 1.0f,   // top right
        1.0f, -1.0f, 0.0f, 1.0f, 0.0f,  // bottom right
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, // bottom left
        -1.0f, 1.0f, 0.0f, 0.0f, 1.0f   // top left
    };
    unsigned int quadIndices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };
    unsigned int quadVAO, quadVBO, quadEBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glGenBuffers(1, &quadEBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Framebuffer configuration for off-screen render
    unsigned int framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    // Create a color attachment texture
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

    if (read_write == 0)
    {
        glUseProgram(screen_programID);
        setInt(screen_programID, "textureColorbuffer", 0); // Set "textureColorbuffer" sampler to use Texture Unit 0
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    }
    else if (read_write == 1)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    }

    FILE *time;
    if (print_time)
    {
        char filename[200];
        sprintf(filename, "./time_%d_%d_%.1f_%d.txt", read_write, SCR_WIDTH, color, iterations);
        time = fopen((char *)filename, "w");
        if (time == NULL)
        {
            perror("output file");
        }
    }

    int counter = 0;
    uint64_t begin, end;

    // Render loop
    while (!glfwWindowShouldClose(window))
    {
        if (print_time != 0)
        {
            begin = get_time();
            fprintf(time, " %" PRIu64 " \n", begin);
        }

        processInput(window);

        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureColorbuffer);

        if (read_write == 0)
        { // read-only: Read textureColorbuffer out and render to the screen

            for (int i = 0; i < iterations; i++)
            {
                glBindVertexArray(quadVAO);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);
            }
        }
        else if (read_write == 1)
        { // write-only: GPU off-screen renders and writes to textureColorbuffer

            for (int i = 0; i < iterations; i++)
            {
                glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
                glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

                glBindVertexArray(VAO);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);
            }
        }
        else if (read_write == 2)
        { // write-read: Write to textureColorbuffer and then render to the screen

            for (int i = 0; i < iterations; i++)
            {
                glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
                glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
                glUseProgram(programID);
                setInt(programID, "SCR_WIDTH", SCR_WIDTH);
                glBindVertexArray(VAO);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);

                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
                glUseProgram(screen_programID);
                setInt(screen_programID, "textureColorbuffer", 0);
                glBindVertexArray(quadVAO);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);
            }
        }

        glBindTexture(GL_TEXTURE_2D, 0);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();

        if (print_time != 0)
        {
            end = get_time();
            fprintf(time, " %" PRIu64 " \n", end);
            counter = counter + 1;
            if (counter == print_time)
            {
                break;
            }
        }
    }

    // Deallocate all resources once they've outlived their purpose:
    glDeleteVertexArrays(1, &VAO);
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &quadVBO);
    glDeleteBuffers(1, &EBO);
    glDeleteFramebuffers(1, &framebuffer);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    glfwTerminate();
    return 0;
}
