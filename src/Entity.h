#pragma once

class Entity{
public:
    int x,y; //position on 2D Map - x & y coordinates
    int width, height; // size on map

    Entity(int x, int y, int width , int height)
        : x(x) , y(y), width(width), height(height) {}

    virtual void update();

};