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
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>

#define TILE_SIZE 4
#define SPRITE_COUNT_X 16
#define SPRITE_COUNT_Y 8
#define SPRITE_WIDTH 1.0 / 16
#define SPRITE_HEIGHT 1.0 / 8
#define GRAVITY -90

using namespace std;


SDL_Window* displayWindow;


// Static tile rendering information
unsigned int** levelData;
int mapHeight, mapWidth;
vector<float> vertexData;
vector<float> texCoordData;

GLuint LoadTexture() {
	int w, h, comp;
	unsigned char* image = stbi_load(RESOURCE_FOLDER"arne_sprites.png", &w, &h, &comp, STBI_rgb_alpha);
	if (image == NULL) {
		cout << "Unable to load image. Make sure the path is correct\n";
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

class SheetSprite {
public:
	SheetSprite();
	SheetSprite(unsigned int textureID, float u, float v, float width, float height, float size);

	void Draw(Matrix& view, ShaderProgram* program, float x, float y);

	float size;
	unsigned int textureID;
	float u;
	float v;
	float width;
	float height;
};

SheetSprite::SheetSprite():size(1), textureID(0) {}

SheetSprite::SheetSprite(unsigned int textureID, float u, float v, float width, float height, float size): 
	textureID(textureID), u(u), v(v), width(width), height(height), size(size) {}

void SheetSprite::Draw(Matrix& view, ShaderProgram* program, float x, float y) {
	GLfloat texCoords[]{
		u, v + height,
		u + width, v,
		u, v,
		u + width, v,
		u, v + height,
		u + width, v + height
	};
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program->texCoordAttribute);


	float vertices[] = {
		-0.5f * size , -0.5f * size,
		0.5f * size , 0.5f * size,
		-0.5f * size , 0.5f * size,
		0.5f * size , 0.5f * size,
		-0.5f * size , -0.5f * size,
		0.5f * size , -0.5f * size,
	};

	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program->positionAttribute);

	Matrix modelview;
	modelview = modelview * view;
	modelview.Translate(x, y, 0.0f);
	program->SetModelviewMatrix(modelview);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}


class Vector3 {
public:
	Vector3();

	Vector3(float x, float y, float z);
	float x;
	float y;
	float z;
};

Vector3::Vector3():x(0), y(0), z(0) {}

Vector3::Vector3(float x, float y, float z): x(x), y(y), z(z) {}


class Entity {
public:
	
	Entity();

	Entity(Vector3 position, Vector3 size);

	void Update(float elapsed);
	void Render(Matrix& view, ShaderProgram &program);
	void CollidesWith(Entity& entity);

	SheetSprite sprite;

	Vector3 position;
	Vector3 size;
	Vector3 velocity;
	Vector3 acceleration;

	bool draw;
	bool isStatic;
	bool collidedTop;
	bool collidedBottom;
	bool collidedLeft;
	bool collidedRight;
};


Entity::Entity() : position(Vector3()), size(Vector3()), velocity(Vector3()), acceleration(Vector3()), isStatic(true) {}

Entity::Entity(Vector3 position, Vector3 size): position(position), size(size), velocity(Vector3()), acceleration(Vector3()),
												collidedTop(false), collidedBottom(false), collidedLeft(false), collidedRight(false), draw(true){}

void Entity::Update(float elapsed) {

	collidedBottom = false;
	collidedLeft = false;
	collidedRight = false;
	collidedTop = false;
	velocity.x += acceleration.x * elapsed;
	velocity.y += acceleration.y * elapsed;

	// Apply gravity if the hero is not touching the ground
	if (!collidedBottom)
		velocity.y += GRAVITY * elapsed;

	position.y += velocity.y * elapsed;

	for (int i = 0; i < mapHeight; ++i) {
		for (int j = 0; j < mapWidth; ++j) {
			if (levelData[i][j] != 0 &&  levelData[i][j] != 113 && levelData[i][j] != 114 && levelData[i][j] != 115) {
				float bottom = position.y - size.y / 2;
				float top = position.y + size.y / 2;
				float left = position.x - size.x / 2;
				float right = position.x + size.x / 2;
				float tile_bottom = (-TILE_SIZE * i - TILE_SIZE);
				float tile_top = (-TILE_SIZE * i);
				float tile_left = (TILE_SIZE * j);
				float tile_right = (TILE_SIZE * j + TILE_SIZE);
				float tile_center = (-TILE_SIZE * i - TILE_SIZE / 2);

				if (!(bottom > tile_top) &&
					!(top < tile_bottom) &&
					!(left > tile_right) &&
					!(right < tile_left)) {

					float penetration = fabs(fabs(position.y - tile_center) - size.y / 2 - TILE_SIZE / 2);
					if (top < tile_top) {
						velocity.y = 0;
						position.y -= penetration + 0.0001;
						velocity.y = 0;
						acceleration.y = 0;
						collidedTop = true;
						break;
					}
					else if (top > tile_top) {
						position.y += penetration + 0.0001;
						collidedBottom = true;
						velocity.y = 0;
						acceleration.y = 0;
						break;
					}
				}
			}
		}
	}

	position.x += velocity.x * elapsed;

	// Resolve horizontal penetration
	for (int i = 0; i < mapHeight; ++i) {
		for (int j = 0; j < mapWidth; ++j) {
			if (levelData[i][j] != 0  && levelData[i][j] != 113 && levelData[i][j] != 114 && levelData[i][j] != 115) {


				float bottom = position.y - size.y / 2;
				float top = position.y + size.y / 2;
				float left = position.x - size.x / 2;
				float right = position.x + size.x / 2;
				float tile_bottom = (-TILE_SIZE * i - TILE_SIZE);
				float tile_top = (-TILE_SIZE * i);
				float tile_left = (TILE_SIZE * j);
				float tile_right = (TILE_SIZE * j + TILE_SIZE);
				float tile_center = (TILE_SIZE * j + TILE_SIZE / 2);
				if (!(bottom > tile_top) &&
					!(top < tile_bottom) &&
					!(left > tile_right) &&
					!(right < tile_left)) {
					float penetration = fabs(fabs(position.x - tile_center) - size.x / 2 - TILE_SIZE / 2);
					if (right > tile_right) {
						if (collidedBottom) {
						}
						if (velocity.x < 0) velocity.x = 0;
						position.x += penetration + 0.0001;
						collidedLeft = true;
					}
					else if (right < tile_right) {
						if (velocity.x > 0) velocity.x = 0;
						position.x -= penetration + 0.0001;
						collidedRight = true;
					}
				}
			}
		}
	}
	
	
}

void Entity::Render(Matrix& view, ShaderProgram &program) {
	sprite.Draw(view, &program, position.x, position.y);
}

void Entity::CollidesWith(Entity& entity) {
	if (size.x / 2 + entity.size.x / 2 > fabs((position.x + size.x / 2) - (entity.position.x + size.x / 2)) &&
		size.y / 2 + entity.size.y / 2 > fabs((position.y + size.y / 2) - (entity.position.y + size.y / 2))) {
		entity.draw = false;
	}
}


class GameState {
public:
	GameState();
	float camera;
	vector<Entity*> dynamics;
};

GameState::GameState(): camera(0) {}

GameState& setup() {
	GameState game = GameState();

	return game;
}






void placeEntity(string type, float x, float y, GLuint texID, GameState& game) {
	Vector3 position = Vector3(x, y, 0);
	Vector3 size = Vector3(4.0f, 4.0f, 0);
	Entity*  new_ent = new Entity(position, size);

	int index = 0;
	if (type == "Rock")
		index = 80;
	else
		index = 23;
	float u = (float)(((int)index) % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X;
	float v = (float)(((int)index) / SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y;
	SheetSprite new_sprite = SheetSprite(texID, u, v, SPRITE_WIDTH, SPRITE_HEIGHT, 4.1);
	new_ent->sprite = new_sprite;

	new_ent->isStatic = false;

	game.dynamics.push_back(new_ent);
}

bool readHeader(ifstream &stream) {
	string line;
	mapWidth = -1;
	mapHeight = -1;
	while (getline(stream, line)) {
		if (line == "") break;

		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);

		if (key == "width")
			mapWidth = atoi(value.c_str());
		else if (key == "height")
			mapHeight = atoi(value.c_str());
	}
	if (mapWidth == -1 || mapHeight == -1)
		return false;
	else {
		levelData = new unsigned int*[mapHeight];
		for (int i = 0; i < mapHeight; ++i) {
			levelData[i] = new unsigned int[mapWidth];
		}
		return true;
	}
}

bool readLayerData(ifstream &stream) {
	string line;
	while (getline(stream, line)) {
		if (line == "") break;
		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "data") {
			for (int y = 0; y < mapHeight; y++) {
				getline(stream, line);
				istringstream lineStream(line);
				string tile;
				for (int x = 0; x < mapWidth; x++) {
					getline(lineStream, tile, ',');
					int val = atoi(tile.c_str());
					if (val > 0)
						levelData[y][x] = val - 1;
					else
						levelData[y][x] = 0;
				}
			}
		}
	}

	for (int y = 0; y < mapHeight; ++y) {
		for (int x = 0; x < mapWidth; ++x) {
			if (levelData[y][x] != 0) {
				float u = (float)(((int)levelData[y][x]) % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X;
				float v = (float)(((int)levelData[y][x]) / SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y;
				vertexData.insert(vertexData.end(), {
					(float)TILE_SIZE * x,					 (float)-TILE_SIZE * y,
					(float)TILE_SIZE * x,					 (float)(-TILE_SIZE * y) - TILE_SIZE,
					(float)(TILE_SIZE * x) + TILE_SIZE,		 (float)(-TILE_SIZE * y) - TILE_SIZE,

					(float)TILE_SIZE * x,					 (float)-TILE_SIZE * y,
					(float)(TILE_SIZE * x) + TILE_SIZE,		 (float)(-TILE_SIZE * y) - TILE_SIZE,
					(float)(TILE_SIZE * x) + TILE_SIZE,		 (float)-TILE_SIZE * y

				});

				texCoordData.insert(texCoordData.end(), {
					u, v,
					u,  v + (float)SPRITE_HEIGHT,
					u + (float)SPRITE_WIDTH, v + (float)SPRITE_HEIGHT,
					u, v,
					u + (float)SPRITE_WIDTH, v + (float)SPRITE_HEIGHT,
					u + (float)SPRITE_WIDTH, v
				});
			}
		}
	}
	return true;
}



bool readEntityData(ifstream &stream, GameState& game) {
	string line;
	string type;
	GLuint texID = LoadTexture();

	glBindTexture(GL_TEXTURE_2D, texID);

	while (getline(stream, line)) {
		if (line == "") break;
		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);

		if (key == "type") type = value;
		else if (key == "location") {
			istringstream lineStream(value);
			string xPosition, yPosition;
			getline(lineStream, xPosition, ',');
			getline(lineStream, yPosition, ',');

			float placeX = atoi(xPosition.c_str()) * TILE_SIZE;
			float placeY = atoi(yPosition.c_str()) * -TILE_SIZE;
			placeEntity(type, placeX, placeY, texID, game);
		}
	}
	
	
	return true;
}

void Input(GameState* game, Matrix& project, SDL_Event& event) {
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	if (keys[SDL_SCANCODE_LEFT]) {
		game->dynamics[0]->velocity.x = -30;
		
	}
	else if (keys[SDL_SCANCODE_RIGHT]) {
		game->dynamics[0]->velocity.x = 30;
	}
	else {
		game->dynamics[0]->velocity.x = 0;
	}
	if (keys[SDL_SCANCODE_SPACE] && game->dynamics[0]->collidedBottom) {
		game->dynamics[0]->velocity.y = 40.0f;
	}
}

void Update(GameState* game, float elapsed) {
	game->dynamics[0]->Update(elapsed);
	for (int i = 1; i < game->dynamics.size(); ++i)
		game->dynamics[0]->CollidesWith(*game->dynamics[i]);
}

void Render(GameState& game, ShaderProgram& program, GLuint sprites) {

	Matrix view = Matrix();
	view.Translate(-(game.dynamics[0]->position.x), 0.0f, 0.0f);

	for (int i = 0; i < vertexData.size() / 12; ++i) {
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
		modelview = modelview * view;
		program.SetModelviewMatrix(modelview);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);
	}

	for (int i = 0; i < game.dynamics.size(); ++i) {
		if (game.dynamics[i]->draw) {
			game.dynamics[i]->Render(view, program);
		}
	}

	

}

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#ifdef _WINDOWS
	glewInit();
#endif

	glViewport(0, 0, 640, 360);

	ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	SDL_Event event;

	GLuint sprites = LoadTexture();

	Matrix projectionMatrix;

	projectionMatrix.SetOrthoProjection(-56.0f, 56.0f, -32.0f, 32.0f, -1.0f, 1.0f);

	float lastFrameTicks = 0.0f;

	GameState game = GameState();

	ifstream infile("HW_4_b.txt");
	string line;
	while (getline(infile, line)) {
		if (line == "[header]") {
			if (!readHeader(infile)) {
				assert(false);
			}
		}
		else if (line == "[layer]") {
			readLayerData(infile);
			game = setup();
		}
		else if (line == "[Object Layer 1]") {
			readEntityData(infile, game);
		}
	}

	glUseProgram(program.programID);

	glBindTexture(GL_TEXTURE_2D, sprites);
	
	// Main game loop
	bool done = false;
	while (!done) {
		
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}
		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		glClearColor(0.5294f, 0.8078f, 0.98f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		 
		program.SetProjectionMatrix(projectionMatrix);
		
		Input(&game, projectionMatrix, event);
		Update(&game, elapsed);
		Render(game, program, sprites);

	
		SDL_GL_SwapWindow(displayWindow);

	}

	SDL_Quit();
	return 0;
}