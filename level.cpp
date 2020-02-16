//
// Created by david on 14/2/20.
//

#include "level.h"
#include "canons.h"
#include "enemies.h"
#include <SDL_log.h>
#include "yaml_converters.h"
#include "pickups.h"
#include "component.h"
#include "bullets.h"
#include "Player.h"

void Level::Update(float dt) {
    AvancezLib::KeyStatus keys{};
    engine->getKeyStatus(keys);
    if (keys.esc) {
        Destroy();
        engine->quit();
    }

    if (player->position.x < level_width - WINDOW_WIDTH) {
        if (player->position.x > camera_x + WINDOW_WIDTH / 2.)
            camera_x = (float) player->position.x - WINDOW_WIDTH / 2.;
    } else if (camera_x < level_width - WINDOW_WIDTH)
        camera_x += PLAYER_SPEED * PIXELS_ZOOM * 2 * dt;
    else
        camera_x = level_width - WINDOW_WIDTH;

    // We progressively init the enemies in front of the camera
    while (camera_x + WINDOW_WIDTH + RENDERING_MARGINS > next_enemy_x && !not_found_enemies.empty()) {
        auto *enemy = not_found_enemies.top();
        game_objects[RENDERING_LAYER_ENEMIES]->insert(enemy);
        enemy->Init();
        not_found_enemies.pop();
        next_enemy_x = not_found_enemies.top()->position.x;
    }

    // And eliminate the enemies behind the camera
    std::set<GameObject *>::iterator it = game_objects[RENDERING_LAYER_ENEMIES]->begin();
    while (it != game_objects[RENDERING_LAYER_ENEMIES]->end()) {
        auto *game_object = *it;
        if (game_object->position.x < camera_x - RENDERING_MARGINS) {
            it = game_objects[RENDERING_LAYER_ENEMIES]->erase(it);
            if (game_object->onRemoval == DESTROY) {
                game_object->Destroy();
            }
        } else {
            it++;
        }
    }

    // Draw background (smoothing the zoom)
    int camera_without_zoom = int(floor(camera_x / PIXELS_ZOOM));
    int reminder = int(round(camera_x - camera_without_zoom * PIXELS_ZOOM));
    background->draw(-reminder, 0, WINDOW_WIDTH + PIXELS_ZOOM, WINDOW_HEIGHT,
            camera_without_zoom, 0, WINDOW_WIDTH / PIXELS_ZOOM + 1,
            WINDOW_HEIGHT / PIXELS_ZOOM);

    for (int i = 1; i <= playerControl->getRemainingLives(); i++) {
        spritesheet->draw(
                ((3 - i) * (LIFE_SPRITE_WIDTH + LIFE_SPRITE_MARGIN) + LIFE_SPRITE_MARGIN) * PIXELS_ZOOM,
                9 * PIXELS_ZOOM,
                LIFE_SPRITE_WIDTH * PIXELS_ZOOM, LIFE_SPRITE_HEIGHT * PIXELS_ZOOM,
                LIFE_SPRITE_X, LIFE_SPRITE_Y, LIFE_SPRITE_WIDTH, LIFE_SPRITE_HEIGHT
        );
    }

    grid.ClearCollisionCache(); // Clear collision cache
    for (const auto &layer: game_objects) {
        // Update objects which are enabled and not to be removed
        for (auto *game_object : *layer)
            if (game_object->IsEnabled() && !game_object->IsMarkedToRemove())
                game_object->Update(dt);
    }

    // Delete objects marked to remove
    for (const auto &layer: game_objects) {
        it = layer->begin();
        while (it != layer->end()) {
            auto *game_object = *it;
            if (game_object->IsMarkedToRemove()) {
                it = layer->erase(it);
                game_object->UnmarkToRemove();
                if (game_object->onRemoval == DESTROY) {
                    game_object->Destroy();
                }
            } else {
                it++;
            }
        }
    }

    // Add new ones
    while (!game_objects_to_add.empty()) {
        std::pair<GameObject *, int> next = game_objects_to_add.front();
        game_objects_to_add.pop();
        game_objects[next.second]->insert(next.first);
    }
}

void Level::Create(const std::string &folder, const std::shared_ptr<Sprite> &player_spritesheet,
                   const std::shared_ptr<Sprite> &enemy_spritesheet, const std::shared_ptr<Sprite> &pickup_spritesheet,
                   AvancezLib *avancezLib) {
    SDL_Log("Level::Create");
    this->engine = avancezLib;
    spritesheet = player_spritesheet;
    enemies_spritesheet = enemy_spritesheet;
    pickups_spritesheet = pickup_spritesheet;
    try {
        YAML::Node scene_root = YAML::LoadFile(folder + "/level.yaml");

        background.reset(engine->createSprite((folder + scene_root["background"].as<std::string>()).data()));
        level_width = background->getWidth() * PIXELS_ZOOM;
        level_floor = std::make_shared<Floor>((folder + scene_root["floor_mask"].as<std::string>()).data());
        grid.Create(34 * PIXELS_ZOOM, level_width, WINDOW_HEIGHT);

        CreateBulletPools();
        CreatePlayer();

        for (const auto &rc_node: scene_root["rotating_canons"]) {
            auto *tank = new RotatingCanon();
            tank->Create(this,
                    rc_node["pos"].as<Vector2D>() * PIXELS_ZOOM,
                    rc_node["burst_length"].as<int>());
            tank->AddReceiver(this);
            not_found_enemies.push(tank);
        }
        for (const auto &rc_node: scene_root["gulcans"]) {
            auto *gulcan = new Gulcan();
            gulcan->Create(this, rc_node["pos"].as<Vector2D>() * PIXELS_ZOOM);
            gulcan->AddReceiver(this);
            not_found_enemies.push(gulcan);
        }
        for (const auto &rc_node: scene_root["ledders"]) {
            auto *ledder = new Ledder();
            ledder->Create(this,
                    rc_node["time_hidden"].as<float>(),
                    rc_node["time_shown"].as<float>(),
                    rc_node["cooldown_time"].as<float>(),
                    rc_node["show_standing"].as<bool>(),
                    rc_node["burst_length"].as<int>(),
                    rc_node["burst_cooldown"].as<float>(),
                    rc_node["horizontally_precise"].as<bool>()
            );
            ledder->position = rc_node["pos"].as<Vector2D>() * PIXELS_ZOOM;
            ledder->AddReceiver(this);
            not_found_enemies.push(ledder);
        }
        for (const auto &rc_node: scene_root["greeders"]) {
            if (rc_node["chance_skip"]) {
                if (m_random_dist(m_mt) < rc_node["chance_skip"].as<float>())
                    continue;
            }
            auto *spawner = new GameObject();
            spawner->position = rc_node["pos"].as<Vector2D>() * PIXELS_ZOOM;
            auto *greeder_spawner = new GreederSpawner();
            greeder_spawner->Create(this, spawner, m_random_dist(m_mt));
            spawner->AddComponent(greeder_spawner);
            spawner->AddReceiver(this);
            not_found_enemies.push(spawner);
        }
        for (const auto &rc_node: scene_root["covered_pickups"]) {
            AnimationRenderer *renderer;
            CreateAndAddPickUpHolder(rc_node["content"].as<PickUpType>(),
                    rc_node["pos"].as<Vector2D>() * PIXELS_ZOOM,
                    new CoveredPickUpHolderBehaviour(),
                    {-12, -15,10, 15},
                    &renderer);
            renderer->AddAnimation({
                    1, 76, 0.15, 1,
                    34, 34, 17, 17,
                    "Closed", AnimationRenderer::STOP_AND_LAST
            });
            renderer->AddAnimation({
                    35, 110, 0.15, 3,
                    34, 34, 17, 17,
                    "Opening", AnimationRenderer::STOP_AND_LAST
            });
            renderer->AddAnimation({
                    137, 76, 0.15, 3,
                    34, 34, 17, 17,
                    "Open", AnimationRenderer::BOUNCE
            });
            renderer->AddAnimation({
                    92, 611, 0.15, 3,
                    30, 30, 15, 15,
                    "Dying", AnimationRenderer::BOUNCE_AND_STOP});
        }
        for (const auto &rc_node: scene_root["flying_pickups"]) {
            AnimationRenderer *renderer;
            CreateAndAddPickUpHolder(rc_node["content"].as<PickUpType>(),
                    rc_node["pos"].as<Vector2D>() * PIXELS_ZOOM,
                    new FlyingPickupHolderBehaviour(),
                    {-9, -6,9, 6},
                    &renderer);
            renderer->AddAnimation({
                    243, 120, 0.15, 1,
                    24, 14, 12, 7,
                    "Flying", AnimationRenderer::STOP_AND_LAST
            });
            renderer->AddAnimation({
                    92, 611, 0.15, 3,
                    30, 30, 15, 15,
                    "Dying", AnimationRenderer::BOUNCE_AND_STOP});
        }
    } catch (YAML::BadFile &exception) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't load level file: %s/level.yaml", &folder[0]);
    }
}

void Level::Destroy() {
    GameObject::Destroy();
    for (const auto &layer: game_objects) {
        for (auto game_object : *layer)
            game_object->Destroy();
    }
    while (!not_found_enemies.empty()) {
        not_found_enemies.top()->Destroy();
        not_found_enemies.pop();
    }
    while (!game_objects_to_add.empty()) {
        game_objects_to_add.front().first->Destroy();
        game_objects_to_add.pop();
    }
    default_bullets->Destroy();
    delete default_bullets;
    default_bullets = nullptr;
    enemy_bullets->Destroy();
    delete enemy_bullets;
    enemy_bullets = nullptr;
    delete player;
    player = nullptr;
    background.reset();
}

void Level::CreateBulletPools() {
    // Create bullet pools for the player
    default_bullets = CreatePlayerBulletPool<BulletStraightMovement>(MAX_DEFAULT_BULLETS, {
            82, 10, 0.2, 1,
            3, 3, 1, 1,
            "Bullet", AnimationRenderer::STOP_AND_LAST
    }, {-1, -1, 2, 2});
    machine_gun_bullets = CreatePlayerBulletPool<BulletStraightMovement>(MAX_MACHINE_GUN_BULLETS, {
            89, 9, 0.2, 1,
            5, 5, 2, 2,
            "Bullet", AnimationRenderer::STOP_AND_LAST
    }, {-2, -2, 2, 2});
    spread_bullets = CreatePlayerBulletPool<BulletStraightMovement>(MAX_SPREAD_BULLETS, {
            88, 8, 0.2, 3,
            8, 8, 4, 4,
            "Bullet", AnimationRenderer::STOP_AND_LAST
    }, {-2, -2, 2, 2});
    fire_bullets = CreatePlayerBulletPool<BulletCirclesMovement>(MAX_FIRE_BULLETS, {
            112, 8, 0.2, 1,
            8, 8, 4, 4,
            "Bullet", AnimationRenderer::STOP_AND_LAST
    }, {-2, -2, 2, 2});

    // Create bullet pool for the npcs
    enemy_bullets = new ObjectPool<Bullet>();
    enemy_bullets->Create(MAX_NPC_BULLETS);
    for (auto *bullet: enemy_bullets->pool) {
        bullet->Create();
        auto *renderer = new AnimationRenderer();
        renderer->Create(this, bullet, enemies_spritesheet);
        renderer->AddAnimation({
                199, 72, 0.2, 1,
                3, 3, 1, 1,
                "Bullet", AnimationRenderer::STOP_AND_LAST
        });
        auto *behaviour = new BulletStraightMovement();
        behaviour->Create(this, bullet);
        auto *box_collider = new BoxCollider();
        box_collider->Create(this, bullet,
                -1 * PIXELS_ZOOM, -1 * PIXELS_ZOOM,
                3 * PIXELS_ZOOM, 3 * PIXELS_ZOOM, PLAYER_COLLISION_LAYER, -1);
        bullet->AddComponent(behaviour);
        bullet->AddComponent(renderer);
        bullet->AddComponent(box_collider);
        bullet->AddReceiver(this);

        bullet->onRemoval = DO_NOT_DESTROY; // Do not destroy until the end of the game
    }
}

void Level::Init() {
    GameObject::Init();
    camera_x = 0;

    next_enemy_x = not_found_enemies.top()->position.x;
    while (next_enemy_x < WINDOW_WIDTH) {
        if (!not_found_enemies.empty()) {
            game_objects[RENDERING_LAYER_ENEMIES]->insert(not_found_enemies.top());
            not_found_enemies.pop();
            next_enemy_x = not_found_enemies.top()->position.x;
        }
    }

    for (const auto &layer: game_objects) {
        for (auto game_object : *layer)
            game_object->Init();
    }
}

void Level::CreatePlayer() {
    player = new Player();
    player->Create(this);
    playerControl = player->GetComponent<PlayerControl *>();
    player->AddReceiver(this);
    game_objects[RENDERING_LAYER_PLAYER]->insert(player);
}

template<typename T>
ObjectPool<Bullet> *Level::CreatePlayerBulletPool(int num_bullets, const AnimationRenderer::Animation &animation,
                                                  const Box &box) {
    auto *pool = new ObjectPool<Bullet>();
    pool->Create(num_bullets);
    for (auto *bullet: pool->pool) {
        bullet->Create();
        auto *renderer = new AnimationRenderer();
        renderer->Create(this, bullet, spritesheet);
        renderer->AddAnimation(animation);
        renderer->AddAnimation({
                104, 0, 0.2, 1,
                7, 7, 3, 3,
                "Kill", AnimationRenderer::STOP_AND_FIRST
        });
        renderer->Play();
        auto* behaviour = new T();
        behaviour->Create(this, bullet);
        auto *box_collider = new BoxCollider();
        box_collider->Create(this, bullet, box * PIXELS_ZOOM, NPCS_COLLISION_LAYER, -1);
        bullet->AddComponent(behaviour);
        bullet->AddComponent(renderer);
        bullet->AddComponent(box_collider);
        bullet->AddReceiver(this);

        bullet->onRemoval = DO_NOT_DESTROY; // Do not destroy until the end of the game
    }
    return pool;
}

void Level::CreateAndAddPickUpHolder(const PickUpType &type, const Vector2D &position,
                                     PickUpHolderBehaviour *behaviour, const Box &box, AnimationRenderer **renderer) {
    auto *pickup = new PickUp();
    pickup->Create(this, pickups_spritesheet, &grid, level_floor, type);
    auto *pick_up_holder = new GameObject();
    pick_up_holder->position = position;
    behaviour->Create(this, pick_up_holder, pickup);
    *renderer = new AnimationRenderer();
    (*renderer)->Create(this, pick_up_holder, enemies_spritesheet);
    auto *collider = new BoxCollider();
    collider->Create(this, pick_up_holder, box * PIXELS_ZOOM, -1, NPCS_COLLISION_LAYER);
    collider->SetListener(behaviour);

    pick_up_holder->AddComponent(behaviour);
    pick_up_holder->AddComponent(*renderer);
    pick_up_holder->AddComponent(collider);
    pick_up_holder->AddReceiver(this);
    not_found_enemies.push(pick_up_holder);
}
