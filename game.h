#pragma once

#include "AnimationRenderer.h"
#include "consts.h"
#include "floor.h"
#include "Player.h"
#include "SimpleRenderer.h"
#include "bullets.h"
#include "canons.h"
#include "enemies.h"

class Game : public GameObject {
public:
    struct PlayerStats {
        int score;
    };
private:
    GameObject *currentScene;
    AvancezLib *engine;
    std::shared_ptr<Sprite> spritesheet;
    std::shared_ptr<Sprite> enemies_spritesheet;
    std::shared_ptr<Sprite> pickups_spritesheet;

    bool paused;
    bool pause_pressed_before;
    bool can_continue;
    int current_level;
    PlayerStats stats[2];
    PlayerStats lastSavedStats[2];
    unsigned short players;
public:
    virtual void Create(AvancezLib *avancezLib);

    void Reset() {
        pause_pressed_before = false;
        paused = false;
        can_continue = true;
        memset(lastSavedStats, 0, 2 * sizeof(PlayerStats));
        memset(stats, 0, 2 * sizeof(PlayerStats));
    }

    int GetCurrentLevel() const;

    void SetCurrentLevel(int currentLevel);

    void SetPlayers(unsigned short value) {
        if (value > 2) value = 2;
        if (value < 1) value = 1;
        players = value;
    }

    [[nodiscard]] int GetPlayers() const { return players; }

    [[nodiscard]] const PlayerStats *GetPlayerStats() const {
        return stats;
    }

    /**
     * Resets the player stats to the last saved ones
     */
    void ResetPlayerStats() {
        memcpy(stats, lastSavedStats, sizeof(PlayerStats) * 2);
    }

    void Init() override {
        Enable();
        Reset();
        players = 1;
        currentScene->Init();
    }


    void Update(float dt) override {
        AvancezLib::KeyStatus keyStatus;
        engine->getKeyStatus(keyStatus);
        if (keyStatus.esc) {
            Destroy();
            engine->quit();
        }

        if (keyStatus.pause && !pause_pressed_before) {
            paused = !paused;
        }
        pause_pressed_before = keyStatus.pause;

        if (paused)
            dt = 0.f;

        if (currentScene)
            currentScene->Update(dt);
    }

    void Start(BaseScene *scene) {
        if (currentScene) {
            currentScene->Destroy();
            delete currentScene;
        }
        currentScene = scene;
    }

    virtual void Draw() {
        engine->swapBuffers();
        engine->clearWindow();
    }

    void Receive(Message m) override;

    void Destroy() override {
        SDL_Log("Game::Destroy");
        if (currentScene) currentScene->Destroy();
    }
};