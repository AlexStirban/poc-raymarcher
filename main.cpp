#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

// consts
const float screenWidth = 1024;
const float screenHeight = 768;
const float keyboardSpeed = 3.f;
const float mouseSpeed = 0.05f;

// Position vars
float yaw = 3.14f;
float pitch = 0.0f;
float mousePosX = 0.f;
float mousePosY = 0.f;
float FOV = 45.f;
glm::vec3 position = glm::vec3(0, 0, 5);
glm::vec3 direction (cos(pitch) * sin(yaw), sin(pitch), cos(pitch) * cos(yaw));
glm::vec3 right = glm::vec3(sin(yaw - 3.14f / 2.0f), 0, cos(yaw - 3.14f / 2.0f));
glm::vec3 up = glm::cross(right, up);
bool firstMouse = true;

// Time vars
double currentTime = 0.f;
double lastTime = 0.f;
float deltaTime = 0.f;


// Callback for errors
void error_callback(int error, const char* desc) {
    std::cout << "ERROR: " << desc << std::endl;
}

// Callback for controls key
static void keyboard_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        switch (key)
        {
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(window, GL_TRUE);
                
            default:
                break;
        }

    }
}


void processKeyboard(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        position += direction * deltaTime * keyboardSpeed;
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        position -= direction * deltaTime * keyboardSpeed;
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        position -= right * deltaTime * keyboardSpeed;
    }

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        position += right * deltaTime * keyboardSpeed;
    }
}


// Moving mouse callback
void mouse_callback(GLFWwindow* window, double x, double y) {
    // Prevent sudden skips when gaining focus

    if (!firstMouse) {
        yaw += mouseSpeed * deltaTime * (screenWidth / 2 - x);
        pitch += mouseSpeed * deltaTime * (screenHeight / 2 - y);
    }
    else {
        yaw += mouseSpeed * deltaTime * (screenWidth / 2);
        pitch += mouseSpeed * deltaTime * (screenHeight / 2);
        firstMouse = false;
    }


    direction = glm::vec3 (cos(pitch) * sin(yaw), sin(pitch), cos(pitch) * cos(yaw));
    right = glm::vec3(sin(yaw - 3.14f / 2.0f), 0, cos(yaw - 3.14f / 2.0f));
    up = glm::cross(right, direction);

    glfwSetCursorPos(window, screenWidth / 2, screenHeight / 2);
}

// Scrolling mouse whell callback
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    FOV -= 2 * yoffset;
    FOV = glm::clamp(FOV, 20.f, 90.f);
}

// Update pos and time
void updateTime() {
    lastTime = currentTime;
    currentTime = glfwGetTime();
    deltaTime = currentTime - lastTime;
}

// Quad
static const GLfloat vertices[] = {
    // Lower triangle
    -1.f, -1.f, 0.f,
     1.f, -1.f, 0.f,
    -1.f, 1.f, 0.f,
    -1.f, 1.f, 0.f,
     1.f, 1.f, 0.f,
     1.f, -1.f, 0.f
};


// Send vertices to the GPU
void loadVertices(GLuint& VAO, GLuint& VBO) {
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO); 
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);
}

// Load shaders from external files
GLuint loadShaders(const char* vertexPath, const char* fragmentPath) {
    GLuint vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    std::ifstream vertexShaderStream(vertexPath);
    std::ifstream fragmentShaderStream(fragmentPath);

    std::string vertexShaderCode;
    std::string fragmentShaderCode;

    // Load & compile vertex shader
    if (vertexShaderStream.is_open()) {
        std::stringstream ss;
        ss << vertexShaderStream.rdbuf();
        vertexShaderCode = ss.str();
        vertexShaderStream.close();
    }
    else {
        std::cout << "Couldn't open " << vertexPath << std::endl;
    }

    const char* vertexCodePtr = vertexShaderCode.c_str();
    glShaderSource(vertexShaderID, 1, &vertexCodePtr, NULL);
    glCompileShader(vertexShaderID);

    // Check vertex shader
    GLint res = GL_FALSE;
    int infoLength = 0;
    glGetShaderiv(vertexShaderID, GL_COMPILE_STATUS, &res);
    glGetShaderiv(vertexShaderID, GL_INFO_LOG_LENGTH, &infoLength);
    if (infoLength > 0) {
        std::vector<char> vertexShaderErrorMessage(infoLength + 1);
        glGetShaderInfoLog(vertexShaderID, infoLength, NULL, &vertexShaderErrorMessage[0]);
        printf("%s\n", &vertexShaderErrorMessage[0]);
    }

    // Load and compile fragment shader
    if (fragmentShaderStream.is_open()) {
        std::stringstream ss;
        ss << fragmentShaderStream.rdbuf();
        fragmentShaderCode = ss.str();
        fragmentShaderStream.close();
    }
    else {
        std::cout << "Couldn't open " << fragmentPath << std::endl;
    }

    const char* fragmentCodePtr = fragmentShaderCode.c_str();
    glShaderSource(fragmentShaderID, 1, &fragmentCodePtr, NULL);
    glCompileShader(fragmentShaderID);

    // Check fragment shader
    res = GL_FALSE;
    infoLength = 0;
    glGetShaderiv(fragmentShaderID, GL_COMPILE_STATUS, &res);
    glGetShaderiv(fragmentShaderID, GL_INFO_LOG_LENGTH, &infoLength);
    if (infoLength > 0) {
        std::vector<char> fragmentShaderErrorMessage(infoLength + 1);
        glGetShaderInfoLog(fragmentShaderID, infoLength, NULL, &fragmentShaderErrorMessage[0]);
        printf("%s\n", &fragmentShaderErrorMessage[0]);
    }


    GLuint programID = glCreateProgram();
    glAttachShader(programID, vertexShaderID);
    glAttachShader(programID, fragmentShaderID);
    glLinkProgram(programID);

    glDeleteShader(vertexShaderID);
    glDeleteShader(fragmentShaderID);

    return programID;
}


int main() {
    // Glew exprimental
    glewExperimental = true;

    // Init GLFW
    if (!glfwInit())
    {
        std::cout << "Error with GLFW" << std::endl;
        return -1;
    }

    // Error callback
    glfwSetErrorCallback(error_callback);

    //
    glfwWindowHint(GLFW_SAMPLES, 4);
    glEnable(GL_MULTISAMPLE);

    // Using OpenGL 3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Opening window and creating OpenGL context
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Raymarching", nullptr, nullptr);

    if (!window) {
        std::cout << "ERROR: " << "Failed to open GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);


    // Init GLEW
    glfwMakeContextCurrent(window);
    glewInit();
    glfwSwapInterval(1);
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    

    // Set controls callback
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetKeyCallback(window, keyboard_callback);
    glfwSetScrollCallback(window, scroll_callback);

    


    // Vertices and shaders
    GLuint VAO, VBO, programID;
    programID = loadShaders("vertex.hlsl", "menger.hlsl");
    loadVertices(VAO, VBO);

    // Inform shaders of data
    glUseProgram(programID);
    glUniform2f(glGetUniformLocation(programID, "resolution"), screenWidth, screenHeight);
    GLuint timeLocation = glGetUniformLocation(programID, "time");
    GLuint dirLocation = glGetUniformLocation(programID, "direction");
    GLuint upLocation = glGetUniformLocation(programID, "up");
    GLuint rightLocation = glGetUniformLocation(programID, "right");
    GLuint posLocation = glGetUniformLocation(programID, "position");
    GLuint FOVLocation = glGetUniformLocation(programID, "currentFOV");

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        // Drawing
        glUseProgram(programID);

        // Time elapsed
        updateTime();

        // Check keyboard
        processKeyboard(window);

        // Inform shader
        glUniform1f(timeLocation, glfwGetTime());
        glUniform1f(FOVLocation, FOV);
        glUniform3f(dirLocation, direction.x, direction.y, direction.z);
        glUniform3f(upLocation, up.x, up.y, up.z);
        glUniform3f(rightLocation, right.x, right.y, right.z);
        glUniform3f(posLocation, position.x, position.y, position.z);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
            
        // Events polling and buffer swapping
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}