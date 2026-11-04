#pragma once

struct DragBreakdown {
    double friction = 0.0;
    double pressure = 0.0;
    double base = 0.0;
    double fin = 0.0;
    double parasitic = 0.0;
    double wave = 0.0;

    double total() const noexcept {
        return friction + pressure + base + parasitic + wave;
    }

    DragBreakdown& operator+=(const DragBreakdown& other) noexcept {
        friction += other.friction;
        pressure += other.pressure;
        base += other.base;
        fin += other.fin;
        parasitic += other.parasitic;
        wave += other.wave;
        return *this;
    }
};
