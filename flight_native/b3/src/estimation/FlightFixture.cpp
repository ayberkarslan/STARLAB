#include "estimation/ValidationRunner.hpp"
#include "sensors/TruthAdapter.hpp"
#include "sixdof/SixDof.hpp"
#include "numerics/Rk4.hpp"
#include "env/Atmosphere.hpp"

namespace estimation::validation {
FlightFixture verticalSixDofFixture() {
    FlightFixture fixture;
    fixture.source="tests/cpp/sixdof_test.cpp pure-translation fixture: mass=10 kg, net force=20 N; rotated to +Z and extended with pad/coast using unchanged eom6 and RK4";
    sixdof::State6 state;
    sixdof::SimParams6 params;
    params.massCallback=[](double,const auto&) { sixdof::MassProps mass; mass.mass=10; return mass; };
    const numerics::Rk4Ops<sixdof::State6,sixdof::StateDerivative6> ops{sixdof::addScaled,sixdof::addWeighted};
    constexpr double dt=.01;
    for(int k=0;k<=800;++k) {
        const double time=k*dt;
        const double acceleration=time<1?0:time<6?2:-sensors::g0;
        // Net force includes support on the pad and gravity during coast.
        params.forceMomentCallback=[acceleration](double,const auto&) {sixdof::ForceMoment f; f.Fz=10*acceleration; return f;};
        auto phase=time<1?sensors::FlightPhase::PRELAUNCH:
            time==1?sensors::FlightPhase::IGNITION:
            time<6?sensors::FlightPhase::POWERED_ASCENT:
            time==6?sensors::FlightPhase::BURNOUT:
            state.w>0?sensors::FlightPhase::COAST:sensors::FlightPhase::DESCENT;
        if(k>600 && fixture.samples.back().velocity.z()>0 && state.w<=0) phase=sensors::FlightPhase::APOGEE_SEPARATION;
        const auto atmosphere=env::Atmosphere::getProperties(state.Z);
        fixture.samples.push_back(sensors::fromSixDof(time,state,sixdof::eom6(time,state,params),
            sensors::Vec3(0,0,-sensors::g0),atmosphere.pressure,atmosphere.temperature,phase));
        if(k<800) state=numerics::rk4Step<sixdof::State6,sixdof::StateDerivative6>(
            [&](double t,const auto& s){return sixdof::eom6(t,s,params);},ops,time,state,dt);
    }
    return fixture;
}
}
