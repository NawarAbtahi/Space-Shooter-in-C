#include "SDL2/SDL_error.h"
#include "SDL2/SDL_events.h"
#include "SDL2/SDL_keyboard.h"
#include "SDL2/SDL_keycode.h"
#include "SDL2/SDL_rect.h"
#include "SDL2/SDL_render.h"
#include "SDL2/SDL_scancode.h"
#include "SDL2/SDL_surface.h"
#include "SDL2/SDL_video.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "./spaceProperties.h"

SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *playerImage;
SDL_Texture *asteroidImage;
SDL_Event event;
SDL_Rect player;
SDL_Rect playerPosition;

//projectile struct
typedef struct{
    SDL_Rect projectileBody;
    bool active;
} Projectile;

Projectile projectiles[MAX_PROJECTILES];

float projectileCooldown = 0.25f; // 250 ms cooldown
float lastFiredTime = 0.0f;

float projectileMoveSpeed = 450.0f;

//asteroid struct
typedef struct{
  SDL_Rect asteroidBody;
  SDL_Rect asteroidPosition; 
  int asteroidTextureWidth, asteroidTextureHeight;
  int speed;
} Asteroid;

Asteroid asteroids[MAX_ASTEROIDS];

float asteroidSpawnTimer = 0.0f;
const float asteroidSpawnDelay = 10.0f;
int asteroidCount = 0;

//fps properties
const int fps = 20;
float frameTime = 0.0f;
int prevTime = 0;
int currentTime = 0;
float deltaTime = 0.0f;
float moveSpeed = 250.0f;

int frameWidth, frameHeight;
int textureWidth, textureHeight;

bool projectileActive = false;

//keystate
const Uint8 *keyState;

//function prototypes
void playerMovement();
void checkBorderCollision();
void initProjectiles();
void updateProjectiles();
void fireProjectile();
void drawProjectile();
void moveAsteroid();
void asteroidProjectileCollision();
void asteroidPlayerCollision();
SDL_Texture *LoadTexture(char *filepath, SDL_Renderer *renderTarget);

int main(int argc, char *args[]){

  bool gameRunning = true;
  //initializing SDL
  if(SDL_Init(SDL_INIT_VIDEO) < 0){
    printf("Error initializing SDL: %s\n", SDL_GetError());
  }

  //initializing window
  window = SDL_CreateWindow(
      "SpaceShooter",
      SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED,
      WINDOW_WIDTH,
      WINDOW_HEIGHT,
      0);
  if(!window){
    printf("Error creating window: %s\n", SDL_GetError());
  }

  //initializing renderer
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if(!renderer){
    printf("error creating renderer: %s\n", SDL_GetError());
  }
  
  //player image and properties
  playerImage = LoadTexture("../src/img/spaceShipSprite.png", renderer);

  SDL_QueryTexture(playerImage, NULL, NULL, &textureWidth, &textureHeight);

  playerPosition.x = (WINDOW_WIDTH / 2) - player.w;
  playerPosition.y = (WINDOW_HEIGHT / 2) + 200;
  playerPosition.w = playerPosition.h = PLAYER_SIZE;
  
  frameWidth = textureWidth / 4;
  frameHeight = textureHeight;

  player.x = player.y = 0;
  player.w = frameWidth;
  player.h = frameHeight;
  
  //asteroid image and properties
  asteroidImage = LoadTexture("../src/img/asteroid.png", renderer);

  srand(time(0));
  for(int i = 0; i < MAX_ASTEROIDS; i++){
    SDL_QueryTexture(asteroidImage, NULL, NULL, &asteroids[i].asteroidTextureWidth, &asteroids[i].asteroidTextureHeight);
    asteroids[i].asteroidPosition.x = rand() % 751;
    asteroids[i].asteroidPosition.y = 0;
    asteroids[i].asteroidPosition.w = asteroids[i].asteroidPosition.h = ASTEROID_SIZE;

    asteroids[i].asteroidBody.x = asteroids[i].asteroidBody.y = 0;
    asteroids[i].asteroidBody.w = asteroids[i].asteroidTextureWidth;
    asteroids[i].asteroidBody.h = asteroids[i].asteroidTextureHeight;
    asteroids[i].speed = (rand() % 101) + 250;
  }
  
  initProjectiles();


  //game loop
  while (gameRunning) {
    //background color
    SDL_SetRenderDrawColor(renderer, 21, 23, 27, 255);
    SDL_RenderClear(renderer);
    
    //fps
    prevTime = currentTime;
    currentTime = SDL_GetTicks();
    deltaTime = (currentTime - prevTime) / 1000.0f;

    //events (closing the window)
    while (SDL_PollEvent(&event) != 0) {

      if(event.type == SDL_QUIT){
        gameRunning = false;
      }

    }

    asteroidSpawnTimer += deltaTime;
    
    //asteroid spawning cooldown
    if(asteroidSpawnTimer >= asteroidSpawnDelay && asteroidCount < MAX_ASTEROIDS){
      asteroids[asteroidCount].asteroidPosition.x = rand() % 751;
      asteroids[asteroidCount].asteroidPosition.y = 0;
      asteroids[asteroidCount].asteroidPosition.w = asteroids[asteroidCount].asteroidPosition.h = 34;

      asteroids[asteroidCount].asteroidBody.x = asteroids[asteroidCount].asteroidBody.y = 0;
      asteroids[asteroidCount].asteroidBody.w = asteroids[asteroidCount].asteroidTextureWidth;
      asteroids[asteroidCount].asteroidBody.h = asteroids[asteroidCount].asteroidTextureHeight;

      asteroids[asteroidCount].speed = (rand() % 101) + 250;

      asteroidCount++;
      asteroidSpawnTimer = 10.0f;
    }

    playerMovement(); 
    drawProjectile();
    moveAsteroid();

    //drawing the player
    SDL_RenderCopy(renderer, playerImage, &player, &playerPosition);
    
    //rendering the asteroids
    for(int i = 0; i < MAX_ASTEROIDS; i++){
      SDL_RenderCopy(renderer, asteroidImage, &asteroids[i].asteroidBody, &asteroids[i].asteroidPosition);
    }

    SDL_RenderPresent(renderer);
    
    asteroidProjectileCollision();
    asteroidPlayerCollision();
    checkBorderCollision();
  }

  SDL_DestroyWindow(window);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyTexture(playerImage);
  SDL_DestroyTexture(asteroidImage);
  window = NULL;
  renderer = NULL;
  playerImage = NULL;
  asteroidImage = NULL;
  SDL_Quit();
  return 0;
}

//For creating images
SDL_Texture *LoadTexture(char *filepath, SDL_Renderer *renderTarget){
  SDL_Texture *texture = NULL;
  SDL_Surface *surface = IMG_Load(filepath);

  if (!surface) {
    printf("error making surface: %s\n", SDL_GetError());
  }
  
  texture = SDL_CreateTextureFromSurface(renderTarget, surface);

  if (!texture) {
    printf("error making optimizedsurface: %s\n", SDL_GetError());
  }

  SDL_FreeSurface(surface);
  return texture;  
}

//Initializes player movement
void playerMovement(){
  keyState = SDL_GetKeyboardState(NULL);

  if(keyState[SDL_SCANCODE_D]){
    playerPosition.x += moveSpeed * deltaTime;
  }
  else if (keyState[SDL_SCANCODE_A]) {
    playerPosition.x -= moveSpeed * deltaTime;
  }

  frameTime += deltaTime;
  if(frameTime >= 0.15f){
    frameTime = 0;
    player.x += frameWidth;

    if(player.x >= textureWidth)
      player.x = 0;
  }
}

//initializes projectiles
void initProjectiles() {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        projectiles[i].projectileBody.w = PROJECTILE_WIDTH;
        projectiles[i].projectileBody.h = PROJECTILE_HEIGHT;
        projectiles[i].active = false;
    }
}

//updates projectiles
void updateProjectiles() {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (projectiles[i].active) {
            projectiles[i].projectileBody.y -= projectileMoveSpeed * deltaTime;

            // Deactivate if out of bounds
            if (projectiles[i].projectileBody.y <= 0) {
                projectiles[i].active = false;
            }

            // Render the projectile
            SDL_SetRenderDrawColor(renderer, 199, 0, 57, 255);
            SDL_RenderFillRect(renderer, &projectiles[i].projectileBody);
        }
    }
}

//firing the projectiles (logic)
void fireProjectile() {
    if (currentTime - lastFiredTime >= projectileCooldown * 1000) { // cooldown in milliseconds
        for (int i = 0; i < MAX_PROJECTILES; i++) {
            if (!projectiles[i].active) {
                projectiles[i].projectileBody.x = playerPosition.x + playerPosition.w / 2 - projectiles[i].projectileBody.w / 2;
                projectiles[i].projectileBody.y = playerPosition.y;
                projectiles[i].projectileBody.w = PROJECTILE_WIDTH;
                projectiles[i].projectileBody.h = PROJECTILE_HEIGHT;
                projectiles[i].active = true;
                lastFiredTime = currentTime;
                break;
            }
        }
    }
}

//draws the projectiles and handles all of the logics
void drawProjectile() {
    keyState = SDL_GetKeyboardState(NULL);

    if (keyState[SDL_SCANCODE_SPACE]) {
        fireProjectile();
    }

    updateProjectiles();
}

//checks if players hits the edge of a screen and thereby stops the player going out of the screen
void checkBorderCollision(){

  if(playerPosition.x + playerPosition.w >= WINDOW_WIDTH){
    playerPosition.x = WINDOW_WIDTH - playerPosition.w;
  }

  else if (playerPosition.x<= 0) {
    playerPosition.x = 0;
  }

}

void moveAsteroid(){

  for(int i = 0; i < MAX_ASTEROIDS; i++){
    asteroids[i].asteroidPosition.y += asteroids[i].speed * deltaTime;

    if(asteroids[i].asteroidPosition.y >= WINDOW_HEIGHT - asteroids[i].asteroidPosition.h){
      asteroids[i].asteroidPosition.y = 0;
      asteroids[i].asteroidPosition.x = rand() % ((WINDOW_WIDTH + 1) - ASTEROID_SIZE);
      asteroids[i].speed = (rand() % 101) + 250; 
    }
  }

}

void asteroidProjectileCollision(){
  for(int i = 0; i < MAX_PROJECTILES; i++){
    for(int j = 0; j < MAX_ASTEROIDS; j++){
      if(projectiles[i].active){
        if(projectiles[i].projectileBody.x < asteroids[j].asteroidPosition.x + asteroids[j].asteroidPosition.w && 
            projectiles[i].projectileBody.x + projectiles[i].projectileBody.w > asteroids[j].asteroidPosition.x &&
            projectiles[i].projectileBody.y < asteroids[j].asteroidPosition.y + asteroids[j].asteroidPosition.h && 
            projectiles[i].projectileBody.y + projectiles[i].projectileBody.h > asteroids[j].asteroidPosition.y){
          asteroids[j].asteroidPosition.y = 0;
          projectiles[i].active = false;
       
          asteroids[j].asteroidPosition.y = 0;
          asteroids[j].asteroidPosition.x = rand() % ((WINDOW_WIDTH + 1) - ASTEROID_SIZE);
          asteroids[j].speed = (rand() % 101) + 250; 
          break;
        }      
      }
    }
  }
}

void asteroidPlayerCollision(){
  for(int i = 0; i < MAX_ASTEROIDS; i++){
    if(asteroids[i].asteroidPosition.y + asteroids[i].asteroidPosition.h >= playerPosition.y &&
      asteroids[i].asteroidPosition.y <= playerPosition.y + playerPosition.h &&
      asteroids[i].asteroidPosition.x + asteroids[i].asteroidPosition.w >= playerPosition.x &&
      asteroids[i].asteroidPosition.x <= playerPosition.x + playerPosition.w){

      asteroids[i].asteroidPosition.y = 0;
      asteroids[i].asteroidPosition.x = rand() % ((WINDOW_WIDTH + 1) - ASTEROID_SIZE);
      asteroids[i].speed = (rand() % 101) + 250;

      playerPosition.x = (WINDOW_WIDTH / 2) - player.w;
      playerPosition.y = (WINDOW_HEIGHT / 2) + 200;
    }
  }
}
