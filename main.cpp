/**
* Author: [Michael Sanfilippo]
* Assignment: Pong Clone
* Date due: 2023-10-21, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedues on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define GL_GLEXT_PROTOTYPE 1
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

const int WINDOW_WIDTH = 640 * 2,
WINDOW_HEIGHT = 480 * 2;

const float BG_RED = 0.7f,
BG_BLUE = 0.2f,
BG_GREEN = 0.1f,
BG_OPACITY = 0.0f;

const int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

//Delta Time Variables
const float MILLISECONDS_IN_SECOND = 1000.0f;
float g_previous_ticks = 0.0f;

//Texture Variables
GLuint ball_texture_id;
GLuint paddle_texture_id;
const char BALL_SPRITE[] = "ball.png";
const char PADDLE_SPRITE[] = "paddle.png";
const int num_of_textures = 1;
const GLint level_of_detail = 0;
const GLint texture_border = 0;

const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

SDL_Window* g_display_window;
ShaderProgram g_program;

bool game_is_running = true;
bool is_two_player_mode = true;

// Initialize Matrices
glm::mat4 g_view_matrix, g_projection_matrix;
glm::mat4 g_model_matrix_p1, g_model_matrix_p2, g_model_matrix_ball;

// Paddle 1 Initialization
glm::vec3 p1_position = glm::vec3(-4.75f, 0.0f, 0.0f);
glm::vec3 p1_movement = glm::vec3(0.0f, 0.0f, 0.0f);

// Paddle 2 Initialization
glm::vec3 p2_position = glm::vec3(4.75f, 0.0f, 0.0f);
glm::vec3 p2_movement = glm::vec3(0.0f, 0.0f, 0.0f);

// Ball Initialization
glm::vec3 ball_position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 ball_movement = glm::vec3(0.0f, 0.0f, 0.0f);

// Paddle and Ball Sizes
float paddle_width = 0.5f;
float paddle_height = 2.0f;
float ball_width = 0.5f;
float ball_height = 0.5f;

//Speeds
const float paddle_speed = 4.5f;
const float ball_speed = 2.5f;

//Initial Ball Movement
bool ball_start_move = false;

//Variables for Collision Check 
float collison_factor = 0.09;
bool p1_collision = false;
bool p2_collision = false;

//Load textures that are to be rendered
GLuint load_texture(const char* filepath)
{
    //loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    //If image is not found
    if (image == NULL) {
        LOG("Unable to load image. Make sure path is correct");
        LOG(filepath);
        assert(false);
    }

    //Generating and Binding Texture ID to images 
    GLuint textureID;
    glGenTextures(num_of_textures, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, level_of_detail, GL_RGBA, width, height, texture_border, GL_RGBA, GL_UNSIGNED_BYTE, image);

    //Sets Texture Filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    //Release file from memory and return texture id
    stbi_image_free(image);

    return textureID;

}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
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
    g_program.load(V_SHADER_PATH, F_SHADER_PATH);
    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    g_program.set_projection_matrix(g_projection_matrix);
    g_program.set_view_matrix(g_view_matrix);
    glUseProgram(g_program.get_program_id());
    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    //Load shaders and image
    ball_texture_id = load_texture(BALL_SPRITE);
    paddle_texture_id = load_texture(PADDLE_SPRITE);

    //Enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input() 
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
            game_is_running = false;
            break;
        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_t) {
                // Toggle between one player and two players mode
                is_two_player_mode = !is_two_player_mode;
            }
            break;
        }
    }


    const Uint8* keys = SDL_GetKeyboardState(NULL);

    //Ball Initial Movement
    if (keys[SDL_SCANCODE_TAB]) {
        if (!ball_start_move) {
            ball_start_move = true;
            ball_movement.x = -1.0f;
            ball_movement.y = 0.3f;
        }
    }

    // Paddle 1 Controls
    if (keys[SDL_SCANCODE_W] && p1_position.y + (paddle_height / 2.0f) < 3.7f) 
    {
        p1_movement.y = 1.0f;
    }
    else if (keys[SDL_SCANCODE_S] && p1_position.y - (paddle_height / 2.0f) > -3.7f) 
    {
        p1_movement.y = -1.0f;
    }
    else 
    {
        p1_movement.y = 0.0f;
    }
    if (glm::length(p1_movement) > 1.0f) 
    {
        p1_movement = glm::normalize(p1_movement);
    }

    // Paddle 2 Controls (if in two-player mode)
    if (is_two_player_mode) 
    {
        if (keys[SDL_SCANCODE_UP] && p2_position.y + (paddle_height / 2.0f) < 3.7f) 
        {
            p2_movement.y = 1.0f;
        }
        else if (keys[SDL_SCANCODE_DOWN] && p2_position.y - (paddle_height / 2.0f) > -3.7f) 
        {
            p2_movement.y = -1.0f;
        }
        else 
        {
            p2_movement.y = 0.0f;
        }
        if (glm::length(p2_movement) > 1.0f)
        {
            p2_movement = glm::normalize(p2_movement);
        }
    }
}

void collision_check()
{
    //Paddle 1 Collision Check 
    float x_distance = fabs(ball_position.x - p1_position.x) - ((ball_width * collison_factor + paddle_width * collison_factor) / 2.0f);
    float y_distance = fabs(ball_position.y - p1_position.y) - ((ball_height * collison_factor + paddle_height) / 2.0f);
    if (x_distance < 0 && y_distance < 0 && !p1_collision)
    {
        ball_movement.x = -ball_movement.x; 
        p1_collision = true;
    }
    else
    {
        p1_collision = false;
    }

    //Paddle 2 Collision Check
    
     float x_distance2 = fabs(ball_position.x - p2_position.x) - ((ball_width * collison_factor + paddle_width * collison_factor) / 2.0f);
     float y_distance2 = fabs(ball_position.y - p2_position.y) - ((ball_height * collison_factor + paddle_height) / 2.0f);
     if (x_distance2 < 0 && y_distance2 < 0 && !p2_collision)
     {
         ball_movement.x = -ball_movement.x;
         p2_collision = true;
     }
     else
     {
         p2_collision = false;
     }

     //Top and Bottom Wall Collision Check 
     if (ball_position.y + ball_height / 2.0f >= 3.75)
     {
         ball_movement.y = -ball_movement.y;
     }

     if (ball_position.y - ball_height / 2.0f <= -3.75)
     {
         ball_movement.y = -ball_movement.y;
     }
}

void winner()
{
    if ((ball_position.x + ball_width / 2.0f >= 5.0f) || (ball_position.x - ball_width / 2.0f <= -5.0f))
    {
        game_is_running = false;
    }
}

void update() 
{
    //Calculating Delta Time
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    //Update Paddle 1 Position
    p1_position += p1_movement * paddle_speed * delta_time;
    g_model_matrix_p1 = glm::mat4(1.0f);
    g_model_matrix_p1 = glm::translate(g_model_matrix_p1, p1_position);
    g_model_matrix_p1 = glm::scale(g_model_matrix_p1, glm::vec3(paddle_width, paddle_height, 1.0f));

    //Update Paddle 2 Position 
    if (is_two_player_mode) 
    {
        p2_position += p2_movement * paddle_speed * delta_time;
        g_model_matrix_p2 = glm::mat4(1.0f);
        g_model_matrix_p2 = glm::translate(g_model_matrix_p2, p2_position);
        g_model_matrix_p2 = glm::scale(g_model_matrix_p2, glm::vec3(paddle_width, paddle_height, 1.0f));
    }

    //Update Ball Position
    ball_position += ball_movement * ball_speed * delta_time;
    g_model_matrix_ball = glm::mat4(1.0f);
    g_model_matrix_ball = glm::translate(g_model_matrix_ball, ball_position);
    g_model_matrix_ball = glm::scale(g_model_matrix_ball, glm::vec3(ball_width, ball_height, 1.0f));

    //Call Collision Function
    collision_check();

    //Call Winner Fucntion
    winner();

}

void draw_object(glm::mat4& object_model_matrix, GLuint& object_texture_id)
{
    g_program.set_model_matrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

// Render the game
void render() 
{
    glClear(GL_COLOR_BUFFER_BIT);

    //Setting Model Matricies
    g_program.set_model_matrix(g_model_matrix_p1);
    g_program.set_model_matrix(g_model_matrix_p2);
    g_program.set_model_matrix(g_model_matrix_ball);

    float vertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
    float texture_coordinates[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
  
    glVertexAttribPointer(g_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_program.get_position_attribute());
    glVertexAttribPointer(g_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_program.get_tex_coordinate_attribute());
   
    //Drawing Objects
    draw_object(g_model_matrix_p1, paddle_texture_id);
    draw_object(g_model_matrix_p2, paddle_texture_id);
    draw_object(g_model_matrix_ball, ball_texture_id);

    glDisableVertexAttribArray(g_program.get_position_attribute());
    glDisableVertexAttribArray(g_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown()
{
    SDL_Quit();
}

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
