#pragma once

class Entity {
public:
    int x, y;
    int w, h;

    Entity(int x, int y, int w, int h)
        : x(x), y(y), w(w), h(h) {}

    virtual void update() {}
};
