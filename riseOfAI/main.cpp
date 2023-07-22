/**
* Author: Justin Cheok
* Assignment: Rise of the AI
* Date due: 2023-07-21, 11:59pm
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
#define ENEMY_COUNT 3
#define LEVEL1_WIDTH 20
#define LEVEL1_HEIGHT 5

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL_mixer.h>
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
#include "Map.h"

// ––––– STRUCTS AND ENUMS ––––– //
struct GameState
{
    Entity *player;
    Entity *enemies;
    
    Map *map;
    
    Mix_Chunk *jump_sfx;
    Mix_Chunk* win_sfx;
    Mix_Chunk* lose_sfx;
    Mix_Chunk* pounce_sfx;
};

// ––––– CONSTANTS ––––– //
const int WINDOW_WIDTH  = 640,
          WINDOW_HEIGHT = 480;

const float BG_RED = 0.1922f,
            BG_BLUE = 0.549f,
            BG_GREEN = 0.9059f,
            BG_OPACITY = 1.0f;

const int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

const float MILLISECONDS_IN_SECOND = 1000.0;

const char SPRITESHEET_FILEPATH[] = "assets/Cat Sprite Sheet.png";
const char FONT_FILEPATH[] = "assets/font1.png";

const int NUMBER_OF_TEXTURES = 1;
const GLint LEVEL_OF_DETAIL  = 0;
const GLint TEXTURE_BORDER   = 0;

unsigned int LEVEL_1_DATA[] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 34, 35, 36, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 17, 0,
    0, 0, 0, 0, 29, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,16, 17, 0, 0, 1,
    1, 2, 2, 2, 3, 0, 0, 1, 2, 2, 3, 0, 0, 1, 2, 2, 2, 2, 1, 3,
    4, 4, 4, 4, 4, 0, 0, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
};

// ––––– VARIABLES ––––– //
GameState g_state;

SDL_Window* g_display_window;
bool g_game_is_running = true;
bool game_over = false;
bool game_start = true;

ShaderProgram g_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_accumulator = 0.0f;
const int FONTBANK_SIZE = 16;
// ––––– GENERAL FUNCTIONS ––––– //
GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);
    
    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }
    
    GLuint m_texture_id;
    glGenTextures(NUMBER_OF_TEXTURES, &m_texture_id);
    glBindTexture(GL_TEXTURE_2D, m_texture_id);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // the last argument can change depending on what you are looking for
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(image);
    
    return m_texture_id;
}

// Draw text function used from in-class exercise
void draw_text(ShaderProgram* g_program, GLuint font_m_texture_id, std::string text, float screen_size, float spacing, glm::vec3 position)
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
        // 1. Get their index in the spritesheet, as well as their offset (i.e. their m_position
        //    relative to the whole sentence)
        int spritesheet_index = (int)text[i];  // ascii value of character
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

    g_program->SetModelMatrix(model_matrix);
    glUseProgram(g_program->programID);

    glVertexAttribPointer(g_program->positionAttribute, 2, GL_FLOAT, false, 0, vertices.data());
    glEnableVertexAttribArray(g_program->positionAttribute);
    glVertexAttribPointer(g_program->texCoordAttribute, 2, GL_FLOAT, false, 0, texture_coordinates.data());
    glEnableVertexAttribArray(g_program->texCoordAttribute);

    glBindTexture(GL_TEXTURE_2D, font_m_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, (int)(text.size() * 6));

    glDisableVertexAttribArray(g_program->positionAttribute);
    glDisableVertexAttribArray(g_program->texCoordAttribute);
}

void initialise()
{
    // ––––– GENERAL STUFF ––––– //
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    g_display_window = SDL_CreateWindow("WASD to move, E to pounce (blink) attack, kill enemies to win!",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);
    
    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);
    
#ifdef _WINDOWS
    glewInit();
#endif
    // ––––– VIDEO STUFF ––––– //
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    
    g_program.Load(V_SHADER_PATH, F_SHADER_PATH);
    
    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    
    g_program.SetProjectionMatrix(g_projection_matrix);
    g_program.SetViewMatrix(g_view_matrix);
    
    glUseProgram(g_program.programID);
    
    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    
    // -- MAP -- //
    GLuint map_m_texture_id = load_texture("assets/platformerPack_industrial_tilesheet.png");
    g_state.map = new Map(LEVEL1_WIDTH, LEVEL1_HEIGHT, LEVEL_1_DATA, map_m_texture_id, 1.0f, 14, 8);
    
    // ––––– Player ––––– //
    // Existing
    g_state.player = new Entity();
    g_state.player->set_entity_type(PLAYER);
    g_state.player->set_position(glm::vec3(0.0f, 0.0f, 0.0f));
    g_state.player->set_movement(glm::vec3(0.0f));
    g_state.player->m_speed = 2.5f;
    g_state.player->set_acceleration(glm::vec3(0.0f, -9.81f, 0.0f));
    g_state.player->m_texture_id = load_texture(SPRITESHEET_FILEPATH);
    g_state.player->set_enemy_num(ENEMY_COUNT);
    
    // Walking
    g_state.player->m_walking[g_state.player->LEFT]  = new int[8] { 40, 41, 42, 43, 44, 45, 46, 47 };
    g_state.player->m_walking[g_state.player->RIGHT] = new int[8] { 40, 41, 42, 43, 44, 45, 46, 47 };
    g_state.player->m_walking[g_state.player->UP]    = new int[8] { 64, 65, 66, 67, 68, 69, 70, 64 };
    g_state.player->m_walking[g_state.player->DOWN]  = new int[8] {  0,  1,  2,  3, 24, 25, 26, 27 };

    g_state.player->m_animation_indices = g_state.player->m_walking[g_state.player->DOWN];
    g_state.player->m_animation_frames = 8;
    g_state.player->m_animation_index  = 0;
    g_state.player->m_animation_time   = 0.0f;
    g_state.player->m_animation_cols   = 8;
    g_state.player->m_animation_rows   = 10;
    g_state.player->set_height(0.8f);
    g_state.player->set_width(0.8f);
    
    // Jumping
    g_state.player->m_jumping_power = 5.0f;

    // Pouncing
    g_state.player->m_pounce_speed = 125.0f;
    
    // ––––– CROW ––––– //
    g_state.enemies = new Entity[ENEMY_COUNT];
    g_state.enemies[0].set_entity_type(ENEMY);
    g_state.enemies[0].set_ai_type(CROW);
    g_state.enemies[0].set_ai_state(IDLE);
    g_state.enemies[0].m_texture_id = load_texture("assets/Crow.png");
    g_state.enemies[0].set_position(glm::vec3(8.0f, 1.5f, 0.0f));
    g_state.enemies[0].set_movement(glm::vec3(0.0f));
    g_state.enemies[0].m_speed = 0.8f;
    g_state.enemies[0].set_acceleration(glm::vec3(0.0f, 0.0f, 0.0f));

    // Crow animation
    g_state.enemies[0].m_walking[g_state.enemies[0].LEFT] = new int[4]{ 1, 5, 9,  13 };
    g_state.enemies[0].m_walking[g_state.enemies[0].RIGHT] = new int[4]{ 3, 7, 11, 15 };
    g_state.enemies[0].m_walking[g_state.enemies[0].UP] = new int[4]{ 2, 6, 10, 14 };
    g_state.enemies[0].m_walking[g_state.enemies[0].DOWN] = new int[4]{ 0, 4, 8,  12 };

    g_state.enemies[0].m_animation_indices = g_state.enemies[0].m_walking[g_state.enemies[0].UP];
    g_state.enemies[0].m_animation_frames = 4;
    g_state.enemies[0].m_animation_index = 0;
    g_state.enemies[0].m_animation_time = 0.0f;
    g_state.enemies[0].m_animation_cols = 4;
    g_state.enemies[0].m_animation_rows = 4;
    g_state.enemies[0].set_height(0.8f);
    g_state.enemies[0].set_width(0.8f);


    // Mole(?) enemy
    g_state.enemies[1].set_entity_type(ENEMY);
    g_state.enemies[1].set_ai_type(GUARD);
    g_state.enemies[1].set_ai_state(IDLE);
    g_state.enemies[1].m_texture_id = load_texture("assets/asimole.png");
    g_state.enemies[1].set_position(glm::vec3(2.5f, 3.0f, 0.0f));
    g_state.enemies[1].set_movement(glm::vec3(0.0f));
    g_state.enemies[1].m_speed = 1.0f;
    g_state.enemies[1].set_acceleration(glm::vec3(0.0f, -9.81f, 0.0f));


    // Missile
    g_state.enemies[2].set_entity_type(ENEMY);
    g_state.enemies[2].set_ai_type(MISSILE);
    g_state.enemies[2].set_ai_state(IDLE);
    g_state.enemies[2].m_texture_id = load_texture("assets/missile.png");
    g_state.enemies[2].set_position(glm::vec3(18.0f, 3.0f, 0.0f));
    g_state.enemies[2].set_movement(glm::vec3(0.0f));
    g_state.enemies[2].m_speed = 1.2f;
    g_state.enemies[2].m_jumping_power = 1.0f;
    g_state.enemies[2].set_acceleration(glm::vec3(0.0f, -1.0f, 0.0f));

    // ––––– AUDIO STUFF ––––– //
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);

    Mix_VolumeMusic(MIX_MAX_VOLUME / 15.0f);
    
    g_state.jump_sfx = Mix_LoadWAV("assets/jump.wav");
    g_state.win_sfx = Mix_LoadWAV("assets/win.wav");
    g_state.lose_sfx = Mix_LoadWAV("assets/lose.wav");
    g_state.pounce_sfx = Mix_LoadWAV("assets/attack.wav");

    // ––––– GENERAL STUFF ––––– //
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
                game_start = false;
                switch (event.key.keysym.sym) {
                    case SDLK_q:
                        // Quit the game with a keystroke
                        g_game_is_running = false;
                        break;
                        
                    case SDLK_w:
                        // Jump
                        if (g_state.player->m_collided_bottom)
                        {
                            g_state.player->m_is_jumping = true;
                            g_state.player->m_animation_indices = g_state.player->m_walking[g_state.player->UP];
                            Mix_PlayChannel(-1, g_state.jump_sfx, 0);
                        }
                        break;
                    case SDLK_e:
                        // Dash Attack
                        g_state.player->m_is_pouncing = true;
                        Mix_PlayChannel(-1, g_state.pounce_sfx, 0);
                        g_state.player->m_animation_indices = g_state.player->m_walking[g_state.player->DOWN];
                        break;
                    default:
                        break;
                }
                
            default:
                break;
        }
    }
    
    const Uint8 *key_state = SDL_GetKeyboardState(NULL);

    // Animations
    if (key_state[SDL_SCANCODE_A])
    {
        g_state.player->m_movement.x = -1.0f;
        g_state.player->m_animation_indices = g_state.player->m_walking[g_state.player->LEFT];
    }
    else if (key_state[SDL_SCANCODE_D])
    {
        g_state.player->m_movement.x = 1.0f;
        g_state.player->m_animation_indices = g_state.player->m_walking[g_state.player->RIGHT];
    }
    else if (key_state[SDL_SCANCODE_W])
    {
        g_state.player->m_animation_indices = g_state.player->m_walking[g_state.player->UP];
    }
    else {
        g_state.player->m_animation_indices = g_state.player->m_walking[g_state.player->DOWN];
    }
    if (glm::length(g_state.player->m_movement) > 1.0f)
    {
        g_state.player->m_movement = glm::normalize(g_state.player->m_movement);
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
    
    while (delta_time >= FIXED_TIMESTEP) {
        g_state.player->update(FIXED_TIMESTEP, g_state.player, g_state.enemies, ENEMY_COUNT, g_state.map);
        
        for (int i = 0; i < ENEMY_COUNT; i++) g_state.enemies[i].update(FIXED_TIMESTEP, g_state.player, NULL, 0, g_state.map);
        
        delta_time -= FIXED_TIMESTEP;
        g_view_matrix = glm::mat4(1.0f);
        g_view_matrix = glm::translate(g_view_matrix, glm::vec3(-g_state.player->get_position().x, 0.0f, 0.0f));
    }
    
    g_accumulator = delta_time;
}

void render()
{
    g_program.SetViewMatrix(g_view_matrix);
    glClear(GL_COLOR_BUFFER_BIT);

    g_state.player->render(&g_program);
    g_state.map->render(&g_program);

    for (int i = 0; i < ENEMY_COUNT; i++)
    {
        if (g_state.enemies[i].get_active_state()) g_state.enemies[i].render(&g_program);
    }
    GLuint font_m_texture_id = load_texture(FONT_FILEPATH);
    if (game_start) {
        draw_text(&g_program, font_m_texture_id, "WASD to move", 1.0f, -0.5f, glm::vec3(-2.0f, 2.0f, 0.0f));
        draw_text(&g_program, font_m_texture_id, "E to pounce attack", 1.0f, -0.5f, glm::vec3(-4.0f, 1.0f, 0.0f));
    }
    if (!g_state.player->get_active_state()) {
        draw_text(&g_program, font_m_texture_id, "You lose!", 1.0f, -0.5f, glm::vec3(g_state.player->get_position().x - 1.5f, 2.0f, 0.0f));
        draw_text(&g_program, font_m_texture_id, "Q to quit", 1.0f, -0.5f, glm::vec3(g_state.player->get_position().x - 1.5f, 1.0f, 0.0f));
        if (!game_over) {
            Mix_PauseMusic();
            Mix_PlayChannel(-1, g_state.lose_sfx, 0);
            game_over = true;
        }
    }
    else if (g_state.player->get_enemy_num() == 0) {
        draw_text(&g_program, font_m_texture_id, "You win!", 1.0f, -0.5f, glm::vec3(g_state.player->get_position().x - 1.5f, 2.0f, 0.0f));
        draw_text(&g_program, font_m_texture_id, "Q to quit", 1.0f, -0.5f, glm::vec3(g_state.player->get_position().x - 1.5f, 1.0f, 0.0f));
        if (!game_over) {
            Mix_PauseMusic();
            Mix_PlayChannel(-1, g_state.win_sfx, 0);
            game_over = true;
        }
    }
    SDL_GL_SwapWindow(g_display_window);
}

void shutdown()
{    
    SDL_Quit();
    
    delete [] g_state.enemies;
    delete    g_state.player;
    delete    g_state.map;
    Mix_FreeChunk(g_state.pounce_sfx);
    Mix_FreeChunk(g_state.win_sfx);
    Mix_FreeChunk(g_state.lose_sfx);
    Mix_FreeChunk(g_state.jump_sfx);
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
