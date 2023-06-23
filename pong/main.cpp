/**
* Author: Justin Cheok
* Assignment: Pong Clone
* Date due: 2023-06-23, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

// NOTE: The character's hitbox (collision box) is towards their hands

#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1
#define LOG(argument) std::cout << argument << '\n'
#define STB_IMAGE_IMPLEMENTATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include <cmath>

// Include stdlib.h for rand
#include <stdlib.h>

// Include vector for drawing font
#include <vector>

const int WINDOW_WIDTH = 640,
WINDOW_HEIGHT = 480;

const float BG_RED = 0.9608f,
BG_BLUE = 0.9608f,
BG_GREEN = 0.9608f,
BG_OPACITY = 1.0f;

const int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

const char paddle1_SPRITE[] = "evan.png";
const char paddle2_SPRITE[] = "mage.png";
const char ball_SPRITE[] = "slime.png";
const char font_SPRITE[] = "font1.png";

const int NUMBER_OF_TEXTURES = 1;
const GLint LEVEL_OF_DETAIL = 0;
const GLint TEXTURE_BORDER = 0;

const float MILLISECONDS_IN_SECOND = 1000.0;

const int FONTBANK_SIZE = 16;

SDL_Window* g_display_window;
bool g_game_is_running = true;

ShaderProgram g_paddle1_program;
GLuint        g_paddle1_texture_id;

ShaderProgram g_paddle2_program;
GLuint        g_paddle2_texture_id;

ShaderProgram g_ball_program;
GLuint        g_ball_texture_id;

ShaderProgram g_text_program;
GLuint        g_text_texture_id;

glm::mat4 g_view_matrix,
g_paddle1_model_matrix,
g_paddle2_model_matrix,
g_ball_model_matrix,
g_projection_matrix;

float g_paddle_speed = 3.0f,
g_ball_speed = 2.5f,
g_previous_ticks = 0.0f;

bool start = true,
game_over = false,
p1_win = true;

glm::vec3 g_paddle1_position = glm::vec3(-4.2f, 0.0f, 0.0f),
g_paddle1_movement = glm::vec3(0.0f, 0.0f, 0.0f),
g_paddle1_scale = glm::vec3(1.0f, 1.0f, 0.0f),

g_paddle2_position = glm::vec3(4.2f, 0.0f, 0.0f),
g_paddle2_movement = glm::vec3(0.0f, 0.0f, 0.0f),
g_paddle2_scale = glm::vec3(1.0f, 1.0f, 0.0f),

g_ball_position = glm::vec3(0.0f, 0.0f, 0.0f),
g_ball_movement = glm::vec3(0.0f, 0.0f, 0.0f),
g_ball_scale = glm::vec3(1.0f, 1.0f, 0.0f);


/* Pythagorean theorem for circle-to-circle collision
float pythagorean(glm::vec3& a, glm::vec3& b)
{
    //distance^2 = (x-distance)^2 + (y-distance)^2)
    return sqrt(pow(b.x - a.x, 2) + pow(b.y - a.y, 2));
}
*/

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

    stbi_image_free(image);

    return textureID;
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);

    g_display_window = SDL_CreateWindow("Pong Clone",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    // ———————————————— paddle1 ———————————————— //

    g_paddle1_program.Load(V_SHADER_PATH, F_SHADER_PATH);

    g_paddle1_model_matrix = glm::mat4(1.0f);
    g_paddle1_model_matrix = glm::scale(g_paddle1_model_matrix, g_paddle1_scale);

    g_paddle1_program.SetProjectionMatrix(g_projection_matrix);
    g_paddle1_program.SetViewMatrix(g_view_matrix);
    glUseProgram(g_paddle1_program.programID);
    g_paddle1_texture_id = load_texture(paddle1_SPRITE);

    // ———————————————— paddle2 ———————————————— //
    g_paddle2_program.Load(V_SHADER_PATH, F_SHADER_PATH);

    g_paddle2_model_matrix = glm::mat4(1.0f);
    g_paddle2_model_matrix = glm::scale(g_paddle2_model_matrix, g_paddle2_scale);

    g_paddle2_program.SetProjectionMatrix(g_projection_matrix);
    g_paddle2_program.SetViewMatrix(g_view_matrix);
    glUseProgram(g_paddle2_program.programID);
    g_paddle2_texture_id = load_texture(paddle2_SPRITE);


    // ———————————————— ball ———————————————— //
    g_ball_program.Load(V_SHADER_PATH, F_SHADER_PATH);

    g_ball_model_matrix = glm::mat4(1.0f);
    g_ball_model_matrix = glm::scale(g_ball_model_matrix, g_ball_scale);

    g_ball_program.SetProjectionMatrix(g_projection_matrix);
    g_ball_program.SetViewMatrix(g_view_matrix);
    glUseProgram(g_ball_program.programID);
    g_ball_texture_id = load_texture(ball_SPRITE);

    // ———————————————— text ———————————————— //
    g_text_program.Load(V_SHADER_PATH, F_SHADER_PATH);
    g_text_program.SetProjectionMatrix(g_projection_matrix);
    g_text_program.SetViewMatrix(g_view_matrix);

    glUseProgram(g_text_program.programID);
    g_text_texture_id = load_texture(font_SPRITE);

    // ———————————————— GENERAL ———————————————— //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
}

void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            g_game_is_running = !g_game_is_running;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_q:
                g_game_is_running = !g_game_is_running;
                break;

            default: break;
            }
        }
    }
    const Uint8* key_state = SDL_GetKeyboardState(NULL);

    if (key_state[SDL_SCANCODE_W])
    {
        g_paddle1_movement.y = 1.0f;
    }
    else if (key_state[SDL_SCANCODE_S])
    {
        g_paddle1_movement.y = -1.0f;
    }

    if (glm::length(g_paddle1_movement) > 1.0f)
    {
        g_paddle1_movement = glm::normalize(g_paddle1_movement);
        g_paddle2_movement = glm::normalize(g_paddle2_movement);
    }

    if (key_state[SDL_SCANCODE_UP])
    {
        g_paddle2_movement.y = 1.0f;
    }
    else if (key_state[SDL_SCANCODE_DOWN])
    {
        g_paddle2_movement.y = -1.0f;
    }

    if (key_state[SDL_SCANCODE_Q])
    {
        g_game_is_running = false;
    }
}
// Randomize Y-direction after collision
float randomY() {
    float randomInt = rand() % 100 + 1; //Give a number between 1 and 100
    // Decimal between 0-1
    return randomInt > 50 ? (rand() % 100 + static_cast<float>(1)) / 100 : -((rand() % 100 + static_cast<float>(1)) / 100);
}
void update()
{
    if (start) {
        g_ball_movement.x = 1.0;
        // g_ball_movement.y = randomY();
        start = false;
    }

    // ———————————————— COLLISIONS ———————————————— //

    
    /* Testing circle-to-circle collision
    
    if (pythagorean(g_ball_position, g_paddle1_position) < 0.75f) { // Left player collision
        g_ball_movement.x = 1.0f;
        g_ball_movement.y = randomY();
    }
    else if (pythagorean(g_ball_position, g_paddle2_position) < 0.75f) { // Right player collision
        g_ball_movement.x = -1.0f;
        g_ball_movement.y = randomY();
    }
    */
    // Box-to-box collision
    if ((g_ball_position.x <= -5.0f) || (g_ball_position.x >= 5.0f)) { // If ball is out
        game_over = true;
        g_ball_movement = glm::vec3(0.0f, 0.0f, 0.0f);
        if (g_ball_position.x <= -5.0f) {
            p1_win = false;
        }
        else if (g_ball_position.x >= 5.0f) {
            p1_win = true;
        }
    }
    float collision_factor = 0.2f;
    float pad1_x_distance = fabs(g_paddle1_position.x - g_ball_position.x) - ((g_paddle1_scale.x * collision_factor + g_ball_scale.x * collision_factor) / 2.0f);
    float pad1_y_distance = fabs(g_paddle1_position.y - g_ball_position.y) - ((g_paddle1_scale.y * collision_factor + g_ball_scale.y * collision_factor) / 2.0f);

    float pad2_x_distance = fabs(g_paddle2_position.x - g_ball_position.x) - ((g_paddle2_scale.x * collision_factor + g_ball_scale.x * collision_factor) / 2.0f);
    float pad2_y_distance = fabs(g_paddle2_position.y - g_ball_position.y) - ((g_paddle2_scale.y * collision_factor + g_ball_scale.y * collision_factor) / 2.0f);

    // Top & bottom barrier collision
    if (g_ball_position.y >= 3.75f) {
        g_ball_movement.y = -1.0f;
    }
    else if (g_ball_position.y <= -3.75f) {
        g_ball_movement.y = 1.0f;
    }
    
    if (pad1_x_distance < 0 && pad1_y_distance < 0) {
        g_ball_movement.x = 1.0f;
        g_ball_movement.y = randomY();
    }
    if (pad2_x_distance < 0 && pad2_y_distance < 0) {
        g_ball_movement.x = -1.0f;
        g_ball_movement.y = randomY();
    }

    // ———————————————— DELTA TIME CALCULATIONS ———————————————— //
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    g_paddle1_position += g_paddle1_movement * delta_time * g_paddle_speed;
    g_paddle2_position += g_paddle2_movement * delta_time * g_paddle_speed;

    g_ball_position += g_ball_movement * delta_time * g_ball_speed;

    // ———————————————— RESETTING MODEL MATRIX & TRANSLATIONS ———————————————— //
    g_paddle1_model_matrix = glm::mat4(1.0f);
    g_paddle1_model_matrix = glm::translate(g_paddle1_model_matrix, g_paddle1_position);
    g_paddle1_movement = glm::vec3(0.0f, 0.0f, 0.0f);

    g_paddle2_model_matrix = glm::mat4(1.0f);
    g_paddle2_model_matrix = glm::translate(g_paddle2_model_matrix, g_paddle2_position);
    g_paddle2_movement = glm::vec3(0.0f, 0.0f, 0.0f);

    g_ball_model_matrix = glm::mat4(1.0f);
    g_ball_model_matrix = glm::translate(g_ball_model_matrix, g_ball_position);
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);

    if (game_over) {
        if (p1_win) {
            draw_text(&g_text_program, g_text_texture_id, std::string("PLAYER 1 WINS"), 0.25f, 0.0f, glm::vec3(-1.5f, 2.0f, 0.0f));
            draw_text(&g_text_program, g_text_texture_id, std::string("PRESS Q TO QUIT"), 0.25f, 0.01f, glm::vec3(-1.75f, 1.5f, 0.0f));
        }
        else {
            draw_text(&g_text_program, g_text_texture_id, std::string("PLAYER 2 WINS"), 0.25f, 0.0f, glm::vec3(-1.5f, 2.0f, 0.0f));
            draw_text(&g_text_program, g_text_texture_id, std::string("PRESS Q TO QUIT"), 0.25f, 0.01f, glm::vec3(-1.75f, 1.5f, 0.0f));
        }
       
    }

    // ———————————————— paddle1 ———————————————— //
    float paddle1_vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f
    };

    float paddle1_texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    };

    glVertexAttribPointer(g_paddle1_program.positionAttribute, 2, GL_FLOAT, false, 0, paddle1_vertices);
    glEnableVertexAttribArray(g_paddle1_program.positionAttribute);

    glVertexAttribPointer(g_paddle1_program.texCoordAttribute, 2, GL_FLOAT, false, 0, paddle1_texture_coordinates);
    glEnableVertexAttribArray(g_paddle1_program.texCoordAttribute);

    g_paddle1_program.SetModelMatrix(g_paddle1_model_matrix);
    glBindTexture(GL_TEXTURE_2D, g_paddle1_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(g_paddle1_program.positionAttribute);
    glDisableVertexAttribArray(g_paddle1_program.texCoordAttribute);

    // ———————————————— paddle2 ———————————————— //
    float paddle2_vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f
    };

    float paddle2_texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    };

    glVertexAttribPointer(g_paddle2_program.positionAttribute, 2, GL_FLOAT, false, 0, paddle2_vertices);
    glEnableVertexAttribArray(g_paddle2_program.positionAttribute);

    glVertexAttribPointer(g_paddle2_program.texCoordAttribute, 2, GL_FLOAT, false, 0, paddle2_texture_coordinates);
    glEnableVertexAttribArray(g_paddle2_program.texCoordAttribute);

    g_paddle2_program.SetModelMatrix(g_paddle2_model_matrix);
    glBindTexture(GL_TEXTURE_2D, g_paddle2_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(g_paddle2_program.positionAttribute);
    glDisableVertexAttribArray(g_paddle2_program.texCoordAttribute);

    // ———————————————— ball ———————————————— //
    float ball_vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f
    };

    float ball_texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    };

    glVertexAttribPointer(g_ball_program.positionAttribute, 2, GL_FLOAT, false, 0, ball_vertices);
    glEnableVertexAttribArray(g_ball_program.positionAttribute);

    glVertexAttribPointer(g_ball_program.texCoordAttribute, 2, GL_FLOAT, false, 0, ball_texture_coordinates);
    glEnableVertexAttribArray(g_ball_program.texCoordAttribute);

    g_ball_program.SetModelMatrix(g_ball_model_matrix);
    glBindTexture(GL_TEXTURE_2D, g_ball_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(g_ball_program.positionAttribute);
    glDisableVertexAttribArray(g_ball_program.texCoordAttribute);

    // ———————————————— GENERAL ———————————————— //
    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() { SDL_Quit(); }

/**
 Start here-we can see the general structure of a game loop without worrying too much about the details yet.
 */
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