#include "PreviewWindow.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <thread>
#include <iostream>
#include <variant>

const std::string VERT_SHADER = R"(
                                    #version 330 core
                                    layout (location = 0) in vec3 aPos;

                                    out vec2 texCoord;

                                    void main(){
                                        gl_Position = vec4(aPos, 1.0);
                                        texCoord = vec2((aPos.x+1.0)/2.0, 1.0 - ((aPos.y+1.0)/2.0));
                                    }
                                )";
const std::string FRAG_SHADER = R"(
                                    #version 330 core
                                    out vec3 FragColor;
                                    in vec2 texCoord;

                                    uniform sampler2D tex;

                                    void main(){
                                        FragColor = texture(tex, texCoord).rgb;
                                        FragColor = pow(FragColor, vec3(1/2.2, 1/2.2, 1/2.2));
                                    }
                                )";

PreviewWindow::PreviewWindow(std::shared_ptr<FrameBuffer> frameBuffer)
    : img(std::move(frameBuffer))
{ }

std::variant<unsigned int, std::string> compileShader(const std::string& source, GLenum type)
{
    auto shaderId = glCreateShader(type);
    const char* shaderSrcPtr = source.c_str();
    glShaderSource(shaderId, 1, &shaderSrcPtr, nullptr);
    glCompileShader(shaderId);

    int success;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(shaderId, sizeof(infoLog), nullptr, infoLog);
        return std::string(infoLog);
    };
    return shaderId;
}

void PreviewWindow::show()
{
    //Initialize the library
    if (!glfwInit()) {
        return;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    //Create a windowed mode window and its OpenGL context
    float aspectRatio= static_cast<float>(this->img->getVerticalResolution()) / static_cast<float>(this->img->getHorizontalResolution());
    float windowWidth = 1280.0f;
    float windowHeight = windowWidth*aspectRatio;
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Preview", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    {
        return;
    }

    // Generate vertex array
    GLuint vaId;
    glGenVertexArrays(1, &vaId);
    glBindVertexArray(vaId);
    GLuint vbId;
    glGenBuffers(1, &vbId);
    glBindBuffer(GL_ARRAY_BUFFER, vbId);
    static const GLfloat vertexData[] = {
        -1.0f, -1.0f, 0.0f,
         3.0f, -1.0f, 0.0f,
        -1.0f,  3.0f, 0.0f,
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);
    glVertexAttribPointer(
        0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
        3,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalized?
        3*sizeof(float),    // stride
        nullptr             // array buffer offset
    );
    glEnableVertexAttribArray(0);

    // Create texture
    GLuint tex;
    glGenTextures(1, &tex);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, this->img->getHorizontalResolution(), this->img->getVerticalResolution(), 0, GL_RGB, GL_FLOAT, &this->img->getData().at(0));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


    // Load shader
    auto result = compileShader(VERT_SHADER, GL_VERTEX_SHADER);
    if(std::holds_alternative<std::string>(result))
    {
        const auto& error = std::get<std::string>(result);
        std::cerr << "Vertex shader compilation failed:" << std::endl << error << std::endl;
        return; //TODO: gltf cleanup
    }
    auto vertexShaderId = std::get<unsigned int>(result);

    result = compileShader(FRAG_SHADER, GL_FRAGMENT_SHADER);
    if(std::holds_alternative<std::string>(result))
    {
        const auto& error = std::get<std::string>(result);
        std::cerr << "Fragment shader compilation failed:" << std::endl << error << std::endl;
        return;
    }
    auto fragmentShaderId = std::get<unsigned int>(result);

    auto shaderProgramId = glCreateProgram();
    glAttachShader(shaderProgramId, vertexShaderId);
    glAttachShader(shaderProgramId, fragmentShaderId);
    glLinkProgram(shaderProgramId);

    int success;
    glGetProgramiv(shaderProgramId, GL_LINK_STATUS, &success);
    if(!success) {

        char infoLog[512];
        glGetShaderInfoLog(shaderProgramId, sizeof(infoLog), nullptr, infoLog);
        std::cerr << "Shader linking failed: " << std::endl << infoLog << std::endl;
        return;
    }

    // Main UI loop
    while (!glfwWindowShouldClose(window))
    {
        auto start = std::chrono::high_resolution_clock::now();

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, this->img->getHorizontalResolution(), this->img->getVerticalResolution(), 0, GL_RGB, GL_FLOAT, &this->img->getData().at(0));

        glUseProgram(shaderProgramId);
        glBindVertexArray(vaId);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        //this->img;

        //Swap front and back buffers
        glfwSwapBuffers(window);

        //Poll for and process events
        glfwPollEvents();

        auto end = std::chrono::high_resolution_clock::now();
        auto renderTime = end - start;
        // update every 100ms
        auto sleepTime = std::chrono::milliseconds(100) - renderTime;
        if(sleepTime.count() > 0){
            std::this_thread::sleep_for(sleepTime);
        }
    }

    glDeleteTextures(1, &tex);

    glDeleteProgram(shaderProgramId);
    glDeleteShader(fragmentShaderId);
    glDeleteShader(vertexShaderId);


    glDeleteBuffers(1, &vbId);
    glDeleteVertexArrays(1, &vaId);

    glfwTerminate();
}

void PreviewWindow::showAsync()
{
    if(uiThread != nullptr)
    {
        throw std::runtime_error("The window already has an associated UI thread.");
    }

    uiThread = std::make_unique<std::thread>(&PreviewWindow::show, this);
}

void PreviewWindow::wait()
{
    if(uiThread != nullptr)
    {
        uiThread->join();
    }
}
