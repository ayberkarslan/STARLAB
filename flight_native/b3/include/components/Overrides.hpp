#pragma once

struct Overrides {
    bool massEnabled = false;
    double mass = 0.0;

    bool cgEnabled = false;
    double cgLocalZ = 0.0;

    bool cpEnabled = false;
    double cpAbsolute = 0.0;

    bool cnaEnabled = false;
    double cna = 0.0;

    bool cdEnabled = false;
    double cd = 0.0;
};
