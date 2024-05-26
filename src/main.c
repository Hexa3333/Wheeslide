/* TODOS
    - Fix sprite UV
    
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include "cglm/vec2.h"
#include "cglm/mat3.h"
#include "cglm/affine.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define DEBUG 1
#define F3_TO_RGB(a, b, c) a / 255.0f, b / 255.0f, c / 255.0f 

typedef unsigned int DrawBuffer, VertexBuffer;
typedef unsigned int Shader;
typedef unsigned int Sprite;

static const float G_ScreenWidth = 360.0f,
                   G_ScreenHeight = 720.0f;
GLFWwindow* G_GlfwWindow;
Shader G_Shader;
float G_GameSpeed;

int SetWindow()
{
    if (!glfwInit())
    {
        fprintf(stderr, "Glfw initialisation failed.");
        return 0;
    }

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    G_GlfwWindow = glfwCreateWindow((int)G_ScreenWidth, G_ScreenHeight,
                                    "Wheeslide", NULL, NULL);
    if (!G_GlfwWindow)
    {
        glfwTerminate();
        return 0;
    }

    glfwMakeContextCurrent(G_GlfwWindow);
    gladLoadGL();

    return 1;
}

int GameShouldRun()
{
    return !glfwWindowShouldClose(G_GlfwWindow);
}


/* Single shader; toggled on */
Shader SetShader()
{
    /* Read the files into vertexSrc & fragSrc */
    FILE* fpVertexSrc = fopen("src/shader.vert", "rb");
    FILE* fpFragSrc = fopen("src/shader.frag", "rb");

    if (!fpVertexSrc || !fpFragSrc)
    {
        fprintf(stderr, "Error reading shader source!\nMake sure...");
        return 0;
    }

    fseek(fpVertexSrc, 0, SEEK_END);
    fseek(fpFragSrc, 0, SEEK_END);

    size_t lenVertexSrc = ftell(fpVertexSrc);
    size_t lenFragSrc = ftell(fpFragSrc);
    
    fseek(fpVertexSrc, 0, SEEK_SET);
    fseek(fpFragSrc, 0, SEEK_SET);

    char* vertexSrc = malloc(lenVertexSrc+1);
    char* fragSrc = malloc(lenFragSrc+1);

    fread(vertexSrc, 1, lenVertexSrc, fpVertexSrc);
    fread(fragSrc, 1, lenFragSrc, fpFragSrc);
    vertexSrc[lenVertexSrc] = 0;
    fragSrc[lenFragSrc] = 0;
    
    fclose(fpVertexSrc);
    fclose(fpFragSrc);

    /* vertexSrc & fragSrc are the actual shader code */
    unsigned int shaderProgram = glCreateProgram();
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(vertexShader, 1, (const char**) &vertexSrc, NULL);
    glShaderSource(fragmentShader, 1, (const char**) &fragSrc, NULL);

    free(vertexSrc);
    free(fragSrc);

    glCompileShader(vertexShader);
    glCompileShader(fragmentShader);

    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    #ifdef DEBUG
    char infoLog[256];
    int infoLen;
    int success;

    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 256, &infoLen, infoLog);
        infoLog[infoLen] = 0;
        fprintf(stderr, "%s\n", infoLog);
    }

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 256, &infoLen, infoLog);
        infoLog[infoLen] = 0;
        fprintf(stderr, "%s\n", infoLog);
    }

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 256, &infoLen, infoLog);
        infoLog[infoLen] = 0;
        fprintf(stderr, "%s\n", infoLog);
    }
    #endif

    glUseProgram(shaderProgram);
    return shaderProgram;
}

void UseShader(Shader shader)
{
    glUseProgram(shader);
}

float squareVertices[] = {
    -0.5f, -0.5f * G_ScreenWidth / G_ScreenHeight,      0.0f, 0.0f,   // bottom left
    0.5f,  -0.5f * G_ScreenWidth / G_ScreenHeight,      0.99f, 0.0f,   // bottom right
    0.5f,  0.5f  * G_ScreenWidth / G_ScreenHeight,      0.99f, 0.99f,   // top right
    -0.5f, 0.5f  * G_ScreenWidth / G_ScreenHeight,      0.0f, 0.99f   // top left
};
unsigned int squareIndices[] = {
    1, 2, 3,
    0, 1, 3
};

Sprite SetSpritePNG(const char* fp)
{
    Sprite sprite;
    glGenTextures(1, &sprite);
    glBindTexture(GL_TEXTURE_2D, sprite);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    int width, height, comp;
    unsigned char* data = stbi_load(fp, &width, &height, &comp, 0);  
    if (!data)
    {
        fprintf(stderr, "Failed to read image");
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);

    return sprite;
}

void UseSprite(Sprite sprite)
{
    glBindTexture(GL_TEXTURE_2D, sprite);
}

void DeleteSprite(Sprite* sprite)
{
    glDeleteTextures(1, sprite);
}


DrawBuffer CreateDrawBuffer(float* vertices, int sizeVertices, int* indices, int sizeIndices)
{
    DrawBuffer VAO; 
    VertexBuffer VBO, IBO;

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &IBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);

    glBufferData(GL_ARRAY_BUFFER, sizeVertices, vertices, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeIndices, indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

void BindDrawBuffer(DrawBuffer vArray)
{
    glBindVertexArray(vArray);
}

struct GameObject
{
    vec3 position;
    DrawBuffer drawBuffer;
    Sprite sprite;
    /*
        Collider
    */
};

struct _Player
{
    struct GameObject go;
    float speed;
    unsigned int score;
} G_Player;

void DrawGameObject(struct GameObject go)
{
    UseSprite(go.sprite);

    static mat4 matrix;
    glm_mat4_identity(matrix); // reset matrix
    glm_translate(matrix, (float*) go.position);
    glUniformMatrix4fv(glGetUniformLocation(G_Shader, "transform"),
                       1, GL_FALSE, (float*) matrix);

    BindDrawBuffer(go.drawBuffer);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

static float playerSpeed = 0.5f;

/* TODO
    Spawn trees
*/

struct GameObject Trees[4];
int treeNextIndex = 0; // loop 0,1,2,3,0,1,2,3

float GetRandomPosition();

int main(void)
{
    // * 360 / 780
    if (!SetWindow())
        return 1;

    stbi_set_flip_vertically_on_load(1);
    G_Shader = SetShader();
    G_GameSpeed = 1.0f;

    struct GameObject player = {
        { .0f, .7f, .0f },
        CreateDrawBuffer(squareVertices, sizeof(squareVertices), squareIndices, sizeof(squareIndices)),
        SetSpritePNG("player.png")
    };

    // pos: {rando, -2.0f, 0 }
    struct GameObject tree = {
        { 0, -1.0f, 0 },
        CreateDrawBuffer(squareVertices, sizeof(squareVertices), squareIndices, sizeof(squareIndices)),
        SetSpritePNG("treeSprite.png")
    };

    float lastTime = 0;
    while (GameShouldRun())
    {
        float now = (float) glfwGetTime();
        float DT = now - lastTime;
        lastTime = glfwGetTime();
        
        // Snow White: (242, 240, 235)
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(F3_TO_RGB(242, 240, 235), 1.0f);
        UseShader(G_Shader);

        DrawGameObject(player);
        DrawGameObject(tree);

        //for (int i = 0; i < 4; i++)
        //    DrawGameObject(Trees[i]);


        if (glfwGetKey(G_GlfwWindow, GLFW_KEY_A) && player.position[0] > -1.f)
            player.position[0] -= playerSpeed * (G_ScreenHeight / G_ScreenWidth) * DT;
        if (glfwGetKey(G_GlfwWindow, GLFW_KEY_D) && player.position[0] < 1.f)
            player.position[0] += playerSpeed * (G_ScreenHeight / G_ScreenWidth) * DT;

        if (glfwGetKey(G_GlfwWindow, GLFW_KEY_ESCAPE))
            break;

        glfwSwapBuffers(G_GlfwWindow);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}