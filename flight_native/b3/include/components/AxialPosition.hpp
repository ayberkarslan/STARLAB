#pragma once

// OpenRocket-style axial placement, represented independently from UI labels.
// All distances are metres in the parent component's local +Z direction.
enum class AxialPositionMode {
    After,
    FromTop,
    FromBottom,
    Absolute,
};

struct AxialPosition {
    AxialPositionMode mode = AxialPositionMode::After;
    double offset = 0.0;
};
