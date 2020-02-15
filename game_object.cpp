#include "game_object.h"
#include "component.h"
#include "avancezlib.h"

int GameObject::s_nextId = 0;

void GameObject::Create() {
    enabled = false;
}

void GameObject::AddComponent(Component *component) {
    components.push_back(component);
}


void GameObject::Init() {
    for (auto it = components.begin(); it != components.end(); it++)
        (*it)->Init();

    enabled = true;
}

void GameObject::Update(float dt) {
    for (auto it = components.begin(); it != components.end(); it++) {
        if (!enabled)
            return;
        (*it)->Update(dt);
    }
}

void GameObject::Destroy() {
    SDL_Log("GameObject(%d)::Destroy", id);
    for (auto it = components.begin(); it != components.end(); it++)
        (*it)->Destroy();
}

GameObject::~GameObject() {
    SDL_Log("GameObject(%d)::~GameObject", id);
}

void GameObject::AddReceiver(GameObject *go) {
    receivers.push_back(go);
}

void GameObject::Send(Message m) {
    for (auto i = 0; i < receivers.size(); i++) {
        if (!receivers[i]->enabled)
            continue;

        receivers[i]->Receive(m);
    }
}

void GameObject::OnEnabled() {
    for (auto it = components.begin(); it != components.end(); it++)
        (*it)->OnGameObjectEnabled();
}

void GameObject::OnDisabled() {
    for (auto it = components.begin(); it != components.end(); it++)
        (*it)->OnGameObjectDisabled();
}
