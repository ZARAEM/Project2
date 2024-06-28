/**
* Author: Mearaj Ahmed
* Assignment: Pong Clone
* Date due: 2024-06-29, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

enum AppStatus { RUNNING, TERMINATED };

// Game Window Size
constexpr int WINDOW_WIDTH = 640 * 2,
WINDOW_HEIGHT = 480 * 2;

// Background Color from 0-1
constexpr float BG_RED = (float)255 / 255,
BG_GREEN = (float)255 / 255,
BG_BLUE = (float)255 / 255,
BG_OPACITY = (float)1;

// Viewport (Basically the Camera)
constexpr int VIEWPORT_X = 0,
VIEWPORT_Y = 0,

VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

// Shader Path
constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

// Milliseconds in 1 Second
constexpr float MILLISECONDS_IN_SECOND = 1000.0;

// Number of Textures Generated
constexpr GLint NUMBER_OF_TEXTURES = 2,

/* Don't Pay Attention to these Two */
LEVEL_OF_DETAIL = 0,
TEXTURE_BORDER = 0;

// Path to Sprites
constexpr char PENGUIN_BLUE_FILEPATH[] = "assets/penguin_blue.png",
PUFFLE_ORANGE_FILEPATH[] = "assets/puffle_orange.png";

// Initial Position of Sprites
constexpr glm::vec3 INIT_POS_PENGUIN = glm::vec3(0.0f, 0.0f, 0.0f),
INIT_POS_PUFFLE = glm::vec3(0.5f, -0.25f, 0.0f);

constexpr glm::vec3 INIT_ROT_PENGUIN = glm::vec3(0.0f, 0.0f, 0.0f),
INIT_ROT_PUFFLE = glm::vec3(0.0f, 0.0f, 0.0f);

// Initial Scale of Sprites
constexpr glm::vec3 INIT_SCALE_PENGUIN = glm::vec3(1.0f, 1.0f, 1.0f),
INIT_SCALE_PUFFLE = glm::vec3(0.5f, 0.5, 0.5f);

constexpr float PENGUIN_TRANSLATION_INCREMENT = 0.1f,
PENGUIN_ROTATION_INCREMENT = 0.0f,
PENGUIN_SCALE_INCREMENT = 0.1f;

constexpr float PUFFLE_TRANSLATION_INCREMENT = 0.0f,
PUFFLE_ROTATION_INCREMENT = 0.3f,
PUFFLE_SCALE_INCREMENT = -0.05f;

SDL_Window* g_display_window;
AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program = ShaderProgram();

glm::mat4 g_view_matrix,
g_penguin_matrix,
g_puffle_matrix,
g_projection_matrix;

float g_previous_ticks = 0.0f;

glm::vec3 g_penguin_translation = glm::vec3(0.0f, 0.0f, 0.0f),
g_puffle_translation = glm::vec3(0.0f, 0.0f, 0.0f);

glm::vec3 g_penguin_rotation = glm::vec3(0.0f, 0.0f, 0.0f),
g_puffle_rotation = glm::vec3(0.0f, 0.0f, 0.0f);

glm::vec3 g_penguin_scale = glm::vec3(0.0f, 0.0f, 0.0f),
g_puffle_scale = glm::vec3(0.0f, 0.0f, 0.0f);

GLuint g_penguin_texture_id,
g_puffle_texture_id;

GLuint load_texture(const char* filepath);

void initialise();

void process_input();

void update();

void draw_object(glm::mat4& object_g_model_matrix, GLuint& object_texture_id);

void render();

void shutdown();

int main(int argc, char* argv[])
{
    initialise();

    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}

void initialise()
{
    // Initialise video and joystick subsystems
    SDL_Init(SDL_INIT_VIDEO);

    g_display_window = SDL_CreateWindow("CLUB PENGUIN ?",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

    if (g_display_window == nullptr) {
        std::cerr << "Error: SDL window could not be created.\n";
        SDL_Quit();
        exit(1);
    }

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_penguin_matrix = glm::mat4(1.0f);
    g_puffle_matrix = glm::mat4(1.0f);
    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_GREEN, BG_BLUE, BG_OPACITY);

    g_penguin_texture_id = load_texture(PENGUIN_BLUE_FILEPATH);
    g_puffle_texture_id = load_texture(PUFFLE_ORANGE_FILEPATH);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            g_app_status = TERMINATED;
            break;
        }
    }
}

void update()
{
    /* Delta time calculations */
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    /* Game logic */
    g_penguin_translation.x += PENGUIN_TRANSLATION_INCREMENT * delta_time;
    g_penguin_translation.y += PENGUIN_TRANSLATION_INCREMENT * delta_time;

    g_puffle_rotation.y += PUFFLE_ROTATION_INCREMENT * delta_time;

    g_penguin_scale += PENGUIN_SCALE_INCREMENT * delta_time;
    g_puffle_scale += PUFFLE_SCALE_INCREMENT * delta_time;

    g_penguin_matrix = glm::mat4(1.0f);
    g_puffle_matrix = glm::mat4(1.0f);

    g_penguin_matrix = glm::translate(g_penguin_matrix, INIT_POS_PENGUIN + g_penguin_translation);
    g_penguin_matrix = glm::rotate(g_penguin_matrix, g_penguin_rotation.y, glm::vec3(1.0f, 0.0f, 0.0f));
    g_penguin_matrix = glm::scale(g_penguin_matrix, INIT_SCALE_PENGUIN + g_penguin_scale);

    g_puffle_matrix = glm::translate(g_penguin_matrix, INIT_POS_PUFFLE + g_puffle_translation);
    g_puffle_matrix = glm::rotate(g_puffle_matrix, g_puffle_rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
    g_puffle_matrix = glm::scale(g_puffle_matrix, INIT_SCALE_PUFFLE + g_puffle_scale);

}

void draw_object(glm::mat4& object_g_model_matrix, GLuint& object_texture_id)
{
    g_shader_program.set_model_matrix(object_g_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so use 6, not 3
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    // Vertices
    float vertices[] =
    {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };

    // Textures
    float texture_coordinates[] =
    {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false,
        0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT,
        false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    // Bind texture
    draw_object(g_penguin_matrix, g_penguin_texture_id);
    draw_object(g_puffle_matrix, g_puffle_texture_id);

    // We disable two attribute arrays now
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() { SDL_Quit(); }

GLuint load_texture(const char* filepath) {
    // step 1: loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    // step 2: generating and binding a texture id to our image
    GLuint textureid;
    glGenTextures(1, &textureid);
    glBindTexture(GL_TEXTURE_2D, textureid);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    // step 3: setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // step 4: releasing our file from memory and returning our texture id
    stbi_image_free(image);

    return textureid;
}