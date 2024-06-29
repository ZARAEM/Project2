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
#define GL_GLEXT_PROTOTYPES 1
#define LOG(argument) std::cout << argument << '\n'

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
constexpr int 
WINDOW_WIDTH = 640 * 2,
WINDOW_HEIGHT = 480 * 2;

// Background Color from 0-1
// #76b6c4 oceanish color I think
constexpr float 
BG_RED = (float) 118 / 255,
BG_GREEN = (float) 182 / 255,
BG_BLUE = (float) 196 / 255,
BG_OPACITY = 1.0f;

// Viewport (Basically the Camera)
constexpr int 
VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

// Shader Path
constexpr char 
V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

// Milliseconds in 1 Second
constexpr float MILLISECONDS_IN_SECOND = 1000.0f;

// Number of Textures Generated
constexpr GLint NUMBER_OF_TEXTURES = 2,

/* Don't Pay Attention to these Two */
LEVEL_OF_DETAIL = 0,
TEXTURE_BORDER = 0;

// Path to Sprites
//Credit: https://kenney.nl/
//Recolored to red and blue 
constexpr char
SHIPONE_FILEPATH[] = "assets/shipOne.png",
SHIPTWO_FILEPATH[] = "assets/shipTwo.png",
CANNONBALL_FILEPATH[] = "assets/cannonBall.png";

// Initial Position
constexpr glm::vec3 INIT_POS_SHIPONE = glm::vec3(-4.0f, 0.0f, 0.0f),
INIT_POS_SHIPTWO = glm::vec3(4.0f, 0.0f, 0.0f);

//constexpr glm::vec3 INIT_ROT_SHIPONE = glm::vec3(0.0f, 0.0f, 0.0f),
//INIT_ROT_SHIPTWO = glm::vec3(0.0f, 0.0f, 0.0f);

// Initial Scale
constexpr glm::vec3 
INIT_SCALE_SHIPONE = glm::vec3(66.0/128.0f, 113.0/128.0f, 0.0f),
INIT_SCALE_SHIPTWO = glm::vec3(66.0/128.0f, 113.0/128.0f, 0.0f);

SDL_Window* g_display_window;
AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program = ShaderProgram();

glm::mat4 g_view_matrix,
g_shipOne_matrix,
g_shipTwo_matrix,
g_projection_matrix;

float g_previous_ticks = 0.0f;

glm::vec3
g_shipOne_position = glm::vec3(0.0f, 0.0f, 0.0f),
g_shipOne_movement = glm::vec3(0.0f, 0.0f, 0.0f),

g_shipTwo_position = glm::vec3(0.0f, 0.0f, 0.0f),
g_shipTwo_movement = glm::vec3(0.0f, 0.0f, 0.0f);

float 
g_shipOne_speed = 1.0f,
g_shipTwo_speed = 1.0f;

glm::vec3 g_shipOne_rotation = glm::vec3(0.0f, 0.0f, 0.0f),
g_shipTwo_rotation = glm::vec3(0.0f, 0.0f, 0.0f);

glm::vec3 g_shipOne_scale = glm::vec3(0.0f, 0.0f, 0.0f),
g_shipTwo_scale = glm::vec3(0.0f, 0.0f, 0.0f);

GLuint g_shipOne_texture_id,
g_shipTwo_texture_id;

bool g_twoPlayerMode = true;

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

void initialise()
{
    // Initialise video and joystick subsystems
    SDL_Init(SDL_INIT_VIDEO);

    g_display_window = SDL_CreateWindow("SHIP PONG ?",
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

    g_shipOne_matrix = glm::mat4(1.0f);
    g_shipTwo_matrix = glm::mat4(1.0f);

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_GREEN, BG_BLUE, BG_OPACITY);

    g_shipOne_texture_id = load_texture(SHIPONE_FILEPATH);
    g_shipTwo_texture_id = load_texture(SHIPTWO_FILEPATH);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    g_shipOne_movement = glm::vec3(0.0f);
    g_shipTwo_movement = glm::vec3(0.0f);

    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        switch (event.type) {

        // End Game
        case SDL_QUIT:
            g_app_status = TERMINATED;
            break;
        case SDL_WINDOWEVENT_CLOSE:
            g_app_status = TERMINATED;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_t:
                g_twoPlayerMode = !g_twoPlayerMode;
                break;

            default:
                break;
            }
        default:
            break;
        }
    }

    const Uint8* key_state = SDL_GetKeyboardState(NULL);

    if (key_state[SDL_SCANCODE_W] && key_state[SDL_SCANCODE_S]) {
        if (g_twoPlayerMode) {
            g_shipOne_movement.y = 0.0f;
        }
        else {
            g_shipOne_movement.y = 0.0f;
        }
    }

    else if (key_state[SDL_SCANCODE_W] && !key_state[SDL_SCANCODE_S]) {
        if (g_twoPlayerMode) {
            g_shipOne_movement.y = 1.0f;
        }
        else {
            g_shipOne_movement.y = 0.0f;
        }
    }

    else if (key_state[SDL_SCANCODE_S] && !key_state[SDL_SCANCODE_W]) {
        if (g_twoPlayerMode) {
            g_shipOne_movement.y = -1.0f;
        }
        else {
            g_shipOne_movement.y = 0.0f;
        }
    }

    if (key_state[SDL_SCANCODE_UP] && key_state[SDL_SCANCODE_DOWN]) {
        g_shipTwo_movement.y = 0.0f;
    }

    else if (key_state[SDL_SCANCODE_UP] && !key_state[SDL_SCANCODE_DOWN]) {
        g_shipTwo_movement.y = 1.0f;
    }

    else if (key_state[SDL_SCANCODE_DOWN] && !key_state[SDL_SCANCODE_UP]) {
        g_shipTwo_movement.y = -1.0f;
    }

    if (glm::length(g_shipOne_movement) > 1.0f) {
        g_shipOne_movement = glm::normalize(g_shipOne_movement);
    }

    if (glm::length(g_shipTwo_movement) > 1.0f) {
        g_shipTwo_movement = glm::normalize(g_shipTwo_movement);
    }
}

void update()
{
    /* Delta time calculations */
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    g_shipOne_position += g_shipOne_movement * g_shipOne_speed * delta_time;
    g_shipTwo_position += g_shipTwo_movement * g_shipTwo_speed * delta_time;

    // Game Logic
    g_shipOne_matrix = glm::mat4(1.0f); // Reset
    g_shipOne_matrix = glm::translate(g_shipOne_matrix, INIT_POS_SHIPONE); // xPos
    g_shipOne_matrix = glm::translate(g_shipOne_matrix, g_shipOne_position); // yPos

    g_shipTwo_matrix = glm::mat4(1.0f); // Reset
    g_shipTwo_matrix = glm::translate(g_shipTwo_matrix, INIT_POS_SHIPTWO); // xPos
    g_shipTwo_matrix = glm::translate(g_shipTwo_matrix, g_shipTwo_position); // yPos

    /* Top and Bottom of Screen */
    const float TOP = 3.75f;
    const float BOTTOM = -1 * TOP;

    g_shipOne_matrix = glm::scale(g_shipOne_matrix, INIT_SCALE_SHIPONE);
    g_shipTwo_matrix = glm::scale(g_shipTwo_matrix, INIT_SCALE_SHIPTWO);

    // Top and bottom calculated by taking current position and adding/subtractng half of size of ship

    /* Top of shipOne */
    float shipOne_top = g_shipOne_position.y + INIT_POS_SHIPONE.y + (INIT_SCALE_SHIPONE.y / 2.0f);
    
    /* Bottom of shipOne */
    float shipOne_bottom = g_shipOne_position.y + INIT_POS_SHIPONE.y - (INIT_SCALE_SHIPONE.y / 2.0f);

    /* Top of shipTwo */
    float shipTwo_top = g_shipTwo_position.y + INIT_POS_SHIPTWO.y + (INIT_SCALE_SHIPTWO.y / 2.0f);
    
    /* Bottom of shipTwo */
    float shipTwo_bottom = g_shipTwo_position.y + INIT_POS_SHIPTWO.y - (INIT_SCALE_SHIPTWO.y / 2.0f);

    // If ships are above or below top or bottom respectively, move them to topmost/bottomost position
    
    if (shipOne_top > TOP) {
        g_shipOne_position.y = TOP - INIT_POS_SHIPONE.y - (INIT_SCALE_SHIPONE.y / 2.0f);
    }

    if (shipOne_bottom < BOTTOM) {
        g_shipOne_position.y = BOTTOM - INIT_POS_SHIPONE.y + (INIT_SCALE_SHIPONE.y / 2.0f);
    }

    if (shipTwo_top > TOP) {
        g_shipTwo_position.y = TOP - INIT_POS_SHIPTWO.y - (INIT_SCALE_SHIPTWO.y / 2.0f);
    }

    if (shipTwo_bottom < BOTTOM) {
        g_shipTwo_position.y = BOTTOM - INIT_POS_SHIPTWO.y + (INIT_SCALE_SHIPTWO.y / 2.0f);
    }

}

void draw_object(glm::mat4& object_g_model_matrix, GLuint& object_texture_id)
{
    g_shader_program.set_model_matrix(object_g_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);
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
    draw_object(g_shipOne_matrix, g_shipOne_texture_id);
    draw_object(g_shipTwo_matrix, g_shipTwo_texture_id);

    // We disable two attribute arrays now
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() { SDL_Quit(); }