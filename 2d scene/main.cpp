#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

enum Coordinate
{
    x_coordinate,
    y_coordinate
};

#define LOG(argument) std::cout << argument << '\n'

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
const float DEGREES_PER_SECOND = 90.0f;

const int NUMBER_OF_TEXTURES = 1; // to be generated, that is
const GLint LEVEL_OF_DETAIL = 0;  // base image level; Level n is the nth mipmap reduction image
const GLint TEXTURE_BORDER = 0;   // this value MUST be zero

const char PLAYER_SPRITE_FILEPATH[] = "keyboards.png";
const char OTHER_SPRITE_FILEPATH[] = "faceit.png";

SDL_Window* display_window;
bool game_is_running = true;
bool is_growing = true;

ShaderProgram program;
ShaderProgram program2;
glm::mat4 view_matrix, model_matrix, model_matrix2, projection_matrix, trans_matrix;

float previous_ticks = 0.0f;

GLuint player_texture_id;
GLuint other_texture_id;

GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    // STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);

    return textureID;
}

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

    model_matrix = glm::mat4(1.0f);
    model_matrix2 = glm::mat4(1.0f);
    view_matrix = glm::mat4(1.0f);
    projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    program.SetProjectionMatrix(projection_matrix);
    program.SetViewMatrix(view_matrix);
    program2.SetProjectionMatrix(projection_matrix);
    program2.SetViewMatrix(view_matrix);

    glUseProgram(program.programID);
    glUseProgram(program2.programID);

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    player_texture_id = load_texture(PLAYER_SPRITE_FILEPATH);
    other_texture_id = load_texture(OTHER_SPRITE_FILEPATH);

    // enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
int counter = 0;
bool onoff = 1;
const float INIT_TRIANGLE_ANGLE = glm::radians(15.0);

// Delta time (tick rate)
float x_player = 0.0005f;
float z_rotate = 0.0004f;
void update()
{
    float ticks = (float)SDL_GetTicks() / 1000.f;
    float delta_time = ticks - previous_ticks;
    previous_ticks = ticks;

    x_player += 0.00001 * delta_time;
    z_rotate += 0.00001 * delta_time;
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
    if (counter % 10000 == 0) {
        /* Resetting position, best way to do this is actually to have
        * floats as x y and z values, then reset them once they get to
        * a certain value, but this was done for in-class exercise
        */
        model_matrix = glm::mat4(1.0f);

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

void draw_object(glm::mat4& object_model_matrix, GLuint& object_texture_id)
{
    program.SetModelMatrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so we use 6 instead of 3
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Vertices
    float vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };

    // Textures
    float texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };

    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program.positionAttribute);

    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(program.texCoordAttribute);

    // Bind texture
    draw_object(model_matrix, player_texture_id);

    // We disable two attribute arrays now
    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);

    // Vertices
    float vertices2[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };

    // Textures
    float texture_coordinates2[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };

    glVertexAttribPointer(program2.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program2.positionAttribute);

    glVertexAttribPointer(program2.texCoordAttribute, 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(program2.texCoordAttribute);

    // Bind texture
    draw_object(model_matrix2, other_texture_id);

    // We disable two attribute arrays now
    glDisableVertexAttribArray(program2.positionAttribute);
    glDisableVertexAttribArray(program2.texCoordAttribute);

    SDL_GL_SwapWindow(display_window);
}

void shutdown()
{
    SDL_Quit();
}

/**
 Start here—we can see the general structure of a game loop without worrying too much about the details yet.
 */
int main(int argc, char* argv[])
{
    initialise();

    while (game_is_running)
    {
        update();
        render();
    }

    shutdown();
    return 0;
}
