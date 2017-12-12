//Old

#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <SDL_mixer.h>

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
#define GRAVITY -0.01
#define FIXED_TIMESTEP 0.166666f

using namespace std;

enum GameMode { STATE_TITLE, STATE_GAME, STATE_PAUSE, STATE_VIC };

GameMode mode = STATE_TITLE;



SDL_Window* displayWindow;


// Static tile rendering information
unsigned int** levelData;
int mapHeight, mapWidth;
vector<float> vertexData;
vector<float> texCoordData;
int winner;

GLuint LoadSprites() {
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


GLuint LoadText() {
	int w, h, comp;
	unsigned char* image = stbi_load(RESOURCE_FOLDER"font1.png", &w, &h, &comp, STBI_rgb_alpha);
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
	float texture_size = 1.0 / 16.0f;	// the full png contains 16 x 16  char textures
	vector<float> textVertexData;
	vector<float> textTexCoordData;

	// Prepare the character and its proportions in-game
	for (int i = 0; i < text.size(); i++) {
		int spriteIndex = (int)text[i];

		float texture_x = (float)(spriteIndex % 16) / 16.0f;
		float texture_y = (float)(spriteIndex / 16) / 16.0f;

		textVertexData.insert(textVertexData.end(), {
			((size + spacing) * i) + (-0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
		});

		textTexCoordData.insert(textTexCoordData.end(), {
			texture_x, texture_y,
			texture_x, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x + texture_size, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x, texture_y + texture_size,
		});
	}

	glBindTexture(GL_TEXTURE_2D, fontTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// Render each text in the string

	for (int i = 0; i < textVertexData.size() / 12; ++i) {
		float vertices[12];
		float texCoords[12];

		for (int j = 0; j < 12; ++j) {
			vertices[j] = *(textVertexData.data() + (i * 12) + j);
			texCoords[j] = *(textTexCoordData.data() + (i * 12) + j);
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


void worldToTileCoordinates(float worldX, float worldY, int* gridX, int* gridY) {

	*gridX = (int)(worldX / TILE_SIZE);
	if (worldY > 0)
		worldY -= 64.0f;
	*gridY = (int)((-worldY) / TILE_SIZE);

}

class SheetSprite {
public:
	SheetSprite();
	SheetSprite(unsigned int textureID, float width, float height, float size);

	void Draw(Matrix& view, ShaderProgram* program, float x, float y, int index);

	float size;
	unsigned int textureID;
	float u;
	float v;
	float width;
	float height;
};

SheetSprite::SheetSprite() :size(1), textureID(0) {}

SheetSprite::SheetSprite(unsigned int textureID, float width, float height, float size) :
	textureID(textureID), width(width), height(height), size(size) {}

void SheetSprite::Draw(Matrix& view, ShaderProgram* program, float x, float y, int index) {

	float u = (float)((index) % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X;
	float v = (float)((index) / SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y;
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

Vector3::Vector3() :x(0), y(0), z(0) {}

Vector3::Vector3(float x, float y, float z) : x(x), y(y), z(z) {}


class Entity {
public:

	Entity();

	Entity(Vector3 position, Vector3 size);

	bool Update(float elapsed, bool gameNum, Mix_Chunk* thud);
	void Render(Matrix& view, ShaderProgram &program, int index);
	void CollidesWith(Entity& entity, Mix_Chunk* pickup);

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

Entity::Entity(Vector3 position, Vector3 size) : position(position), size(size), velocity(Vector3()), acceleration(Vector3()),
collidedTop(false), collidedBottom(false), collidedLeft(false), collidedRight(false), draw(true) {}

bool Entity::Update(float elapsed, bool gameNum, Mix_Chunk* thud) {
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

	int* gridX = new int(0);
	int* gridY = new int(0);

	// Check for collision on bottom

	worldToTileCoordinates(position.x, position.y - size.y / 2, gridX, gridY);
	if (levelData[*gridY][*gridX] == 100) {
		position.x = 6.1f;
		if (position.y > 0)
			position.y = 6.01f;
		else
			position.y = -58.0f;
		return true;
	}
	else if (levelData[*gridY][*gridX] != 0 && levelData[*gridY][*gridX] != 113 && levelData[*gridY][*gridX] != 114) {
		if (velocity.y < -0.1)
			Mix_PlayChannel(-1, thud, 0);
		float tile_center = (-TILE_SIZE * (*gridY) - TILE_SIZE / 2);
		float penetration = 0;
		if (gameNum)
			penetration = fabs(fabs(position.y - tile_center) - size.y / 2 - TILE_SIZE / 2);
		else
			penetration = fabs(fabs(position.y - 64.0f - tile_center) - size.y / 2 - TILE_SIZE / 2);
		position.y += penetration + 0.01;
		collidedBottom = true;
		velocity.y = 0;
		acceleration.y = 0;

	}

	// Check for collision on top

	worldToTileCoordinates(position.x, position.y + size.y / 2, gridX, gridY);

	if (levelData[*gridY][*gridX] != 0 && levelData[*gridY][*gridX] != 113 && levelData[*gridY][*gridX] != 114 && levelData[*gridY][*gridX] != 115) {
		float tile_center = (-TILE_SIZE * (*gridY) - TILE_SIZE / 2);
		float penetration = 0;
		if (gameNum)
			penetration = fabs(fabs(position.y - tile_center) - size.y / 2 - TILE_SIZE / 2);
		else
			penetration = fabs(fabs(position.y - 64.0f - tile_center) - size.y / 2 - TILE_SIZE / 2);
		position.y -= penetration + 0.1;
		velocity.y = 0;
		acceleration.y = 0;
		collidedTop = true;
	}
	// Check for collision on right


	worldToTileCoordinates(position.x + size.x / 2, position.y, gridX, gridY);



	if (levelData[*gridY][*gridX] != 0 && levelData[*gridY][*gridX] != 113 && levelData[*gridY][*gridX] != 114 && levelData[*gridY][*gridX] != 115) {
		float tile_center = -(-TILE_SIZE * (*gridX) - TILE_SIZE / 2);

		float penetration = fabs(fabs(position.x - tile_center) - size.x / 2 - TILE_SIZE / 2);
		if (velocity.x > 0) velocity.x = 0;
		position.x -= penetration + 0.01;
		collidedRight = true;
	}

	// Check for collision on left

	worldToTileCoordinates(position.x - size.x / 2, position.y, gridX, gridY);

	if (levelData[*gridY][*gridX] != 0 && levelData[*gridY][*gridX] != 113 && levelData[*gridY][*gridX] != 114 && levelData[*gridY][*gridX] != 115) {
		float tile_center = -(-TILE_SIZE * (*gridX) - TILE_SIZE / 2);
		float penetration = fabs(fabs(position.x - tile_center) - size.x / 2 - TILE_SIZE / 2);
		if (velocity.x < 0) velocity.x = 0;
		position.x += penetration + 0.1;
		collidedLeft = true;
	}

	position.x += velocity.x * elapsed;

	return false;

}

void Entity::Render(Matrix& view, ShaderProgram &program, int index) {
	if (index >= 80 && velocity.x == 0)
		sprite.Draw(view, &program, position.x, position.y, 80);
	else
		sprite.Draw(view, &program, position.x, position.y, index);
}

void Entity::CollidesWith(Entity& entity, Mix_Chunk* pickup) {
	if (size.x / 2 + entity.size.x / 2 > fabs((position.x + size.x / 2) - (entity.position.x + size.x / 2)) &&
		size.y / 2 + entity.size.y / 2 > fabs((position.y + size.y / 2) - (entity.position.y + size.y / 2))) {
		if (entity.draw == true)
			Mix_PlayChannel(-1, pickup, 0);
		entity.draw = false;

	}
}


class GameState {
public:
	GameState();
	vector<Entity*> dynamics;
};

GameState::GameState() {}

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
		index = 51;
	float u = (float)(((int)index) % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X;
	float v = (float)(((int)index) / SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y;
	SheetSprite new_sprite = SheetSprite(texID, SPRITE_WIDTH, SPRITE_HEIGHT, 4.1);
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



bool readEntityData(ifstream &stream, GameState& game_1, GameState& game_2) {
	string line;
	string type;
	GLuint spriteID = LoadSprites();

	glBindTexture(GL_TEXTURE_2D, spriteID);

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

			float placeX = atoi(xPosition.c_str()) * TILE_SIZE;	// What do I add here?
			float placeY = atoi(yPosition.c_str()) * -TILE_SIZE;
			placeEntity(type, placeX, placeY, spriteID, game_1);
			placeEntity(type, placeX, placeY + 64.0f, spriteID, game_2);
		}
	}


	return true;
}


void loadLevel(GameState&  game_1, GameState& game_2, const string& filename, vector<Mix_Music*>& musics) {
	vertexData.clear();
	texCoordData.clear();
	mode = STATE_GAME;
	if (filename == "HW_4_b.txt")
		Mix_PlayMusic(musics[1], -1);
	else if (filename == "level_2.txt")
		Mix_PlayMusic(musics[2], -1);
	else
		Mix_PlayMusic(musics[3], -1);


	ifstream infile(filename);
	string line;
	while (getline(infile, line)) {
		if (line == "[header]") {
			if (!readHeader(infile)) {
				assert(false);
			}
		}
		else if (line == "[layer]") {
			readLayerData(infile);
			game_1 = setup();
			game_2 = setup();
		}
		else if (line == "[Object Layer 1]") {
			readEntityData(infile, game_1, game_2);
		}
	}
}

void Input(GameState&  game_1, GameState& game_2, Matrix& project, SDL_Event& event, bool& done, vector<Mix_Music*>& musics, Mix_Chunk* jump) {
	const Uint8 *keys = SDL_GetKeyboardState(NULL);

	switch (mode) {
	case STATE_TITLE:
		if (keys[SDL_SCANCODE_1]) {
			loadLevel(game_1, game_2, "HW_4_b.txt", musics);
		}
		else if (keys[SDL_SCANCODE_2]) {
			loadLevel(game_1, game_2, "level_2.txt", musics);
		}
		else if (keys[SDL_SCANCODE_3]) {
			loadLevel(game_1, game_2, "level_3.txt", musics);
		}
		else if (keys[SDL_SCANCODE_ESCAPE]) {
			done = true;
		}
		break;
	case STATE_PAUSE:
		if (keys[SDL_SCANCODE_ESCAPE]) {
			done = true;
		}
		else if (keys[SDL_SCANCODE_M]) {
			mode = STATE_TITLE;
			Mix_PlayMusic(musics[0], -1);
		}
		break;
	case STATE_GAME:
		// Player 1
		if (keys[SDL_SCANCODE_LEFT]) {
			game_1.dynamics[0]->velocity.x = -0.3;

		}
		else if (keys[SDL_SCANCODE_RIGHT]) {
			game_1.dynamics[0]->velocity.x = 0.3f;
		}
		else {
			game_1.dynamics[0]->velocity.x = 0.0f;
		}
		if (keys[SDL_SCANCODE_UP] && game_1.dynamics[0]->collidedBottom) {
			Mix_PlayChannel(-1, jump, 0);
			game_1.dynamics[0]->velocity.y = 0.5f;
		}

		// Player 2
		if (keys[SDL_SCANCODE_A]) {
			game_2.dynamics[0]->velocity.x = -0.3;

		}
		else if (keys[SDL_SCANCODE_D]) {
			game_2.dynamics[0]->velocity.x = 0.3f;
		}
		else {
			game_2.dynamics[0]->velocity.x = 0.0f;
		}
		if (keys[SDL_SCANCODE_W] && game_2.dynamics[0]->collidedBottom) {
			Mix_PlayChannel(-1, jump, 0);
			game_2.dynamics[0]->velocity.y = 0.5f;
		}
		break;
	case STATE_VIC:
		if (keys[SDL_SCANCODE_1]) {
			loadLevel(game_1, game_2, "HW_4_b.txt", musics);
		}
		else if (keys[SDL_SCANCODE_2]) {
			loadLevel(game_1, game_2, "level_2.txt", musics);
		}
		else if (keys[SDL_SCANCODE_3]) {
			loadLevel(game_1, game_2, "level_3.txt", musics);
		}
		if (keys[SDL_SCANCODE_ESCAPE]) {
			done = true;
		}
		else if (keys[SDL_SCANCODE_M]) {
			mode = STATE_TITLE;
			Mix_PlayMusic(musics[0], -1);
		}
		break;
	}


}

void Update(GameState* game_1, GameState* game_2, float elapsed, vector<Mix_Chunk*>& chunks) {

	switch (mode) {
	case STATE_GAME:

		bool one_wins = true;
		bool two_wins = true;
		for (int i = 1; i < game_1->dynamics.size(); ++i) {
			if (game_1->dynamics[i]->draw)
				one_wins = false;
			if (game_2->dynamics[i]->draw)
				two_wins = false;
		}

		if (one_wins) {
			winner = 1;
			mode = STATE_VIC;
			return;
		}
		else if (two_wins) {
			winner = 0;
			mode = STATE_VIC;
			return;
		}



		if (game_1->dynamics[0]->Update(elapsed, true, chunks[1])) {
			for (int i = 1; i < game_1->dynamics.size(); ++i)
				game_1->dynamics[i]->draw = true;

		}
		for (int i = 1; i < game_1->dynamics.size(); ++i)
			game_1->dynamics[0]->CollidesWith(*game_1->dynamics[i], chunks[0]);




		//bool boolean = game_2->dynamics[0]->Update(elapsed, false, chunks[1]);

		if (game_2->dynamics[0]->Update(elapsed, false, chunks[1])) {
			//cout << boolean << endl;
			for (int i = 1; i < game_2->dynamics.size(); ++i)
				game_2->dynamics[i]->draw = true;
		}
		for (int i = 1; i < game_2->dynamics.size(); ++i)
			game_2->dynamics[0]->CollidesWith(*game_2->dynamics[i], chunks[0]);
	}


}


void renderTitle(ShaderProgram& program, GLuint texts) {
	drawText(program, texts, "RACING STONES", 16, -6.0f, -64.0f, 36.0f);
	drawText(program, texts, "TURBO", 24, -6.0f, -40.0f, 16.0f);
	drawText(program, texts, "Top", 12, -6.0f, -88.0f, 8.0f);
	drawText(program, texts, "W", 12, -6.0f, -82.0f, -2.0f);
	drawText(program, texts, "A S D", 12, -6.0f, -94.0f, -12.0f);
	drawText(program, texts, "Bottom", 12, -6.0f, 60.0f, 8.0f);
	drawText(program, texts, "Arrow", 12, -6.0f, 64.0f, -2.0f);
	drawText(program, texts, "Keys", 12, -6.0f, 68.0f, -12.0f);
	drawText(program, texts, "Press 1, 2, or 3", 8, -4.0f, -36.0f, -16.0f);
	drawText(program, texts, "to start that level!", 8, -4.0f, -40.0f, -24.0f);
	drawText(program, texts, "Press \"P\" to Pause", 8.0f, -2.0f, -52.0f, -40.0f);
	drawText(program, texts, "Press \"Esc\" to Quit", 8, -4.0f, -40.0f, -56.0f);
}


void renderPause(ShaderProgram& program, GLuint texts) {
	drawText(program, texts, "PAUSED", 24, -6.0f, -48.0f, 20.0f);
	drawText(program, texts, "Press \"P\" to Unpause", 6.0f, -2.0f, -40.0f, -20.0f);
	drawText(program, texts, "Press \"M\" for Menu", 6.0f, -2.0f, -36.0f, -38.0f);
	drawText(program, texts, "Press \"Esc\" to Quit", 8, -4.0f, -38.0f, -56.0f);
}

void renderGame(GameState& game_1, GameState& game_2, ShaderProgram& program, GLuint sprites, int coinIndex, int walkIndex) {

	glBindTexture(GL_TEXTURE_2D, sprites);
	Matrix view_1 = Matrix();
	view_1.Translate(-(game_1.dynamics[0]->position.x), 0.0f, 0.0f);


	Matrix view_2 = Matrix();
	view_2.Translate(-(game_2.dynamics[0]->position.x), 0.0f, 0.0f);


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
		modelview = modelview * view_1;
		program.SetModelviewMatrix(modelview);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);

		for (int j = 0; j < 12; ++j) {
			if (j % 2 == 1)
				vertices[j] = *(vertexData.data() + (i * 12) + j) + 64;
			else
				vertices[j] = *(vertexData.data() + (i * 12) + j);
			texCoords[j] = *(texCoordData.data() + (i * 12) + j);
		}

		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program.positionAttribute);

		modelview.Identity();
		modelview = modelview * view_2;
		program.SetModelviewMatrix(modelview);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);
	}

	game_1.dynamics[0]->Render(view_1, program, walkIndex);
	for (int i = 1; i < game_1.dynamics.size(); ++i) {
		if (game_1.dynamics[i]->draw) {
			game_1.dynamics[i]->Render(view_1, program, coinIndex);
		}
	}

	game_2.dynamics[0]->Render(view_2, program, walkIndex);
	for (int i = 1; i < game_2.dynamics.size(); ++i) {
		if (game_2.dynamics[i]->draw) {
			game_2.dynamics[i]->Render(view_2, program, coinIndex);
		}
	}
}

void renderVic(ShaderProgram& program, GLuint texts) {
	if (winner == 1)
		drawText(program, texts, "Rock Bottom Wins!", 16, -6.0f, -72.0f, 0.0f);
	else
		drawText(program, texts, "Sky Rock-ette Wins!", 16, -6.0f, -92.0f, 0.0f);
	drawText(program, texts, "Press 1, 2, or 3", 8, -4.0f, -36.0f, -16.0f);
	drawText(program, texts, "to start that level!", 8, -4.0f, -40.0f, -24.0f);
	drawText(program, texts, "Press \"M\" for Menu", 6.0f, -2.0f, -36.0f, -38.0f);
	drawText(program, texts, "Press \"Esc\" to Quit", 8, -4.0f, -38.0f, -56.0f);
}

void Render(GameState& game_1, GameState& game_2, ShaderProgram& program, GLuint sprites[], int coinIndex, int walkIndex) {
	switch (mode) {
	case STATE_TITLE:
		renderTitle(program, sprites[1]);
		break;
	case STATE_PAUSE:
		renderGame(game_1, game_2, program, sprites[0], coinIndex, walkIndex);
		renderPause(program, sprites[1]);
		break;
	case STATE_GAME:
		renderGame(game_1, game_2, program, sprites[0], coinIndex, walkIndex);
		break;
	case STATE_VIC:
		renderGame(game_1, game_2, program, sprites[0], coinIndex, walkIndex);
		renderVic(program, sprites[1]);
	}




}

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("RACING STONES TURBO", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#ifdef _WINDOWS
	glewInit();
#endif

	glViewport(0, 0, 1280, 720);

	ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	SDL_Event event;

	/*GLuint sprites = LoadSprites();
	GLuint text = LoadText();*/

	GLuint textures[] = { LoadSprites(), LoadText() };

	Matrix projectionMatrix;

	projectionMatrix.SetOrthoProjection(-112.0f, 112.0f, -64.0f, 64.0f, -1.0f, 1.0f);

	float lastFrameTicks = 0.0f;

	GameState game_1 = GameState();
	GameState game_2 = GameState();

	const int bling[] = { 51, 52, 53, 54 };
	const int coin_numFrames = 4;
	const int walk[] = { 80, 81 };
	const int walk_numFrames = 2;
	float coin_animationElapsed = 0.0f;
	float walk_animationElapsed = 0.0f;
	float framesPerSecond = 30.0f;
	int coin_currentIndex = 0;
	int walk_currentIndex = 0;


	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);

	Mix_Music *title_music;
	title_music = Mix_LoadMUS("level_title.mp3");

	Mix_Music *one_music;
	one_music = Mix_LoadMUS("level_one.mp3");

	Mix_Music *two_music;
	two_music = Mix_LoadMUS("level_two.mp3");

	Mix_Music *three_music;
	three_music = Mix_LoadMUS("level_three.mp3");

	Mix_Chunk *pickup;
	pickup = Mix_LoadWAV("pickup.wav");

	Mix_Chunk *jump;
	jump = Mix_LoadWAV("jump.wav");

	Mix_Chunk *thud;
	thud = Mix_LoadWAV("thud.wav");

	vector<Mix_Music*> musics = { title_music, one_music, two_music, three_music };

	vector<Mix_Chunk*> chunks = { pickup, thud };

	/*Mix_Chunk *hurt;
	hurt = Mix_LoadWAV("hurt.wav");*/

	Mix_PlayMusic(title_music, -1);


	glUseProgram(program.programID);

	glBindTexture(GL_TEXTURE_2D, textures[0]);


	// Main game loop

	bool done = false;
	float accumulator = 0.0f;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
			else if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.scancode == SDL_SCANCODE_P) {
					if (mode == STATE_GAME)
						mode = STATE_PAUSE;
					else if (mode == STATE_PAUSE)
						mode = STATE_GAME;
				}
			}
		}

		switch (mode) {
		case STATE_TITLE:
			//Mix_PlayMusic(title_music, -1);
			cout << "Playing Music" << endl;
			break;
		case STATE_GAME:
			//Mix_PlayMusic(one_music, -1);
			break;
		}


		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;



		glClearColor(0.5294f, 0.8078f, 0.98f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);


		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		program.SetProjectionMatrix(projectionMatrix);

		Input(game_1, game_2, projectionMatrix, event, done, musics, jump);
		if (mode != STATE_TITLE && mode != STATE_PAUSE) {
			glBindTexture(GL_TEXTURE_2D, textures[0]);

		}
		//Update(&game_1, &game_2, elapsed, chunks);

		elapsed += accumulator;

		if (elapsed < FIXED_TIMESTEP) {
			accumulator = elapsed;
			continue;
		}


		while (elapsed >= FIXED_TIMESTEP) {
			Update(&game_1, &game_2, FIXED_TIMESTEP, chunks);
			elapsed -= FIXED_TIMESTEP;
		}

		coin_animationElapsed += elapsed;
		walk_animationElapsed += elapsed;

		if (coin_animationElapsed > 1.0 / framesPerSecond) {
			coin_currentIndex++;
			coin_animationElapsed = 0.0;
			if (coin_currentIndex > coin_numFrames - 1) {
				coin_currentIndex = 0;
			}
		}

		if (walk_animationElapsed > 1.0 / framesPerSecond) {
			walk_currentIndex++;
			walk_animationElapsed = 0.0;
			if (walk_currentIndex > walk_numFrames - 1) {
				walk_currentIndex = 0;
			}
		}

		Render(game_1, game_2, program, textures, bling[coin_currentIndex], walk[walk_currentIndex]);


		SDL_GL_SwapWindow(displayWindow);

	}

	Mix_FreeMusic(title_music);
	Mix_FreeMusic(one_music);
	SDL_Quit();
	return 0;
}