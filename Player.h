//
// Created by david on 4/2/20.
//

#ifndef CONTRA_PLAYER_H
#define CONTRA_PLAYER_H

#define PLAYER_SPEED 110
#define PLAYER_JUMP 400
#define FALL_MAX_Y 200

#include <utility>

#include "component.h"
#include "game_object.h"
#include "AnimationRenderer.h"
#include "floor.h"
#include "Gravity.h"
#include "bullets.h"

class Player: public GameObject {

};

class PlayerControl: public Component {
public:
    void Init() override;
    void Update(float dt) override;
    void Create(AvancezLib *engine, GameObject *go, std::set<GameObject *> *game_objects,
            std::weak_ptr<Floor> floor, float* camera_x, ObjectPool<Bullet>* bullet_pool) {
        Component::Create(engine, go, game_objects);
        m_floor = std::move(floor);
        m_cameraX = camera_x;
        m_bulletPool = bullet_pool;
    }
    void Kill();
    void Respawn();
    [[nodiscard]] short getRemainingLives() const { return m_remainingLives; }
private:
    AnimationRenderer* m_animator;
    ObjectPool<Bullet>* m_bulletPool;
    Gravity* m_gravity;
    float* m_cameraX;
    std::weak_ptr<Floor> m_floor;
    bool m_hasInertia;
    bool m_hasShot;
    float m_waitDead, m_invincibleTime;
    bool m_isDeath;
    float m_shootDowntime;
    bool m_facingRight;
    bool m_wasInWater;
    short m_remainingLives;
    int m_idleAnim, m_upAnim, m_crawlAnim,
    m_runAnim, m_runUpAnim, m_runDownAnim,
    m_jumpAnim, m_dieAnim, m_runShootAnim,
    m_splashAnim, m_swimAnim, m_diveAnim,
    m_swimShootAnim, m_swimShootDiagonalAnim,
    m_swimShootUpAnim, m_fallAnim;

    void Fire(const AvancezLib::KeyStatus &keyStatus);
};

#endif //CONTRA_PLAYER_H
