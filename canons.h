//
// Created by david on 8/2/20.
//

#ifndef CONTRA_CANONS_H
#define CONTRA_CANONS_H

#include <memory>

#include "game_object.h"
#include "avancezlib.h"
#include "AnimationRenderer.h"
#include "Player.h"

class RotatingCanon : public GameObject {
public:
    void Create(AvancezLib *engine,std::set<GameObject *> **game_objects,
                const std::shared_ptr<Sprite> &enemies_spritesheet,
                float *camera_x, const Vector2D &pos, Player *player, ObjectPool<Bullet> *bullet_pool,
                Grid *grid, int burst_length);
};

class Gulcan : public GameObject {
public:
    void Create(AvancezLib *engine,std::set<GameObject *> **game_objects,
                const std::shared_ptr<Sprite> &enemies_spritesheet,
                float *camera_x, const Vector2D &pos, Player *player, ObjectPool<Bullet> *bullet_pool,
                Grid *grid);
};

class CanonBehaviour : public Component, public CollideComponentListener {
protected:
    enum CanonState {
        HIDDEN,
        SHOWING,
        SHOWN,
        HIDING
    };
    CanonState m_state;
    int m_dir, m_minDir, m_maxDir, m_defaultDir;
    float m_currentDirTime, m_fireRemainingCooldown, m_burstRemainingCooldown;
    int m_burstLength, m_shotBulletsInBurst;
    float m_burstCooldown, m_fireCooldown, m_rotationInterval;
    PlayerControl *m_player;
    AnimationRenderer *m_animator;
    int animHidden, animShowing, animDirsFirst, animDie;
    ObjectPool<Bullet> *m_bulletPool;
    int m_life;

    [[nodiscard]] Vector2D GetPlayerDir() const {
        return m_player->GetGameObject()->position
        - Vector2D(0, 18) - go->position; // Subtract 18 bc position is the feet
    }

    [[nodiscard]] int DirToInt(const Vector2D &dir) const {
        // In the original game, the real angles seem to be irregular
//        return (12 - // Counter-clockwise, preserving 0
//                int(fmod( // Between 0 and 3.1416
//                        atan2(-dir.y, dir.x) +
//                        6.2832 /*+ 0.2618*/, // we used to add 15º to correct
//                        6.2832) / 0.5236)) % 12;  //0.5236rad = 30deg
          float angle = atan2f(-dir.y, dir.x) / 3.1415 * 180;
          if (angle <= -155 || angle > 155) return 6;
          else if(angle <= -126) return 5;
          else if(angle <= -105) return 4;
          else if(angle <= -75) return 3;
          else if(angle <= -50) return 2;
          else if(angle <= -25) return 1;
          else if(angle <= 25) return 0;
          else if(angle <= 50) return 11;
          else if(angle <= 75) return 10;
          else if(angle <= 105) return 9;
          else if(angle <= 126) return 8;
          else return 7;
    }

    void Fire();
public:
    void Create(AvancezLib *engine, GameObject *go,std::set<GameObject *> **game_objects, Player *player,
                ObjectPool<Bullet> *bullet_pool, int min_dir, int max_dir, int m_defaultDir, float rotation_interval,
                int burst_length, float burst_cooldown, float shoot_cooldown);
    void Init() {
        Component::Init();
        m_animator = go->GetComponent<AnimationRenderer *>();
        animHidden = m_animator->FindAnimation("Closed");
        animShowing = m_animator->FindAnimation("Opening");
        animDirsFirst = m_animator->FindAnimation("Dir0");
        animDie = m_animator->FindAnimation("Dying");
        m_life = 8;
        m_fireRemainingCooldown = 0;
        m_burstRemainingCooldown = m_burstCooldown;
        m_shotBulletsInBurst = 0;
        m_state = HIDDEN;
    }
    void Update(float dt) final;
    virtual void UpdateHidden(const Vector2D& player_dir, float dt);
    void UpdateShowing(const Vector2D& player_dir, float dt);
    void UpdateShown(const Vector2D& player_dir, float dt);
    void UpdateHiding(const Vector2D& player_dir, float dt);
    void OnCollision(const CollideComponent &collider) override;
};

class GulcanBehaviour: public CanonBehaviour {
public:
    void UpdateHidden(const Vector2D &player_dir, float dt) override;
};

#endif //CONTRA_CANONS_H
