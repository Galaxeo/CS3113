/**
* Author: Justin Cheok
* Assignment: Lunar Lander
* Date due: 2023-07-07, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/
#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1
#define FIXED_TIMESTEP 0.0166666f
#define PLATFORM_COUNT 6

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>
#include <vector>
#include "Entity.h"

// ––––– STRUCTS AND ENUMS ––––– //
struct GameState
{
    Entity* player;
    Entity* platforms;
};

// ––––– CONSTANTS ––––– //
const int WINDOW_WIDTH = 640,
WINDOW_HEIGHT = 480;

const float BG_RED = 0.1922f,
BG_BLUE = 0.549f,
BG_GREEN = 0.9059f,
BG_OPACITY = 1.0f;

const int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

const float MILLISECONDS_IN_SECOND = 1000.0;
const char BOAT_FILEPATH[] = "assets/boat.png";
const char PLATFORMgood_FILEPATH[] = "assets/mcIce.png";
const char PLATFORMbad_FILEPATH[] = "assets/mcLava.png";
const char font_SPRITE[] = "assets/font1.png";

const int FONTBANK_SIZE = 16;

const int NUMBER_OF_TEXTURES = 1;
const GLint LEVEL_OF_DETAIL = 0;
const GLint TEXTURE_BORDER = 0;

// ––––– GLOBAL VARIABLES ––––– //
GameState g_state;

SDL_Window* g_display_window;
bool g_game_is_running = true;

ShaderProgram g_program;
glm::mat4 g_view_matrix, g_projection_matrix;
ShaderProgram g_text_program;
GLuint        g_text_texture_id;

float g_previous_ticks = 0.0f;
float g_accumulator = 0.0f;
bool g_gameOver = false;
bool g_gameWon = false;
int g_direction_swap = 10; // serves as fuel
int g_direction = 0; // 0 for nothing, 1 for right, 2 for left

// ––––– GENERAL FUNCTIONS ––––– //
// Draw text function used from in-class exercise
void draw_text(ShaderProgram* program, GLuint font_texture_id, std::string text, float screen_size, float spacing, glm::vec3 position)
{
    // Scale the size of the fontbank in the UV-plane
    // We will use this for spacing and positioning
    float width = 1.0f / FONTBANK_SIZE;
    float height = 1.0f / FONTBANK_SIZE;

    // Instead of having a single pair of arrays, we'll have a series of pairs—one for each character
    // Don't forget to include <vector>!
    std::vector<float> vertices;
    std::vector<float> texture_coordinates;

    // For every character...
    for (int i = 0; i < text.size(); i++) {
        // 1. Get their index in the spritesheet, as well as their offset (i.e. their position
        //    relative to the whole sentence)
        int spritesheet_index = (int)text[i];  // asc   ii value of character
        float offset = (screen_size + spacing) * i;

        // 2. Using the spritesheet index, we can calculate our U- and V-coordinates
        float u_coordinate = (float)(spritesheet_index % FONTBANK_SIZE) / FONTBANK_SIZE;
        float v_coordinate = (float)(spritesheet_index / FONTBANK_SIZE) / FONTBANK_SIZE;

        // 3. Inset the current pair in both vectors
        vertices.insert(vertices.end(), {
            offset + (-0.5f * screen_size), 0.5f * screen_size,
            offset + (-0.5f * screen_size), -0.5f * screen_size,
            offset + (0.5f * screen_size), 0.5f * screen_size,
            offset + (0.5f * screen_size), -0.5f * screen_size,
            offset + (0.5f * screen_size), 0.5f * screen_size,
            offset + (-0.5f * screen_size), -0.5f * screen_size,
            });

        texture_coordinates.insert(texture_coordinates.end(), {
            u_coordinate, v_coordinate,
            u_coordinate, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate + width, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate, v_coordinate + height,
            });
    }
    // 4. And render all of them using the pairs
    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, position);

    program->SetModelMatrix(model_matrix);
    glUseProgram(program->programID);

    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices.data());
    glEnableVertexAttribArray(program->positionAttribute);
    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texture_coordinates.data());
    glEnableVertexAttribArray(program->texCoordAttribute);

    glBindTexture(GL_TEXTURE_2D, font_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, (int)(text.size() * 6));

    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);
}
GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(image);

    return textureID;
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Lunar Lander assignment",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_program.Load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_program.SetProjectionMatrix(g_projection_matrix);
    g_program.SetViewMatrix(g_view_matrix);

    glUseProgram(g_program.programID);

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    // ––––– PLATFORMS ––––– //
    GLuint platformBad_texture_id = load_texture(PLATFORMbad_FILEPATH);
    GLuint platformGood_texture_id = load_texture(PLATFORMgood_FILEPATH);

    g_state.platforms = new Entity[PLATFORM_COUNT];

    g_state.platforms[PLATFORM_COUNT - 1].m_texture_id = platformBad_texture_id;
    g_state.platforms[PLATFORM_COUNT - 1].set_position(glm::vec3(-2.0f, 0.65f, 0.0f));
    g_state.platforms[PLATFORM_COUNT - 1].set_width(0.4f);
    g_state.platforms[PLATFORM_COUNT - 1].update(0.0f, NULL, 0);

    for (int i = 0; i < PLATFORM_COUNT - 3; i++)
    {
        g_state.platforms[i].m_texture_id = platformBad_texture_id;
        g_state.platforms[i].set_position(glm::vec3(i + 1.0f, -1.0f, 0.0f));
        g_state.platforms[i].set_width(0.4f);
        g_state.platforms[i].update(0.0f, NULL, 0);
    }

    g_state.platforms[PLATFORM_COUNT - 2].m_texture_id = platformBad_texture_id;
    g_state.platforms[PLATFORM_COUNT - 2].set_position(glm::vec3(1.0f, 2.0f, 0.0f));
    g_state.platforms[PLATFORM_COUNT - 2].set_width(0.4f);
    g_state.platforms[PLATFORM_COUNT - 2].update(0.0f, NULL, 0);

    g_state.platforms[PLATFORM_COUNT - 3].m_texture_id = platformGood_texture_id;
    g_state.platforms[PLATFORM_COUNT - 3].set_position(glm::vec3(1.0f, -3.5f, 0.0f));
    g_state.platforms[PLATFORM_COUNT - 3].set_width(0.4f);
    g_state.platforms[PLATFORM_COUNT - 3].update(0.0f, NULL, 0);
    g_state.platforms[PLATFORM_COUNT - 3].set_goal(true);

    // ––––– BOAT ––––– //
    // Existing
    g_state.player = new Entity();
    g_state.player->set_position(glm::vec3(0.0f, 2.5f, 0.0f));
    g_state.player->set_movement(glm::vec3(0.0f));
    g_state.player->m_speed = 1.0f;
    g_state.player->set_acceleration(glm::vec3(0.0f, -0.5f, 0.0f));
    g_state.player->m_texture_id = load_texture(BOAT_FILEPATH);

    // Jumping
    g_state.player->m_jumping_power = 3.0f;

    // -- TEXT -- //
    g_text_program.Load(V_SHADER_PATH, F_SHADER_PATH);
    g_text_program.SetProjectionMatrix(g_projection_matrix);
    g_text_program.SetViewMatrix(g_view_matrix);

    glUseProgram(g_text_program.programID);
    g_text_texture_id = load_texture(font_SPRITE);

    // ––––– GENERAL ––––– //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    g_state.player->set_movement(glm::vec3(0.0f));

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
            // End game
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            g_game_is_running = false;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_q:
                // Quit the game with a keystroke
                g_game_is_running = false;
                break;

            default:
                break;
            }

        default:
            break;
        }
    }

    const Uint8* key_state = SDL_GetKeyboardState(NULL);

    if ((!g_gameOver && !g_gameWon) && g_direction_swap > 0) {
        if (key_state[SDL_SCANCODE_LEFT])
        {
            g_state.player->set_acceleration(glm::vec3(-25.0f, 0.0f, 0.0f));
            g_state.player->m_animation_indices = g_state.player->m_walking[g_state.player->LEFT];
            if (g_direction != 1) {
                g_direction_swap -= 1;
                g_direction = 1;
            }
            
        }
        else if (key_state[SDL_SCANCODE_RIGHT])
        {
            g_state.player->set_acceleration(glm::vec3(25.0f, 0.0f, 0.0f));
            g_state.player->m_animation_indices = g_state.player->m_walking[g_state.player->RIGHT];
            if (g_direction != 2) {
                g_direction_swap -= 1;
                g_direction = 2;
            }
        }

        if (glm::length(g_state.player->m_movement) > 1.0f)
        {
            g_state.player->m_movement = glm::normalize(g_state.player->m_movement);
        }
    }
    else {
        if (key_state[SDL_SCANCODE_ESCAPE])
        {
            g_game_is_running = false;
        }
    }

    
}

void update()
{
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    delta_time += g_accumulator;

    if (delta_time < FIXED_TIMESTEP)
    {
        g_accumulator = delta_time;
        return;
    }

    while (delta_time >= FIXED_TIMESTEP)
    {
        g_state.player->update(FIXED_TIMESTEP, g_state.platforms, PLATFORM_COUNT);
        delta_time -= FIXED_TIMESTEP;
    }

    if (g_state.player->m_collided_bottom || g_state.player->m_collided_left || g_state.player->m_collided_right || g_state.player->m_collided_top) {
        g_state.player->set_movement(glm::vec3(0.0f));
        g_state.player->set_acceleration(glm::vec3(0.0f));
    }

    g_accumulator = delta_time;

    if (g_state.player->get_position().y <= -3.5f) {
        g_state.player->set_movement(glm::vec3(0.0f));
        g_state.player->set_acceleration(glm::vec3(0.0f));
    }

}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    g_state.player->render(&g_program);

    if (g_game_is_running) {
        draw_text(&g_text_program, g_text_texture_id, std::string("Remaining Fuel: " + std::to_string(g_direction_swap)), 0.25f, 0.0f, glm::vec3(-2.5f, 3.0f, 0.0f));
    }
    if (g_state.player->m_collided_bottom || g_state.player->m_collided_left || g_state.player->m_collided_right || g_state.player->m_collided_top) {
        if (g_state.player->m_finish) {
            g_gameWon = true;
        }
        else {
            g_gameOver = true;
        }
    }
    if (g_state.player->get_position().y <= -3.5f) {
        g_gameOver = true;
    }
    
    if (g_gameOver) {
        draw_text(&g_text_program, g_text_texture_id, std::string("Mission Failed!"), 0.25f, 0.0f, glm::vec3(-1.0f, 0.5f, 0.0f));
        draw_text(&g_text_program, g_text_texture_id, std::string("Press ESC/Q to exit"), 0.25f, 0.0f, glm::vec3(-1.5f, 0.0f, 0.0f));
    }
    else if (g_gameWon) {
        draw_text(&g_text_program, g_text_texture_id, std::string("Mission Accomplished!"), 0.25f, 0.0f, glm::vec3(-1.0f, 0.5f, 0.0f));
        draw_text(&g_text_program, g_text_texture_id, std::string("Press ESC/Q to exit"), 0.25f, 0.0f, glm::vec3(-1.5f, 0.0f, 0.0f));
    }
    for (int i = 0; i < PLATFORM_COUNT; i++) g_state.platforms[i].render(&g_program);

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown()
{
    SDL_Quit();

    delete[] g_state.platforms;
    delete g_state.player;
}

// ––––– GAME LOOP ––––– //
int main(int argc, char* argv[])
{
    initialise();

    while (g_game_is_running)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}