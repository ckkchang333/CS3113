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

#include <stdlib.h>
#include <vector>

using namespace std;


SDL_Window* displayWindow;

enum GameMode {STATE_MENU, STATE_GAME};

GameMode mode = STATE_MENU;



GLuint LoadTexture(const char * filePath) {
	int w, h, comp;
	unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);
	if (image == NULL) {
		std::cout << "Unable to load image. Make sure the path is correct\n";
		assert(false);
	}

	GLuint retTexture;
	glGenTextures(1, &retTexture);
	glBindTexture(GL_TEXTURE_2D, retTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	stbi_image_free(image);
	return retTexture;
}

void drawText(ShaderProgram& program, int fontTexture, string text, float size, float spacing, float x, float y) {
	float texture_size = 1.0 / 16.0f;
	vector<float> vertexData;
	vector<float> texCoordData;

	for (int i = 0; i < text.size(); i++) {
		int spriteIndex = (int)text[i];

		float texture_x = (float)(spriteIndex % 16) / 16.0f;
		float texture_y = (float)(spriteIndex / 16) / 16.0f;

		vertexData.insert(vertexData.end(), {
			((size + spacing) * i) + (-0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
		});
		
		texCoordData.insert(texCoordData.end(), {
			texture_x, texture_y,
			texture_x, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x + texture_size, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x, texture_y + texture_size,
		});
	}

	glBindTexture(GL_TEXTURE_2D, fontTexture);



	for (int i = 0; i < vertexData.size()/12; ++i) {
		float vertices[12];
		float texCoords[12];

		for (int j = 0; j < 12; ++j) {
			vertices[j] = *(vertexData.data() + (i * 12) + j);
			texCoords[j] = *(texCoordData.data() + (i * 12) + j);
		}
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program.positionAttribute);

		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		Matrix modelview;
		modelview.Translate(x, y, 0.0f);
		program.SetModelviewMatrix(modelview);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);
	}

	
}



class Entity {
public:
	Entity(): index(0), x(0), y(0), width(0), height(0), velocity(0),  visible(false), x_direction(0), y_direction(0) {}
	
	Entity(int index,  float x, float y, float width, float height, float velocity, bool vis = true, float x_direction = 1.0f, float y_direction = 1.0f) :
		index(index), visible(vis), x(x), y(y), width(width), height(height), velocity(velocity), x_direction(x_direction), y_direction(y_direction) {}

	void draw(ShaderProgram& program, GLuint texID,  int spriteCountX, int spriteCountY);

	int index;

	float x;
	float y;

	float width;
	float height;

	float velocity;
	float x_direction;
	float y_direction;

	bool visible; 


};

void Entity::draw(ShaderProgram& program, GLuint texID,  int spriteCountX, int spriteCountY) {
	glBindTexture(GL_TEXTURE_2D, texID);
	float vertices[] = {
		/*width / 2, height / 2,
		-(width / 2), -(height / 2),
		width / 2, -(height / 2),
		-(width / 2), -(height / 2),
		width / 2, height / 2,
		-(width / 2), height / 2*/
		x, y - height,
		x + width, y - height,
		x + width, y,
		x, y - height,
		x + width, y,
		x, y
	};

	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program.positionAttribute);

	float u = (float)(((int)index) % spriteCountX) / (float)spriteCountX;
	float v = (float)(((int)index) / spriteCountX) / (float)spriteCountY;
	float spriteWidth = 1.0 / (float)spriteCountX;
	float spriteHeight = 1.0 / (float)spriteCountY;

	GLfloat texCoords[] = {
		u, v + spriteHeight,
		u + spriteWidth, v + spriteHeight,
		u + spriteWidth, v,
		u, v + spriteHeight,
		u + spriteWidth, v,
		u, v
	};

	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program.texCoordAttribute);

	Matrix modelview;
	modelview.Translate(x, y, 0.0f);
	program.SetModelviewMatrix(modelview);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);
}

class GameState {
public:
	GameState() {}
	Entity hero;
	Entity enemies[25];
	Entity hero_bullet;
	Entity enemy_bullets[3];
	int score;
};

GameState& setup() {
	GameState game;
	game.hero = Entity(43, -0.0f, -3.0f, 0.5f, 0.5f, 0.0f);


	int pos_x = 0;
	for (int i = 0; i < 5; ++i) {
		game.enemies[i * 5] = Entity(10, -1.75f + pos_x * .75f, 3.25f, 0.75f, 0.75f, 0.0005, true, 1.0f);
		game.enemies[i * 5 + 1] = Entity(49, -1.75f + pos_x * .75f, 2.50f, 0.75f, 0.75f, 0.0005, true, 1.0f);
		game.enemies[i * 5 + 2] = Entity(52, -1.75f + pos_x * .75f, 1.75f, 0.75f, 0.75f, 0.0005, true, 1.0f);
		game.enemies[i * 5 + 3] = Entity(55, -1.75f + pos_x * .75f, 1.00f, 0.75f, 0.75f, 0.0005, true, 1.0f);
		game.enemies[i * 5 + 4] = Entity(58, -1.75f + pos_x * .75f, 0.25f, 0.75f, 0.75f, 0.0005, true, 1.0f);
		++pos_x;
	}

		Entity bullet = Entity();
		bullet.index = 42;
		bullet.width = 1.0f;
		bullet.height = 1.0f;
		bullet.velocity = 0.001f;
		bullet.y_direction = 1.0f;
		game.hero_bullet = bullet;


	for (int i = 0; i < 3; ++i) {
		Entity bullet = Entity();
		bullet.index = 149;
		bullet.width = 1.0f;
		bullet.height = 1.0f;
		bullet.velocity = 0.0005f;
		game.enemy_bullets[i] = bullet;
	}

	game.score = 0;

	return game;
}


void RenderMenu(ShaderProgram& program, GLuint fontTexID) {
	drawText(program, fontTexID, "Press 'S' to ", 0.6f, 0.0f, -3.3f, 1.0f);
	drawText(program, fontTexID, "Start + Shoot", 0.6f, 0.0f, -3.6f, 0.0f);
}

void RenderGame(GameState& game, ShaderProgram& program, GLuint texIDs[]) {
	
	if (game.hero.visible)
		game.hero.draw(program, texIDs[0], 12, 8);

	for (int i = 0; i < 25; ++i) {
		if (game.enemies[i].visible)
			game.enemies[i].draw(program, texIDs[0], 12, 8);
		
	}
	
	for (int i = 0; i < 6; ++i) {
		if (game.hero_bullet.visible) {
			game.hero_bullet.draw(program, texIDs[1], 16, 16);
		}
	}

	for (int i = 0; i < 3; ++i) {
		if (game.enemy_bullets[i].visible) {
			game.enemy_bullets[i].draw(program, texIDs[1], 16, 16);
		}
	}
}

void Render(GameState& game, ShaderProgram& program, GLuint texIDs[]) {
	switch (mode) {
	case STATE_MENU:
		RenderMenu(program, texIDs[1]);
		break;

	case STATE_GAME:
		RenderGame(game, program, texIDs);
		break;
	}
}

void UpdateGame(GameState& game, float elapsed) {
	game.hero.x += game.hero.velocity * game.hero.x_direction;

	float bullet_x_offset = 0.50f;
	float bullet_y_offset = 0.70f;
	if (game.hero_bullet.visible) {
		game.hero_bullet.y += game.hero_bullet.velocity;
		if (game.hero_bullet.y + game.hero_bullet.height > 7.1f)
			game.hero_bullet.visible = false;
		for (int j = 0; j < 25; ++j) {
			if ((!(game.hero_bullet.y - game.hero_bullet.height + 0.35 > game.enemies[j].y) &&
				!(game.hero_bullet.y - 0.60 < game.enemies[j].y - game.enemies[j].height) &&
				!(game.hero_bullet.x + 0.15 > game.enemies[j].x + game.enemies[j].width - 0.25f) &&
				!(game.hero_bullet.x + game.hero_bullet.width - 0.65 < game.enemies[j].x )) && game.enemies[j].visible) {
				game.hero_bullet.visible = false;
				game.enemies[j].visible = false;
			}
		}
		
	}

	for (int i = 0; i < 3; ++i) {
		if (game.enemy_bullets[i].visible) {
			game.enemy_bullets[i].y -= game.enemy_bullets[i].velocity;
			if (game.enemy_bullets[i].y < -3.55f) {
				game.enemy_bullets[i].visible = false;
			}
			else if (!(game.enemy_bullets[i].y - game.enemy_bullets[i].height + 0.70f > game.hero.y) &&
				!(game.enemy_bullets[i].y - 0.50f < game.hero.y - game.hero.height) &&
				!(game.enemy_bullets[i].x + 0.50f > game.hero.x + game.hero.width) &&
				!(game.enemy_bullets[i].x + game.enemy_bullets[i].width - 0.70f < game.hero.x)) {
				mode = STATE_MENU;

			}

		}
		

	}
	
	bool game_end = true;

	for (int i = 0; i < 5; ++i) {
		bool turn = false;
		if (game.enemies[i * 5].x < -2.0f || (game.enemies[i * 5 + 4].x + game.enemies[i * 5 + 4].width) > 2.4f) {
			turn = true;
		}
		for (int j = 0; j < 5; ++j) {
			if (turn) 
			game.enemies[i * 5 + j].x_direction *= -1.0f;
			game.enemies[i * 5 + j].x += game.enemies[i * 5 + j].velocity * game.enemies[i * 5 + j].x_direction;
			if (game_end && game.enemies[i * 5 + j].visible)
				game_end = false;
		}
	}

	if (game_end)
		mode = STATE_MENU;

	
	int x = rand() % 25 + 1;
	for (int i = 0; i < 3; ++i) {
		if (!game.enemy_bullets[i].visible && game.enemies[x].visible) {
			game.enemy_bullets[i].visible = true;
			game.enemy_bullets[i].x = game.enemies[x].x + game.enemies[x].width/4;
			game.enemy_bullets[i].y = game.enemies[x].y;
			break;
		}
	}
}

void Update(GameState& game, float elapsed) {
	switch (mode) {
	case STATE_GAME:
		UpdateGame(game, elapsed);
	break;
	}
}

void InputMenu(GameState& game) {
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	if (keys[SDL_SCANCODE_S]) {
		mode = STATE_GAME;
		game = setup();
	}
}

void InputGame(GameState& game, SDL_Event& event){
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	if (keys[SDL_SCANCODE_LEFT]) {
		if (game.hero.x > -2.00f) {
			game.hero.velocity = 0.001f;
			game.hero.x_direction = -1.0f;
		}
		else
			game.hero.velocity = 0;
	}
	else if (keys[SDL_SCANCODE_RIGHT]) {
		if (game.hero.x + game.hero.width < 2.25f) {
			game.hero.velocity = 0.001f;
			game.hero.x_direction = 1.0f;
		}
		else
			game.hero.velocity = 0;
	}
	else {
		game.hero.velocity = 0;
	}
}

void Input(GameState& game, SDL_Event& event) {
	switch (mode) {
	case STATE_MENU:
		InputMenu(game);
		break;

	case STATE_GAME:
		InputGame(game, event);
		break;
	}
}

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 360, 640, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#ifdef _WINDOWS
	glewInit();
#endif

	glViewport(0, 0, 360, 640);

	ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	SDL_Event event;

	
	Matrix projectionMatrix;
	Matrix modelviewMatrix;

	//projectionMatrix.SetOrthoProjection(-2.0f, 2.0f, -3.55f, 3.55f, -1.0f, 1.0f);

	projectionMatrix.SetOrthoProjection(-4.0f, 4.0f, -7.1f, 7.1f, -1.0f, 1.0f);

	//projectionMatrix.SetOrthoProjection(-12.0f, 12.0f, -21.3f, 21.3f, -1.0f, 1.0f);

	float lastFrameTicks = 0.0f;

	GLuint characters = LoadTexture(RESOURCE_FOLDER"characters_1.png");
	GLuint font = LoadTexture(RESOURCE_FOLDER"font1.png");
	GLuint textures[2] = { characters, font };
	
	GameState game = setup();

	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
			else if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.scancode == SDL_SCANCODE_S) {
					if (!game.hero_bullet.visible) {
						game.hero_bullet.visible = true;
						game.hero_bullet.x = game.hero.x;
						game.hero_bullet.y = game.hero.y + game.hero.height/2;
						break;
					}
					
				}
			}
		}

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;


		glClearColor(0.133f, 0.545f, 0.133f, 1.0f);
		//glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);


		glUseProgram(program.programID);

		program.SetProjectionMatrix(projectionMatrix);
		program.SetModelviewMatrix(modelviewMatrix);

		Input(game, event);
		Update(game, elapsed);
		Render(game, program, textures);

		
		


		// Up-side Wall

		/*
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

		*/


		

  		SDL_GL_SwapWindow(displayWindow);


	}

	SDL_Quit();
	return 0;
}