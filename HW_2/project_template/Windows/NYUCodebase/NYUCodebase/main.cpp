#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>

#include "ShaderProgram.h"
#include "Matrix.h"

#define STB_IMAGE_IMPLEMENTATION 
#include "stb_image.h"




#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;



class Entity {
	public:
		Entity(float x, float y, float width, float height, float velocity, float x_direction = 0, float y_direction = 0) :
			x(x), y(y), width(width), height(height), velocity(velocity), x_direction(x_direction), y_direction(y_direction) {}

		float x;
		float y;

		float width;
		float height;
		
		float velocity;
		float x_direction;
		float y_direction;
};


int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	
	#ifdef _WINDOWS
		glewInit();
	#endif

	glViewport(0, 0, 640, 360);

	ShaderProgram program(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");
	SDL_Event event;

	Matrix projectionMatrix;
	Matrix modelviewMatrix;

	projectionMatrix.SetOrthoProjection(-3.55f, 3.55f, -2.0f, 2.0f, -1.0f, 1.0f);

	float lastFrameTicks = 0.0f;

	Entity top_wall = Entity(-3.55f, 2.0f, 7.1f, 0.1f, 0.0f);
	Entity bottom_wall = Entity(-3.55f, -1.9f, 7.1f, 0.1f, 0.0f);
	Entity ball = Entity(-0.05f, 0.05f, 0.1f, 0.1f, 0.0005f, 1.0f, 1.0f);
	Entity left_paddle = Entity(-3.3f, 0.25f, 0.1f, 0.5f, 4.0f);
	Entity right_paddle = Entity(3.2f, 0.25f, 0.1f, 0.5f, 4.0f);


	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}

		const Uint8 *keys = SDL_GetKeyboardState(NULL);

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;


		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

		glUseProgram(program.programID);

		program.SetProjectionMatrix(projectionMatrix);
		program.SetModelviewMatrix(modelviewMatrix);

		if (keys[SDL_SCANCODE_W]) {
			if(left_paddle.y < top_wall.y-top_wall.height)
				left_paddle.y += left_paddle.velocity * elapsed;
		}
		else if (keys[SDL_SCANCODE_S]) {
			if (left_paddle.y - left_paddle.height > bottom_wall.y)
				left_paddle.y -= left_paddle.velocity * elapsed;
		}
		if (keys[SDL_SCANCODE_UP]) {
			if (right_paddle.y < top_wall.y - top_wall.height)
				right_paddle.y += right_paddle.velocity * elapsed;
		}
		else if (keys[SDL_SCANCODE_DOWN]) {
			if (right_paddle.y - right_paddle.height > bottom_wall.y)
				right_paddle.y -= right_paddle.velocity * elapsed;
		}
	

		// Up-side Wall
		float vertices[] = {top_wall.x, top_wall.y - top_wall.height, 
							top_wall.x + top_wall.width, top_wall.y - top_wall.height, 
							top_wall.x + top_wall.width, top_wall.y,
							top_wall.x, top_wall.y - top_wall.height,
							top_wall.x + top_wall.width, top_wall.y,
							top_wall.x, top_wall.y};

		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program.positionAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);

		// Down-side Wall
		float vertices2[] = { bottom_wall.x, bottom_wall.y - bottom_wall.height,
							  bottom_wall.x + bottom_wall.width, bottom_wall.y - bottom_wall.height,
							  bottom_wall.x + bottom_wall.width, bottom_wall.y,
							  bottom_wall.x, bottom_wall.y - bottom_wall.height,
							  bottom_wall.x + bottom_wall.width, bottom_wall.y,
							  bottom_wall.x, bottom_wall.y };

		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices2);
		glEnableVertexAttribArray(program.positionAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);


		// Ball

		
		float vertices3[] = { ball.x, ball.y - ball.height,
							  ball.x + ball.width, ball.y - ball.height,
							  ball.x + ball.width, ball.y,
							  ball.x, ball.y - ball.height,
							  ball.x + ball.width, ball.y,
							  ball.x, ball.y };

		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices3);
		glEnableVertexAttribArray(program.positionAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);

		ball.x += ball.velocity * ball.x_direction;
		ball.y += ball.velocity * ball.y_direction; 

		if ((ball.y >= top_wall.y - top_wall.height) || (ball.y - ball.height <= bottom_wall.y))
			ball.y_direction *= -1;

		/*if ((!(ball.y - ball.height > left_paddle.y) &&
			!(ball.y < left_paddle.y - left_paddle.height) &&
			!(ball.x > left_paddle.x + left_paddle.width) &&
			!(ball.x + ball.width < left_paddle.x)) ||
			(!(ball.y - ball.height > right_paddle.y) &&
			!(ball.y < right_paddle.y - right_paddle.height) &&
			!(ball.x > right_paddle.x + right_paddle.width) &&
			!(ball.x + ball.width < right_paddle.x))) {
			ball.x_direction *= -1;
		}*/

		if ((!(ball.y - ball.height > left_paddle.y) &&
			!(ball.y < left_paddle.y - left_paddle.height) &&
			!(ball.x > left_paddle.x + left_paddle.width) &&
			!(ball.x + ball.width < left_paddle.x)) ||
			(!(ball.y - ball.height > right_paddle.y) &&
			!(ball.y < right_paddle.y - right_paddle.height) &&
			!(ball.x > right_paddle.x + right_paddle.width) &&
			!(ball.x + ball.width < right_paddle.x))) {
			ball.x_direction *= -1;
		}

		if (ball.x > 3.55f) {
			ball.x = -0.05f;
			ball.y = 0.05f;
			if (left_paddle.height > 0.3f)
				left_paddle.height -= 0.1f;
			else {
				right_paddle.height += 0.1f;
			}
		}
		else if (ball.x + ball.width < -3.55f) {
			ball.x = -0.05f;
			ball.y = 0.05f;
			if (right_paddle.height > 0.3f)
				right_paddle.height -= 0.1f;
			else {
				left_paddle.height += 0.1f;
			}
			
		}

		// Left Paddle
		float vertices4[] = { left_paddle.x, left_paddle.y - left_paddle.height,
							  left_paddle.x + left_paddle.width, left_paddle.y - left_paddle.height,
							  left_paddle.x + left_paddle.width, left_paddle.y,
							  left_paddle.x, left_paddle.y - left_paddle.height,
							  left_paddle.x + left_paddle.width, left_paddle.y,
							  left_paddle.x, left_paddle.y };

		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices4);
		glEnableVertexAttribArray(program.positionAttribute);
		
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);


		// Right Paddle
		float vertices5[] = { right_paddle.x, right_paddle.y - right_paddle.height,
							  right_paddle.x + right_paddle.width, right_paddle.y - right_paddle.height,
							  right_paddle.x + right_paddle.width, right_paddle.y,
							  right_paddle.x, right_paddle.y - right_paddle.height,
							  right_paddle.x + right_paddle.width, right_paddle.y,
							  right_paddle.x, right_paddle.y };

		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices5);
		glEnableVertexAttribArray(program.positionAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);


		SDL_GL_SwapWindow(displayWindow);


	}

	SDL_Quit();
	return 0;
}
