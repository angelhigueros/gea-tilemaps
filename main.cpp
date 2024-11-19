#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_image.h>
#include <vector>
#include <string>
#include <iostream>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const float GROUND_LEVEL = 500.0f;

struct Cloud {
    float x, y;
    float speed;
};

class Background {
public:
    Background(SDL_Renderer* renderer) : renderer(renderer) {
        SDL_Surface* tilesetSurface = IMG_Load("tileset.png");
        if (!tilesetSurface) {
            std::cerr << "Error cargando el tileset: " << IMG_GetError() << std::endl;
        }
        tileset = SDL_CreateTextureFromSurface(renderer, tilesetSurface);
        SDL_FreeSurface(tilesetSurface);
        if (!tileset) {
            std::cerr << "Error creando textura del tileset: " << SDL_GetError() << std::endl;
        }

        tilemap.resize(19);
        for (int i = 0; i < 19; ++i) {
            tilemap[i].resize(25, 0); 
        }

        for (int i = 17; i < 19; ++i) {
            for (int j = 0; j < 25; ++j) {
                tilemap[i][j] = 1; 
            }
        }

        for (int i = 15; i < 17; ++i) {
            for (int j = 5; j < 20; ++j) {
                tilemap[i][j] = 2;
            }
        }

        clouds = {
            {50, 100, 30},
            {300, 150, 20},
            {600, 120, 25}
        };
    }

    ~Background() {
        SDL_DestroyTexture(tileset);
    }

    void renderBackground() {
        int tileWidth = 32;
        int tileHeight = 32;
        int tilesetColumns = 4; 

        for (int i = 0; i < tilemap.size(); ++i) {
            for (int j = 0; j < tilemap[i].size(); ++j) {
                int tileID = tilemap[i][j];
                SDL_Rect srcRect = {
                    (tileID % tilesetColumns) * tileWidth,
                    (tileID / tilesetColumns) * tileHeight,
                    tileWidth,
                    tileHeight
                };
                SDL_Rect dstRect = {
                    j * tileWidth,
                    i * tileHeight,
                    tileWidth,
                    tileHeight
                };
                SDL_RenderCopy(renderer, tileset, &srcRect, &dstRect);
            }
        }
    }

    void renderClouds(float deltaTime) {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        for (auto& cloud : clouds) {
            cloud.x += cloud.speed * deltaTime;
            if (cloud.x > WINDOW_WIDTH) {
                cloud.x = -100;
            }
            SDL_Rect cloudRect = {static_cast<int>(cloud.x), static_cast<int>(cloud.y), 100, 50};
            SDL_RenderFillRect(renderer, &cloudRect);
        }
    }

private:
    SDL_Renderer* renderer;
    SDL_Texture* tileset;
    std::vector<std::vector<int>> tilemap;
    std::vector<Cloud> clouds;
};

class Character {
public:
    Character(SDL_Renderer* renderer, float x, float y, Mix_Chunk* jumpSound)
        : renderer(renderer), x(x), y(y), originalY(y), velocity(0.0f), isJumping(false), jumpSound(jumpSound) {}

    void startJump() {
        if (!isJumping) {
            isJumping = true;
            velocity = initialJumpVelocity;
            Mix_PlayChannel(-1, jumpSound, 0);
        }
    }

    void moveLeft(float deltaTime) {
        x -= horizontalSpeed * deltaTime;
        if (x < 0) x = 0;
    }

    void moveRight(float deltaTime) {
        x += horizontalSpeed * deltaTime;
        if (x > WINDOW_WIDTH - characterWidth) x = WINDOW_WIDTH - characterWidth;
    }

    void update(float deltaTime) {
        if (isJumping) {
            velocity += gravity * deltaTime;
            y += velocity * deltaTime;
            if (y >= originalY) {
                y = originalY;
                isJumping = false;
                velocity = 0.0f;
            }
        }
    }

    void renderCharacter() {
        int renderX = static_cast<int>(x);
        int renderY = static_cast<int>(y);

        SDL_SetRenderDrawColor(renderer, 255, 223, 0, 255);
        SDL_Rect crownRect = {renderX + 8, renderY - 8, 16, 8};
        SDL_RenderFillRect(renderer, &crownRect);

        SDL_SetRenderDrawColor(renderer, 255, 182, 193, 255);
        SDL_Rect headRect = {renderX + 8, renderY, 16, 16};
        SDL_RenderFillRect(renderer, &headRect);

        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        SDL_Rect bodyRect = {renderX + 8, renderY + 16, 16, 24};
        SDL_RenderFillRect(renderer, &bodyRect);

        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_Rect leftArmRect = {renderX, renderY + 16, 8, 16};
        SDL_Rect rightArmRect = {renderX + 24, renderY + 16, 8, 16};
        SDL_RenderFillRect(renderer, &leftArmRect);
        SDL_RenderFillRect(renderer, &rightArmRect);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_Rect leftLegRect = {renderX + 8, renderY + 40, 8, 16};
        SDL_Rect rightLegRect = {renderX + 16, renderY + 40, 8, 16};
        SDL_RenderFillRect(renderer, &leftLegRect);
        SDL_RenderFillRect(renderer, &rightLegRect);
    }

    float getX() const { return x; }
    float getY() const { return y; }

private:
    SDL_Renderer* renderer;
    float x, y, originalY, velocity;
    bool isJumping;
    const float gravity = 2000.0f;
    const float initialJumpVelocity = -900.0f;
    const float horizontalSpeed = 800.0f;
    const int characterWidth = 32;

    Mix_Chunk* jumpSound;
};

class Dog {
public:
    Dog(SDL_Renderer* renderer, float x, float y, Mix_Chunk* dieSound)
        : renderer(renderer), x(x), y(y), movingRight(true), dieSound(dieSound) {}

    void update(float deltaTime, float playerX, float playerY) {
        if (movingRight) {
            x += speed * deltaTime;
            if (x > WINDOW_WIDTH - dogWidth) movingRight = false;
        } else {
            x -= speed * deltaTime;
            if (x < 0) movingRight = true;
        }

        if (abs(x - playerX) < dogWidth && abs(y - playerY) < 32) {
            Mix_PlayChannel(-1, dieSound, 0);
        }
    }

    void renderDog() {
        int renderX = static_cast<int>(x);
        int renderY = static_cast<int>(y);

        SDL_SetRenderDrawColor(renderer, 139, 69, 19, 255);
        SDL_Rect headRect = {renderX, renderY, 16, 16};
        SDL_RenderFillRect(renderer, &headRect);

        SDL_SetRenderDrawColor(renderer, 160, 82, 45, 255);
        SDL_Rect bodyRect = {renderX - 8, renderY + 16, 32, 16};
        SDL_RenderFillRect(renderer, &bodyRect);

        SDL_SetRenderDrawColor(renderer, 139, 69, 19, 255);
        SDL_Rect leftLegRect = {renderX - 6, renderY + 32, 8, 8};
        SDL_Rect rightLegRect = {renderX + 14, renderY + 32, 8, 8};
        SDL_RenderFillRect(renderer, &leftLegRect);
        SDL_RenderFillRect(renderer, &rightLegRect);

        SDL_SetRenderDrawColor(renderer, 160, 82, 45, 255);
        SDL_Rect tailRect = {renderX - 10, renderY + 20, 8, 4};
        SDL_RenderFillRect(renderer, &tailRect);
    }

private:
    SDL_Renderer* renderer;
    float x, y;
    bool movingRight;
    const float speed = 200.0f;
    const int dogWidth = 32;

    Mix_Chunk* dieSound;
};

class DialogSystem {
public:
    DialogSystem(SDL_Renderer* renderer, TTF_Font* font)
        : renderer(renderer), font(font), dialogIndex(0) {
        dialogs = {
            "Aventurero, salvameee, estoy atrapada en el castillo.",
            "Supera los desafios para poder salvarme.",
            "Cuento contigo, suerte."
        };
    }

    void advanceDialog() {
        if (dialogIndex < dialogs.size() - 1) {
            dialogIndex++;
        }
    }

    void renderDialog() {
        SDL_Color color = {255, 255, 255};
        SDL_Surface* surface = TTF_RenderText_Blended(font, dialogs[dialogIndex].c_str(), color);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

        int textWidth, textHeight;
        SDL_QueryTexture(texture, NULL, NULL, &textWidth, &textHeight);
        SDL_Rect textRect = {20, WINDOW_HEIGHT - 50, textWidth, textHeight};

        SDL_RenderCopy(renderer, texture, NULL, &textRect);

        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }

private:
    SDL_Renderer* renderer;
    TTF_Font* font;
    std::vector<std::string> dialogs;
    int dialogIndex;
};

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "Error inicializando SDL: " << SDL_GetError() << std::endl;
        return -1;
    }

    if (TTF_Init() < 0) {
        std::cerr << "Error inicializando SDL_ttf: " << TTF_GetError() << std::endl;
        return -1;
    }

    if (IMG_Init(IMG_INIT_PNG) == 0) {
        std::cerr << "Error inicializando SDL_image: " << IMG_GetError() << std::endl;
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("Castle Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    TTF_Font* font = TTF_OpenFont("fuente.ttf", 24);
    if (!font) {
        std::cerr << "Error cargando la fuente: " << TTF_GetError() << std::endl;
        return -1;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "Error inicializando SDL_mixer: " << Mix_GetError() << std::endl;
        return -1;
    }

    Mix_Music* backgroundMusic = Mix_LoadMUS("fondo.wav");
    Mix_Chunk* jumpSound = Mix_LoadWAV("salto.wav");
    Mix_Chunk* dieSound = Mix_LoadWAV("die.wav");

    if (!backgroundMusic || !jumpSound || !dieSound) {
        std::cerr << "Error cargando sonidos: " << Mix_GetError() << std::endl;
        return -1;
    }

    Mix_PlayMusic(backgroundMusic, -1);

    Background background(renderer);
    Character character(renderer, 350.0f, GROUND_LEVEL, jumpSound);
    Dog dog(renderer, 100.0f, GROUND_LEVEL, dieSound);
    DialogSystem dialogSystem(renderer, font);

    bool running = true;
    Uint32 lastTime = SDL_GetTicks();
    SDL_Event event;

    while (running) {
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_UP) {
                    character.startJump();
                }
            }

            const Uint8* keystate = SDL_GetKeyboardState(NULL);
            if (keystate[SDL_SCANCODE_RIGHT]) {
                character.moveRight(deltaTime);
            }
            if (keystate[SDL_SCANCODE_LEFT]) {
                character.moveLeft(deltaTime);
            }
            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                dialogSystem.advanceDialog();
            }
        }

        character.update(deltaTime);
        dog.update(deltaTime, character.getX(), character.getY());

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        background.renderBackground();
        background.renderClouds(deltaTime);
        character.renderCharacter();
        dog.renderDog();
        dialogSystem.renderDialog();

        SDL_RenderPresent(renderer);
    }

    Mix_FreeMusic(backgroundMusic);
    Mix_FreeChunk(jumpSound);
    Mix_FreeChunk(dieSound);
    Mix_CloseAudio();

    TTF_CloseFont(font);
    TTF_Quit();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    IMG_Quit();
    SDL_Quit();
    return 0;
}
