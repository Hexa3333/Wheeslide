/* TODOS (maybe maybe)
    - Appropriate colliders
    - Better number to digits translation ?
    - Improve Tree Spawn
        - Add more positions
        - Fix impossible situations ?
    - Single VAO Digit Rendering ?
    
    (DONE) Render Score Text
    (DONE) Random Tree Spawn
   INFO
    - 99.999 is the limit score
        meaning it can only represent 5 digits at once
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

typedef unsigned int BufferArray, VertexBuffer;
typedef unsigned int Shader;
typedef unsigned int Sprite;

static const float G_ScreenWidth = 360.0f, G_ScreenHeight = 720.0f;
GLFWwindow* G_GlfwWindow;

Shader G_Shader;

unsigned int G_Score;
float G_GameSpeed = 0.5f;

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

    GLFWimage icon;
    icon.pixels = stbi_load("icon.png", &icon.width, &icon.height, NULL, 0);

    glfwSetWindowIcon(G_GlfwWindow, 1, &icon);

    stbi_image_free(icon.pixels);

    return 1;
}

int GameShouldRun()
{
    return !glfwWindowShouldClose(G_GlfwWindow);
}

/* Single shader; toggled on */
Shader SetShader()
{
    #ifdef WIN_RELEASE
        #define V_SHADER_SRC "shader.vert"
        #define F_SHADER_SRC "shader.frag"
    #else
        #define V_SHADER_SRC "src/shader.vert"
        #define F_SHADER_SRC "src/shader.frag"
    #endif

    /* Read the files into vertexSrc & fragSrc */
    FILE* fpVertexSrc = fopen(V_SHADER_SRC, "rb");
    FILE* fpFragSrc = fopen(F_SHADER_SRC, "rb");

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

float playerVertices[] = {
    -0.25f, -0.5f * G_ScreenWidth / G_ScreenHeight,      0.0f, 0.0f,   // bottom left
    0.25f,  -0.5f * G_ScreenWidth / G_ScreenHeight,      1.0f, 0.0f,   // bottom right
    0.25f,  0.5f  * G_ScreenWidth / G_ScreenHeight,      1.0f, 1.0f,   // top right
    -0.25f, 0.5f  * G_ScreenWidth / G_ScreenHeight,      0.0f, 1.0f   // top left
};

float squareVertices[] = {
    -0.5f, -0.5f * G_ScreenWidth / G_ScreenHeight,      0.0f, 0.0f,   // bottom left
    0.5f,  -0.5f * G_ScreenWidth / G_ScreenHeight,      1.0f, 0.0f,   // bottom right
    0.5f,  0.5f  * G_ScreenWidth / G_ScreenHeight,      1.0f, 1.0f,   // top right
    -0.5f, 0.5f  * G_ScreenWidth / G_ScreenHeight,      0.0f, 1.0f   // top left
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
    
    // Linear interpolation gives a thicker border so mmmhh
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

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

BufferArray CreateDrawBuffer(float* vertices, int sizeVertices, int* indices, int sizeIndices)
{
    BufferArray VAO; 
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

    return VAO;
}

struct Collider
{
    float xLeft, xRight,
          yTop, yBot;
};

struct GameObject
{
    vec3 position;
    BufferArray drawBuffer;
    Sprite sprite;
    struct Collider collider;
};

void DrawGameObject(struct GameObject go)
{
    glBindTexture(GL_TEXTURE_2D, go.sprite);

    static mat4 matrix;
    glm_mat4_identity(matrix); // reset matrix
    glm_translate(matrix, (float*) go.position);
    glUniformMatrix4fv(glGetUniformLocation(G_Shader, "transform"),
                       1, GL_FALSE, (float*) matrix);

    glBindVertexArray(go.drawBuffer);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

static float G_PlayerSpeed = 0.5f;

void GetRandomLoc(vec2 in)
{
    int r = (int) glfwGetTime();
    r %= 3;

    in[0] = (float)r-1;
    in[1] = -2.0f;
}

struct GameObject G_Trees[4];
float G_TreeCooldown[4];
void SpawnTree()
{
    static int treeNextIndex = 0; // loop 0,1,2,3,0,1,2,3

    vec2 loc;
    GetRandomLoc(loc);

    G_Trees[treeNextIndex].position[0] = loc[0];
    G_Trees[treeNextIndex].position[1] = loc[1];

    treeNextIndex++;
    treeNextIndex %= 4;
}

void GameOver()
{
    printf("GAME OVER!\n");
}

// HARDCODED
void GetNumberUVFromSpriteSheet(int num, float* in)
{
    in[0] = num * 0.1f;         in[1] = 0.0f;     // bottom left
    in[2] =(num * 0.1f) + 0.1f; in[3] = 0.0f;     // bottom right
    in[4] =(num * 0.1f) + 0.1f; in[5] = 1.0f;     // top right
    in[6] = num * 0.1f;         in[7] = 1.0f;     // top left
}

float G_NumberVertices[10][16];
void SetNumberUVs()
{
    float UV[8];
    for (int i = 0; i < 10; i++)
    {
        memcpy(G_NumberVertices[i], squareVertices, sizeof(squareVertices));
        GetNumberUVFromSpriteSheet(i, UV);
        G_NumberVertices[i][2] = UV[0];
        G_NumberVertices[i][3] = UV[1];

        G_NumberVertices[i][6] = UV[2];
        G_NumberVertices[i][7] = UV[3];

        G_NumberVertices[i][10] = UV[4];
        G_NumberVertices[i][11] = UV[5];

        G_NumberVertices[i][14] = UV[6];
        G_NumberVertices[i][15] = UV[7];
    }
}

/// @brief Converts a number to it's individual digits as an array
/// @param num The Score
/// @param in integer array of size 5 to write
void NumToDigitUnits(unsigned int num, int* in)
{
    in[0] = num / 10000;
    in[1] = (num / 1000) - (in[0]*10);
    in[2] = (num / 100) - (in[0]*100) - (in[1]*10);
    in[3] = (num / 10) - (in[0]*1000) - (in[1]*100) - (in[2]*10);
    in[4] = num - (in[0]*10000) - (in[1]*1000) - (in[2]*100) - (in[3]*10);
}

#ifdef WIN_RELEASE
#include <Windows.h>
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    PSTR lpCmdLine, int nCmdShow)
#else
int main(void)
#endif // WIN_RELEASE
{
    // * 360 / 780
    if (!SetWindow())
        return 1;

    stbi_set_flip_vertically_on_load(1);
    SetNumberUVs();
    G_Shader = SetShader();

    Sprite textSpr = SetSpritePNG("numbers.png");
    BufferArray numberBuffers[10];
    for (int i = 0; i < 10; i++)
    {
        numberBuffers[i] = CreateDrawBuffer(G_NumberVertices[i], sizeof(G_NumberVertices[i]), squareIndices, sizeof(squareIndices));
    }

    mat4 digitMatrices[5];
    float positionShift = 0.0f;
    for (int i = 0; i < 5; i++)
    {
        glm_mat4_identity(digitMatrices[i]);
        glm_scale(digitMatrices[i], (vec3){0.2f, 0.2f, 1.0f});

        vec3 digitPosition;
        digitPosition[0] = 0.5f + positionShift;
        digitPosition[1] = -2.0f;
        digitPosition[2] = 0.0f;

        glm_translate(digitMatrices[i], digitPosition);

        positionShift += 0.8f;
    }

    struct GameObject player = {
        { .0f, .7f, .0f },
        CreateDrawBuffer(playerVertices, sizeof(playerVertices), squareIndices, sizeof(squareIndices)),
        SetSpritePNG("player.png")
    };
    
    srand(time(0));
    for (int i = 0; i < 4; i++)
    {
        G_TreeCooldown[i] = 10.0f * (rand() % 10);
        G_Trees[i].position[0] = 0.0f;
        G_Trees[i].position[1] = 5.0f;
        G_Trees[i].drawBuffer = CreateDrawBuffer(squareVertices, sizeof(squareVertices), squareIndices, sizeof(squareIndices));
        G_Trees[i].sprite = SetSpritePNG("treeSprite.png");
    }

    float lastTime = 0;
    while (GameShouldRun())
    {
        float now = (float) glfwGetTime();
        float DT = now - lastTime;
        lastTime = glfwGetTime();

        // Collider update
        player.collider.xLeft = player.position[0] - 0.1f;
        player.collider.xRight = player.position[0] + 0.1f;
        player.collider.yTop = player.position[1] + 0.1f;
        player.collider.yBot = player.position[1] - 0.1f;

        // Snow White: (242, 240, 235)
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(F3_TO_RGB(242, 240, 235), 1.0f);
        glUseProgram(G_Shader);

        for (int i = 0; i < 4; i++)
        {
            vec2 pos = { G_Trees[i].position[0], G_Trees[i].position[1] };
            G_Trees[i].position[1] += G_GameSpeed * DT;

            // Collider update
            float xL = G_Trees[i].collider.xLeft  = pos[0] - 0.4f;
            float xR = G_Trees[i].collider.xRight = pos[0] + 0.4f;
            float yT = G_Trees[i].collider.yTop   = pos[1] + 0.4f;
            float yB = G_Trees[i].collider.yBot   = pos[1];

            // Collided! - Handle GO
            if (player.position[0] > xL && player.position[0] < xR &&
                player.position[1] > yB && player.position[1] < yT)
            {
                //GameOver();
                goto end;
            }

            // Out of bounds - Spawn new
            if (G_Trees[i].position[1] > 2.0f)
            {
                G_TreeCooldown[i] -= G_GameSpeed;
                if (G_TreeCooldown[i] < .0f)
                {
                    G_TreeCooldown[i] = 10.0f * (rand() % 10);
                    SpawnTree();
                }
            }

            DrawGameObject(G_Trees[i]);
        }

        DrawGameObject(player);

        //////////////////////////////////////////////////////
        // TEXT RENDERING - SCORE
        if (G_Score < 100000.0f - (5*G_GameSpeed))
            G_Score += 5 * G_GameSpeed;

        static int digits[5];
        NumToDigitUnits(G_Score, digits);

        for (int i = 0; i < 5; i++)
        {
            glUniformMatrix4fv(glGetUniformLocation(G_Shader, "transform"),
                    1, GL_FALSE, (float*) digitMatrices[i]);

            glBindVertexArray(numberBuffers[digits[i]]);
            glBindTexture(GL_TEXTURE_2D, textSpr);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }


        // SKIING
        if (glfwGetKey(G_GlfwWindow, GLFW_KEY_A) && player.position[0] > -.58f)
            player.position[0] -= G_PlayerSpeed * (G_ScreenHeight / G_ScreenWidth) * DT;
        if (glfwGetKey(G_GlfwWindow, GLFW_KEY_D) && player.position[0] < .68f)
            player.position[0] += G_PlayerSpeed * (G_ScreenHeight / G_ScreenWidth) * DT;
        if (glfwGetKey(G_GlfwWindow, GLFW_KEY_S))
            G_GameSpeed += 0.05f;
        if (glfwGetKey(G_GlfwWindow, GLFW_KEY_W) && G_GameSpeed > 0.3f)
            G_GameSpeed -= 0.05f;

        G_PlayerSpeed = (G_GameSpeed / 2);


        if (glfwGetKey(G_GlfwWindow, GLFW_KEY_ESCAPE))
            break;

        glfwSwapBuffers(G_GlfwWindow);
        glfwPollEvents();
    }
    end:

    

    glfwTerminate();
    return 0;
}