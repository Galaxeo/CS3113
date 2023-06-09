#define GL_SILENCE_DEPRECATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <stdlib.h>
#define STB_IMAGE_IMPLEMENTATION

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

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

const char V_SHADER_PATH[] = "shaders/vertex.glsl",
F_SHADER_PATH[] = "shaders/fragment.glsl";

const int TRIANGLE_RED = 1.0,
TRIANGLE_BLUE = 0.4,
TRIANGLE_GREEN = 0.4,
TRIANGLE_OPACITY = 1.0;

const float GROWTH_FACTOR = 1.01f;
const float SHRINK_FACTOR = 0.99f;
const int MAX_FRAME = 40;

int frame_counter = 0;
bool is_growing = true;

SDL_Window* display_window;
bool game_is_running = true;

GLuint player_texture_id;

// In order to create an object, you need both a program and a model matrix
ShaderProgram program, program2;
glm::mat4 view_matrix, model_matrix, projection_matrix, model_matrix2;

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    display_window = SDL_CreateWindow("Triangle!",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(display_window);
    SDL_GL_MakeCurrent(display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    program.Load(V_SHADER_PATH, F_SHADER_PATH);
    program2.Load(V_SHADER_PATH, F_SHADER_PATH);

    //player_texture_id = load_texture(PLAYER_SPRITE);

    view_matrix = glm::mat4(1.0f);  // Defines the position (location and orientation) of the camera
    model_matrix = glm::mat4(1.0f);  // Defines every translation, rotations, or scaling applied to an object
    model_matrix2 = glm::mat4(1.0f);  // Defines every translation, rotations, or scaling applied to an object
    projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);  // Defines the characteristics of your camera, such as clip planes, field of view, projection method etc.

    program.SetProjectionMatrix(projection_matrix);
    program.SetViewMatrix(view_matrix);

    program2.SetProjectionMatrix(projection_matrix);
    program2.SetViewMatrix(view_matrix);
    // Notice we haven't set our model matrix yet!

    program.SetColor(TRIANGLE_RED, TRIANGLE_BLUE, TRIANGLE_GREEN, TRIANGLE_OPACITY);
    program2.SetColor(TRIANGLE_RED, TRIANGLE_BLUE, TRIANGLE_GREEN, TRIANGLE_OPACITY);

    glUseProgram(program.programID);
    glUseProgram(program2.programID);

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
}

void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE)
        {
            game_is_running = false;
        }
    }
}
int counter = 0;
bool onoff = 1;
float TRAN_VALUE = 0.001f;
const float INIT_TRIANGLE_ANGLE = glm::radians(15.0);

// Delta time (tick rate)
float x_player = 0.005f;
float z_rotate = 0.004f;
//float delta_time = 0.0333f;
float previous_ticks = 0.0f;


void update()
{
    float ticks = (float)SDL_GetTicks() / 1000.f;
    float delta_time = ticks - previous_ticks;
    previous_ticks = ticks;

    //x_player += 0.001 * delta_time;
    z_rotate += 0.0001 * delta_time;
    // This scale vector will make the x- and y-coordinates of the triangle
    // grow by a factor of 1% of it of its original size every frame.
    float scale_factor = 1.001f;
    float other_factor = 0.999f;

    if (counter <= 1000) {
        counter += 1;
    }
    else {
        counter = 0;
        onoff = !onoff;
    }
    if (counter % 1000 == 0) {
        int value = 0.05f + (rand() % 100000) / 100000.0f;

        /* Resetting position, best way to do this is actually to have
        * floats as x y and z values, then reset them once they get to
        * a certain value
        */
        model_matrix = glm::mat4(1.0f);
        model_matrix2 = glm::mat4(1.0f);

        // Set color, this is done correctly (similar to professor's)
        program.SetColor(0.05f + (rand() % 100000) / 100000.0f, 0.05f + (rand() % 100000) / 100000.0f, 0.05f + (rand() % 100000) / 100000.0f, TRIANGLE_OPACITY);
        program2.SetColor(0.05f + (rand() % 100000) / 100000.0f, 0.05f + (rand() % 100000) / 100000.0f, 0.05f + (rand() % 100000) / 100000.0f, TRIANGLE_OPACITY);
    }
    glm::vec3 scale_vector;
    if (onoff == 1) {
        scale_vector = glm::vec3(scale_factor, scale_factor, 1.0f);
    }
    else {
        scale_vector = glm::vec3(other_factor, other_factor, 1.0f);
    }


    // We replace the previous value of the model matrix with the scaled
    // value of model matrix. This would mean that  glm::scale() returns
    // a matrix, which it does!
    model_matrix = glm::translate(model_matrix, glm::vec3(x_player, 0.0f, 0.0f));
    model_matrix = glm::scale(model_matrix, scale_vector);
    model_matrix = glm::rotate(model_matrix, z_rotate, glm::vec3(0.0f, 0.0f, 1.0f));


    model_matrix2 = glm::translate(model_matrix2, glm::vec3(-x_player, 0.0f, 0.0f));
    model_matrix2 = glm::scale(model_matrix2, scale_vector);
    model_matrix2 = glm::rotate(model_matrix2, -z_rotate, glm::vec3(0.0f, 0.0f, 1.0f));
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);

    program.SetModelMatrix(model_matrix);


    float vertices[] =
    {
        0.5f, -0.5f,
        0.0f, 0.5f,
        -0.5f, -0.5f
    };

    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program.positionAttribute);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glDisableVertexAttribArray(program.positionAttribute);

    program2.SetModelMatrix(model_matrix2);
    float vertices2[] =
    {
        0.5f, -0.5f,
        0.0f, 0.5f,
        -0.5f, -0.5f
    };

    glVertexAttribPointer(program2.positionAttribute, 2, GL_FLOAT, false, 0, vertices2);
    glEnableVertexAttribArray(program2.positionAttribute);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glDisableVertexAttribArray(program2.positionAttribute);

    SDL_GL_SwapWindow(display_window);
}

void shutdown() { SDL_Quit(); }

/**
 Start here—we can see the general structure of a game loop without worrying too much about the details yet.
 */
int main(int argc, char* argv[])
{
    initialise();

    while (game_is_running)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}