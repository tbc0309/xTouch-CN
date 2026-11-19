#pragma once

// Event-reporting controllers can have no new packet while a contact is held.
// Only an explicit contact report changes the pointer state.
struct TouchContact {
    bool pressed = false;
    int lastX = 0, lastY = 0;

    bool read(int &x, int &y) const {
        x = lastX;
        y = lastY;
        return pressed;
    }
    bool report(bool down, int x, int y, int &outX, int &outY) {
        pressed = down;
        if (down) {
            lastX = x;
            lastY = y;
        }
        return read(outX, outY);
    }
};
