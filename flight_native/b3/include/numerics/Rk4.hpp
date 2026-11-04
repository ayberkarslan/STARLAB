#pragma once

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace numerics {

template <typename State>
struct Rk4Sample {
    double time = 0.0;
    State state;
};

template <typename State, typename Derivative>
struct Rk4Ops {
    State (*addScaled)(const State& state, const Derivative& derivative, double scale) = nullptr;
    State (*addWeighted)(
        const State& state,
        const Derivative& k1,
        const Derivative& k2,
        const Derivative& k3,
        const Derivative& k4,
        double dt
    ) = nullptr;
};

template <typename State, typename Derivative, typename Ode>
State rk4Step(
    Ode&& ode,
    const Rk4Ops<State, Derivative>& ops,
    double t,
    const State& state,
    double dt
) {
    if (dt <= 0.0) {
        throw std::invalid_argument("RK4 timestep must be positive.");
    }
    if (!ops.addScaled || !ops.addWeighted) {
        throw std::invalid_argument("RK4 operations must be provided.");
    }

    const Derivative k1 = ode(t, state);
    const Derivative k2 = ode(t + 0.5 * dt, ops.addScaled(state, k1, 0.5 * dt));
    const Derivative k3 = ode(t + 0.5 * dt, ops.addScaled(state, k2, 0.5 * dt));
    const Derivative k4 = ode(t + dt, ops.addScaled(state, k3, dt));

    return ops.addWeighted(state, k1, k2, k3, k4, dt);
}

template <typename State, typename Derivative, typename Ode>
std::vector<Rk4Sample<State>> integrateRk4(
    Ode&& ode,
    const Rk4Ops<State, Derivative>& ops,
    const State& initialState,
    double t0,
    double tf,
    double dt
) {
    if (dt <= 0.0) {
        throw std::invalid_argument("RK4 timestep must be positive.");
    }
    if (tf < t0) {
        throw std::invalid_argument("RK4 final time must be greater than or equal to initial time.");
    }

    constexpr double epsilon = 1e-9;
    std::vector<Rk4Sample<State>> samples;
    samples.push_back(Rk4Sample<State>{t0, initialState});

    double t = t0;
    State state = initialState;
    while (t < tf - epsilon) {
        const double h = std::min(dt, tf - t);
        state = rk4Step<State, Derivative>(ode, ops, t, state, h);
        t += h;
        samples.push_back(Rk4Sample<State>{t, state});
    }

    return samples;
}

} // namespace numerics
