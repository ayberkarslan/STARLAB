// roketsim_cli.cpp — Standalone 3-DoF rocket flight simulator
// Uses JSON input, writes CSV output. No Eigen in main loop callbacks.
// CG/CP/forces computed with plain doubles only.

#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <string>
#include <cstdio>
#include <cstdlib>  // for _exit()
#include <limits>
#include <unordered_map>
#include <unordered_set>

#include "nlohmann/json.hpp"
#include "env/Atmosphere.hpp"
#include "env/Geodesy.hpp"
#include "aero/RasaeroTable.hpp"
#ifdef ROKETSIM_ENABLE_DATCOM_DERIVED_EXTENSIONS
#include "aero/extensions/DatcomExtensionCatalog.hpp"
#endif
#include "env/WindModel.hpp"
#include "design_analysis.hpp"
#include "datcom_equivalent_analysis.hpp"
#include "runtime_mass_properties.hpp"
#include "staging.hpp"
#include "motor_analysis.hpp"
#include "motor_catalog.hpp"
#include "malzeme/VeriTabanimalzeme.h"

using json = nlohmann::json;

static const double PI        = 3.14159265358979323846;
static const double DEG2RAD   = PI / 180.0;
static const double earthR    = 6356766.0;
static const double g0        = 9.80665;

static const RoketSiM::AeroExtensions::AeroExtensionPipeline&
productionAeroExtensionPipeline() {
#ifdef ROKETSIM_ENABLE_DATCOM_DERIVED_EXTENSIONS
    static const auto pipeline =
        RoketSiM::AeroExtensions::makeDatcomExtensionPipeline();
#else
    static const RoketSiM::AeroExtensions::AeroExtensionPipeline pipeline;
#endif
    return pipeline;
}

// ── Linear interpolation ────────────────────────────────────────────────
static double lerp(double x,
                   const std::vector<double>& xp,
                   const std::vector<double>& yp)
{
    if (xp.empty()) return 0.0;
    if (x <= xp.front()) return yp.front();
    if (x >= xp.back())  return yp.back();
    for (size_t i = 0; i < xp.size() - 1; ++i) {
        if (x >= xp[i] && x <= xp[i+1]) {
            double t = (x - xp[i]) / (xp[i+1] - xp[i]);
            return yp[i] + t * (yp[i+1] - yp[i]);
        }
    }
    return yp.back();
}

// ── 3-DoF state: [alt_m, vz_ms, theta_rad, dtheta_rad_s] ───────────────
// We model vertical flight with pitch angle from vertical.
// alt   = altitude AGL (m)
// vz    = vertical velocity (m/s, positive up)
// vx    = horizontal (downrange) velocity (m/s)
// pitch = angle from vertical (rad) — 0 = straight up
// dpitch= pitch rate (rad/s)

struct State3 {
    double alt   = 0.0;
    double vz    = 0.0;
    double vx    = 0.0;
    double pitch = 0.0;   // angle from vertical (rad)
    double dpitch= 0.0;   // pitch rate
    double downrange = 0.0;
    bool parachuteDeployed = false;
    bool railCleared = false;
};

struct RecoveryConfig {
    bool enabled = true;
    std::string mode = "Apogee";
    bool altitude_threshold = false;
    double main_deploy_altitude_m = 300.0;
    double drogue_deploy_delay_s = 0.0;
    double main_deploy_delay_s = 0.0;
    double parachute_diameter_m = 1.2;
    double parachute_cd = 1.5;
    double parachute_area_m2 = 1.13;
    double target_descent_rate_ms = 6.0;
    double mass_margin_pct = 10.0;
};

enum class RecoveryState { Stowed, Triggered, Deploying, Inflating, Inflated };

struct RecoveryDevice {
    std::string id;
    std::string name;
    std::string device_type = "generic";
    std::string trigger = "APOGEE";
    double trigger_altitude_agl_m = 0.0;
    double trigger_time_s = 0.0;
    double deployment_delay_s = 0.0;
    double inflation_time_s = 1.0;
    double cd_s_m2 = 0.0;
    bool enabled = true;
    RecoveryState state = RecoveryState::Stowed;
    double triggered_at_s = -1.0;
    double inflation_start_time_s = -1.0;
};

struct SimConfig {
    // rocket
    double mass_dry;
    double prop_mass_init;
    double dm_dt;
    double t_burn;
    double Ixx;           // transverse moment of inertia (kg·m²)
    double cg_z;          // CG distance from nose (m)
    double cp_z;          // CP distance from nose (m)
    double ref_area;      // reference area (m²)
    double ref_dia;       // reference diameter (m)
    double cna;           // normal force slope coefficient
    std::vector<double> aero_mach, cna_curve, cp_curve;
    barrowman_drag::Model drag;
    rasaero_table::Table rasaeroAeroTable;
    bool useRasaeroAeroTable = false;
    double legacy_cd = 0.5;
    // The canonical baseline: everything EXCEPT the active motors' dry
    // structure and remaining propellant (the vehicle phase's static
    // airframe), accumulated about the vehicle reference origin -- see
    // runtime_mass_properties.hpp. evaluateMassProperties() adds each active
    // motor's own dry+propellant contribution on top of this every call;
    // base_mass/base_first_moment/base_i_origin below remain as thin,
    // derived read-outs of this same baseline (never independently
    // computed) so any existing direct reader keeps working unchanged.
    runtime_mass::OriginAccumulator base_origin_static;
    double base_mass = 0.0;
    double base_first_moment = 0.0;
    double base_i_origin = 0.0;
    double motor_dry_mass = 0.0;
    double motor_initial_mass = 0.0;
    double motor_cg_z = 0.0;
    double motor_i_per_mass = 0.0;
    double motor_total_impulse = 0.0;
    int motor_count = 1;
    bool approximate_stage_separation_enabled = false;
    double approximate_stage_separation_time_s = 0.0;
    double approximate_stage_dropped_mass_kg = 0.0;
    double launch_angle_rad = 0.0;
    double launch_altitude_m = 0.0;
    double launch_latitude_deg = 0.0;
    double launch_longitude_deg = 0.0;
    double surface_temperature_k = 288.15;
    double surface_pressure_pa = 101325.0;
    double rail_length_m = 5.0;
    // Axial distance from the vehicle's aft (tail) end to its lowest/aft-most
    // rail button, at launch (fully assembled). The rail guides the vehicle
    // only until that button clears the top of the rail, so the reference
    // point tracked in flight (the tail, alt=0 at t=0) only needs to travel
    // (rail_length_m - rail_button_offset_from_tail_m) before rail-exit --
    // not the full rail_length_m. Zero (no rail button found) preserves the
    // old full-rail-length behavior.
    double rail_button_offset_from_tail_m = 0.0;
    std::vector<RecoveryDevice> recovery_devices;
    env::WindConfig wind;
    // motor
    std::vector<double> mot_t, mot_f;
    // Canonical per-motor flight records.  The legacy mot_t/mot_f fields
    // remain a compatibility path for saved UI simulations.  Definition
    // lives in staging.hpp so the staging runtime contracts and the flight
    // solver share one MotorInstance shape.
    std::vector<staging::MotorInstance> motor_instances;
    // Axial stage plan and its precomputed per-phase attached-vehicle dry
    // snapshots.
    staging::StagePlan stage_plan;
    std::vector<staging::VehiclePhase> vehicle_phases;
    std::vector<staging::StagingWarning> staging_warnings;
    staging::EventSchedule event_schedule;
    // Mutated in place by applyVehiclePhase() as the flight crosses
    // STAGE_SEPARATION events -- always reflects the CURRENTLY active
    // phase, so getMass()/getCg()/getIxx()/deriv() need no phase-index
    // branching of their own.
    //
    // motor_filter_enabled is the explicit on/off switch; active_motor_ids
    // is only meaningful when it is true.  An empty active_motor_ids with
    // filtering ENABLED means a real vehicle phase with zero attached
    // motors (e.g. the final phase after the last motor's stage has
    // separated) -- distinct from filtering being DISABLED, which means
    // "no staging active, include every motor instance" (Checkpoint 1's
    // non-staged motor_instances payload and every legacy payload keep
    // today's behavior exactly).  Collapsing these into one empty-set
    // sentinel would make a legitimate motorless phase behave as
    // "unfiltered" and resurrect every dropped motor's thrust/mass.
    bool motor_filter_enabled = false;
    std::unordered_set<std::string> active_motor_ids;
    // launch
    double launch_angle_deg;  // angle from vertical
    std::string requested_dof = "3-DoF";
    std::string earth_model = "WGS84 local tangent plane";
};

// Re-derives the legacy scalar baseline fields (base_mass/base_first_moment/
// base_i_origin -- the vehicle's z-axis moment and Ixx-about-origin) from
// base_origin_static, the one canonical general baseline. Called every time
// base_origin_static is (re)computed, so those scalars are always a thin
// view of it rather than an independently-set value that could drift.
static void syncLegacyBaselineScalars(SimConfig& c) {
    c.base_mass = c.base_origin_static.mass_kg;
    c.base_first_moment = c.base_origin_static.mz;
    c.base_i_origin = c.base_origin_static.ixx;
}

// Recovery V1 keeps the device configuration (the input contract) separate
// from its run-time state.  CdS is the sole aerodynamic quantity used by the
// integrator; Cd and reference area are retained only as input provenance.

static const char* recoveryStateName(RecoveryState state) {
    switch (state) {
    case RecoveryState::Stowed: return "STOWED";
    case RecoveryState::Triggered: return "TRIGGERED";
    case RecoveryState::Deploying: return "DEPLOYING";
    case RecoveryState::Inflating: return "INFLATING";
    case RecoveryState::Inflated: return "INFLATED";
    }
    return "STOWED";
}

// V1 numerical filling approximation.  This is deliberately isolated from
// the force equation so a Knacke/USAF filling strategy can replace it later.
static double recoveryInflationFraction(const RecoveryDevice& device, double timeS) {
    if (device.state == RecoveryState::Inflated) return 1.0;
    if (device.state != RecoveryState::Inflating || device.inflation_start_time_s < 0.0)
        return 0.0;
    if (device.inflation_time_s <= 1.0e-9) return 1.0;
    const double tau = std::clamp(
        (timeS - device.inflation_start_time_s) / device.inflation_time_s, 0.0, 1.0);
    return tau * tau * (3.0 - 2.0 * tau); // smoothstep
}

static double activeRecoveryCdS(const std::vector<RecoveryDevice>& devices, double timeS) {
    double total = 0.0;
    for (const auto& device : devices) {
        total += device.cd_s_m2 * recoveryInflationFraction(device, timeS);
    }
    return total;
}

// Captured from the same derivative evaluation used by the RK4 solver.  These
// are solver values, not UI-side estimates or reconstructed placeholders.
struct TelemetryMetrics {
    double thrustN = 0.0, axialCd = 0.0, cnaPerRad = 0.0, cpM = 0.0;
    double alphaRad = 0.0, reynolds = 0.0, rocketCgM = 0.0, ixxKgm2 = 0.0;
    // Full runtime mass-property tensor diagnostics (see
    // runtime_mass_properties.hpp / evaluateMassProperties()). cgXM/cgYM
    // and the ixy/ixz/iyz products of inertia are 0 for every currently
    // supported vehicle (no component/motor is actually off-axis in this
    // codebase today), but are computed generally, not hardcoded to 0.
    double cgXM = 0.0, cgYM = 0.0, iyyKgm2 = 0.0, izzKgm2 = 0.0;
    double ixyKgm2 = 0.0, ixzKgm2 = 0.0, iyzKgm2 = 0.0;
    double aeroDownrangeN = 0.0, aeroVerticalN = 0.0, normalForceN = 0.0;
    double aeroPitchMomentNm = 0.0, dampingPitchMomentNm = 0.0;
    double recoveryDragN = 0.0, recoveryCdSM2 = 0.0, recoveryInflationFraction = 0.0;
    double recoveryDragAccelerationMs2 = 0.0, netDownrangeN = 0.0, netVerticalN = 0.0;
    int recoveryState = 0, activeRecoveryDeviceIndex = -1;
    double gravityForceVerticalN = 0.0;
    double accelerationDownrangeMs2 = 0.0, accelerationVerticalMs2 = 0.0;
    double pitchAccelerationRadS2 = 0.0, propellantMassKg = 0.0;
    bool onLaunchRail = false;
};

static json recoveryResult(const RecoveryConfig& recovery,
                           double rocketMassKg,
                           double apogeeM,
                           double apogeeTimeS) {
    if (!recovery.enabled) {
        return {{"enabled", false}, {"mode", recovery.mode}};
    }
    const double effectiveMass = rocketMassKg
        * (1.0 + recovery.mass_margin_pct / 100.0);
    const double denominator = std::max(
        1.225 * recovery.parachute_cd * recovery.parachute_area_m2, 1e-9);
    const double descentRate =
        std::sqrt(2.0 * effectiveMass * g0 / denominator);
    const bool altitudeThreshold = recovery.altitude_threshold;
    const double deployAltitude = altitudeThreshold
        ? std::min(apogeeM, recovery.main_deploy_altitude_m) : apogeeM;
    const double deployTime = apogeeTimeS + (altitudeThreshold
        ? recovery.main_deploy_delay_s : recovery.drogue_deploy_delay_s);
    return {
        {"enabled", true}, {"mode", recovery.mode},
        {"deploy_time_s", deployTime},
        {"deploy_altitude_m", deployAltitude},
        {"main_deploy_altitude_m", recovery.main_deploy_altitude_m},
        {"parachute_cd", recovery.parachute_cd},
        {"parachute_area_m2", recovery.parachute_area_m2},
        {"parachute_diameter_m", recovery.parachute_diameter_m},
        {"estimated_descent_rate_ms", descentRate},
        {"estimated_descent_time_s",
            std::max(deployAltitude, 0.0) / std::max(descentRate, 0.1)},
        {"target_descent_rate_ms", recovery.target_descent_rate_ms},
        {"effective_recovery_mass_kg", effectiveMass},
    };
}

// The vehicle clears the rail once its lowest rail button passes the top of
// the rail, not once the tracked reference point has translated the full
// physical rail length -- see rail_button_offset_from_tail_m.
static double effectiveRailLengthM(const SimConfig& c) {
    return std::max(0.0, c.rail_length_m - c.rail_button_offset_from_tail_m);
}

// A dropped stage's motors must stop contributing entirely -- not just
// stop thrusting. c.motor_filter_enabled == false means no staging is
// active (Checkpoint 1's non-staged motor_instances payload, and every
// legacy payload), so every motor instance counts, exactly as before
// Checkpoint 2. When filtering IS enabled, an empty active_motor_ids is a
// real "zero motors attached" phase, not "unfiltered" -- see SimConfig.
static bool motorIsActive(const SimConfig& c, const staging::MotorInstance& motor) {
    if (!c.motor_filter_enabled) return true;
    return c.active_motor_ids.count(motor.id) != 0;
}

// "Attached" (physically present, see motorIsActive/active_motor_instance_ids)
// and "firing" (currently producing nonzero thrust) are distinct -- a coast
// phase between burnout and separation has an attached motor that isn't
// firing. Mirrors the same curve interpolation deriv() and propellantMassAt()
// use, so this never disagrees with the thrust actually integrated.
static int firingMotorCount(const SimConfig& c, double t) {
    if (c.motor_instances.empty()) {
        if (c.mot_t.size() < 2) return 0;
        return lerp(t, c.mot_t, c.mot_f) > 1e-6 ? 1 : 0;
    }
    int count = 0;
    for (const auto& motor : c.motor_instances) {
        if (!motorIsActive(c, motor)) continue;
        const double local = t - motor.resolved_ignition_time_s;
        if (local < 0.0) continue;
        if (lerp(local, motor.time_s, motor.thrust_n) > 1e-6) ++count;
    }
    return count;
}

// Remaining propellant mass for ONE motor instance at its own local time
// (t - resolved_ignition_time_s). Factored out of propellantMassAt() so
// getIxx() can attribute each motor's remaining propellant to that motor's
// OWN cg_z_m/transverse_inertia_per_mass individually (see getIxx()) instead
// of lumping every motor's propellant onto one mass-weighted-average
// position -- wrong whenever two or more motors have simultaneous nonzero
// propellant at different axial positions (every pre-separation instant of
// a multi-stage vehicle whose upper-stage motor is already attached).
static double remainingPropellantKg(const staging::MotorInstance& motor, double local) {
    if (motor.time_s.size() < 2) return 0.0;
    const double initialPropellant = std::max(0.0, motor.wet_mass_kg - motor.dry_mass_kg);
    if (local <= 0.0) return initialPropellant;
    double impulse = 0.0, totalImpulse = 0.0;
    for (std::size_t i = 1; i < motor.time_s.size(); ++i) {
        const double t0 = motor.time_s[i - 1], t1 = motor.time_s[i];
        const double segment = 0.5 * (motor.thrust_n[i - 1] + motor.thrust_n[i])
            * std::max(t1 - t0, 0.0);
        totalImpulse += segment;
        if (local > t0) {
            const double end = std::min(local, t1);
            const double ratio = (end - t0) / std::max(t1 - t0, 1e-12);
            const double fEnd = motor.thrust_n[i - 1]
                + ratio * (motor.thrust_n[i] - motor.thrust_n[i - 1]);
            impulse += 0.5 * (motor.thrust_n[i - 1] + fEnd) * std::max(end - t0, 0.0);
        }
    }
    return initialPropellant * std::clamp(1.0 - impulse / std::max(totalImpulse, 1e-12), 0.0, 1.0);
}

static double propellantMassAt(const SimConfig& c, double t) {
    if (!c.motor_instances.empty()) {
        double total = 0.0;
        for (const auto& motor : c.motor_instances) {
            if (!motorIsActive(c, motor) || motor.time_s.size() < 2) continue;
            total += remainingPropellantKg(motor, t - motor.resolved_ignition_time_s);
        }
        return total;
    }
    if (!(c.prop_mass_init > 0.0) || c.mot_t.size() < 2
        || !(c.motor_total_impulse > 0.0)) return 0.0;
    double consumedImpulse = 0.0;
    for (std::size_t i = 1; i < c.mot_t.size(); ++i) {
        const double t0 = c.mot_t[i - 1];
        const double t1 = c.mot_t[i];
        if (t <= t0) break;
        const double end = std::min(t, t1);
        const double fraction = (end - t0) / std::max(t1 - t0, 1e-12);
        const double fEnd = c.mot_f[i - 1]
            + fraction * (c.mot_f[i] - c.mot_f[i - 1]);
        consumedImpulse += 0.5 * (c.mot_f[i - 1] + fEnd) * (end - t0);
        if (t <= t1) break;
    }
    return c.prop_mass_init * std::clamp(
        1.0 - consumedImpulse / c.motor_total_impulse, 0.0, 1.0);
}

// The ONE canonical runtime mass-property calculation (see
// docs/flight/runtime_mass_properties_contract.md). Starts from the current
// vehicle phase's static baseline (base_origin_static -- airframe only, no
// motors) and adds each ACTIVE motor's own dry structure and own remaining
// propellant as independent contributions at that motor's own position,
// never lumped onto a shared average position. General: makes no
// axisymmetry assumption -- Ixx/Iyy/products of inertia fall out of the
// underlying data rather than being asserted equal or zero.
//
// Every currently-supported motor record (staging::MotorInstance and the
// legacy single-motor c.motor_cg_z/c.motor_i_per_mass fields) carries only
// an axial (z) position and a single transverse-inertia-per-mass value
// (applied identically to both the x and y axes) -- there is no x/y offset
// and no roll-axis (izz) contribution for a motor in the current data
// model. That is a real, documented limitation of today's motor
// representation, not something this function papers over: a motor's own
// ixz_kgm2/iyz_kgm2/ixy_kgm2 contribution is correctly left at 0 because
// the source data has nothing else to report, not because this function
// assumes it must be 0.
static runtime_mass::RuntimeMassProperties evaluateMassProperties(
        const SimConfig& c, double t) {
    runtime_mass::OriginAccumulator acc = c.base_origin_static;
    if (!c.motor_instances.empty()) {
        for (const auto& motor : c.motor_instances) {
            if (!motorIsActive(c, motor)) continue;
            const double transverse = motor.dry_mass_kg * motor.transverse_inertia_per_mass;
            acc.add({motor.dry_mass_kg, 0.0, 0.0, motor.cg_z_m,
                     transverse, transverse, 0.0, 0.0, 0.0, 0.0});
            const double remaining = remainingPropellantKg(
                motor, t - motor.resolved_ignition_time_s);
            const double propTransverse = remaining * motor.transverse_inertia_per_mass;
            acc.add({remaining, 0.0, 0.0, motor.cg_z_m,
                     propTransverse, propTransverse, 0.0, 0.0, 0.0, 0.0});
        }
    } else {
        const double dryTransverse = c.motor_dry_mass * c.motor_i_per_mass;
        acc.add({c.motor_dry_mass, 0.0, 0.0, c.motor_cg_z,
                 dryTransverse, dryTransverse, 0.0, 0.0, 0.0, 0.0});
        const double remaining = propellantMassAt(c, t);
        const double propTransverse = remaining * c.motor_i_per_mass;
        acc.add({remaining, 0.0, 0.0, c.motor_cg_z,
                 propTransverse, propTransverse, 0.0, 0.0, 0.0, 0.0});
    }
    return acc.finalize();
}

static double getMass(const SimConfig& c, double t) {
    // A genuine StagePlan already drops mass discontinuously at its own
    // resolved separation events (applyVehiclePhase's base_mass swap plus
    // motorIsActive filtering, above). Applying the deprecated scalar
    // approximate-staging drop on top of that would double-drop mass, so
    // real staging (motor_filter_enabled, set only when a real VehiclePhase
    // was applied -- see applyVehiclePhase) is always authoritative and the
    // legacy mechanism is ignored while it is active (Checkpoint 4 Part F).
    const double separationDrop = !c.motor_filter_enabled
        && c.approximate_stage_separation_enabled
        && t >= c.approximate_stage_separation_time_s
        ? c.approximate_stage_dropped_mass_kg : 0.0;
    return std::max(1e-6, evaluateMassProperties(c, t).mass_kg - separationDrop);
}

static double getCg(const SimConfig& c, double t) {
    return evaluateMassProperties(c, t).cg_z_m;
}

static double getIxx(const SimConfig& c, double t, double /*cg*/) {
    // The `cg` parameter is retained for call-site compatibility (every
    // existing caller computes cg via getCg() first and passes it in) but
    // is no longer used: evaluateMassProperties() derives its own cg from
    // the SAME underlying accumulator in the same call, which is exactly
    // equivalent (and never able to disagree, by construction) with a
    // separately-computed getCg(c, t).
    return evaluateMassProperties(c, t).ixx_kgm2;
}

static double getIyy(const SimConfig& c, double t) {
    return evaluateMassProperties(c, t).iyy_kgm2;
}

static double getIzz(const SimConfig& c, double t) {
    return evaluateMassProperties(c, t).izz_kgm2;
}

// Derivatives for 3-DoF
static State3 deriv(double t, const State3& s, const SimConfig& c,
                    const env::WindModel& windModel,
                    const env::WindStep& windStep,
                    TelemetryMetrics* telemetry = nullptr) {
    State3 d;

    double mass = getMass(c, t);
    double cg_z = getCg(c, t);
    double ixx = getIxx(c, t, cg_z);
    // mass/cg_z/ixx above go through getMass()/getCg()/getIxx() so the
    // legacy approximate-staging separationDrop (getMass() only) is still
    // applied exactly as before; the full tensor below is for the
    // diagnostic-only fields (Iyy/Izz/products of inertia, off-axis CG)
    // that never had that adjustment applied to them anyway. This does mean
    // evaluateMassProperties() runs twice per deriv() call rather than
    // once -- deliberately left unconsolidated for this pass to keep the
    // change minimal-risk; a later pass can fold getMass()'s
    // separationDrop into evaluateMassProperties() itself and drop the
    // second call.
    const runtime_mass::RuntimeMassProperties fullMassProperties =
        evaluateMassProperties(c, t);
    const double altitudeAgl = std::max(s.alt, 0.0);

    env::AtmosphericProperties atm = env::Atmosphere::getPropertiesFromSurface(
        altitudeAgl, c.surface_temperature_k, c.surface_pressure_pa);
    const env::WindSample wind = windModel.sample(windStep, t, altitudeAgl);
    const double airVx = s.vx - wind.alongRangeMs;
    const double airVz = s.vz - wind.verticalMs;

    // Speed in body frame: axial (x-body) and lateral
    // pitch = angle from vertical, so:
    //   body-x velocity component = vz*cos(pitch) + vx*sin(pitch)  (axial)
    //   body-z velocity component = -vz*sin(pitch) + vx*cos(pitch) (lateral)
    double cp   = std::cos(s.pitch);
    double sp   = std::sin(s.pitch);
    double v_ax = airVz * cp + airVx * sp;   // axial air velocity in body frame
    double v_lat= -airVz * sp + airVx * cp;  // lateral air velocity in body frame

    double V    = std::sqrt(airVz*airVz + airVx*airVx);
    double q_dyn= 0.5 * atm.density * V * V;

    double thrust = lerp(t, c.mot_t, c.mot_f);
    if (!c.motor_instances.empty()) {
        thrust = 0.0;
        for (const auto& motor : c.motor_instances) {
            if (!motorIsActive(c, motor)) continue;
            const double local = t - motor.resolved_ignition_time_s;
            if (local < 0.0) continue;  // lerp() would otherwise clamp to the curve's first sample
            thrust += lerp(local, motor.time_s, motor.thrust_n);
        }
    }

    // Angle of attack (body axis vs velocity vector)
    double alpha = 0.0;
    double reynolds = 0.0;
    if (V > 0.1) {
        // alpha = angle between body-x axis and velocity vector
        // body-x direction: (sin(pitch) for downrange, cos(pitch) for vertical)
        // velocity direction: (vx/V downrange, vz/V vertical)
        double bx_vr = sp, bz_vr = cp;   // body-x in world frame
        double vx_n  = airVx / V, vz_n = airVz / V;
        double dot   = bx_vr * vx_n + bz_vr * vz_n;
        if (dot > 1.0) dot = 1.0; if (dot < -1.0) dot = -1.0;
        alpha = std::acos(dot);
        // sign of alpha: if v_lat > 0, alpha positive
        if (v_lat > 0) alpha = -alpha;
    }

    // Forces in body frame
    double axialCd = 0.0;
    double tableNormalCoefficient = 0.0;
    const bool usingRasaeroTable = c.useRasaeroAeroTable && !c.rasaeroAeroTable.empty();
    if (V > 0.1) {
        const double mach = V / std::max(atm.speedOfSound, 1e-9);
        const double dynamicViscosity = atm.dynamicViscosity;
        const double kinematicViscosity = atm.density > 0.0
            ? dynamicViscosity / atm.density : 1.0e30;
        reynolds = V * c.drag.aerodynamicLength
            / std::max(kinematicViscosity, 1e-30);
        if (usingRasaeroTable) {
            const auto tableCoeffs = c.rasaeroAeroTable.interpolate(mach, reynolds, std::abs(alpha) / DEG2RAD);
            // CA is an axial-force coefficient, not a conventional CD.
            axialCd = std::max(0.0, thrust > 1.0e-6 ? tableCoeffs.caOn : tableCoeffs.caOff);
            tableNormalCoefficient = std::copysign(tableCoeffs.cn, alpha);
        } else {
            const auto dragResult = barrowman_drag::calculate(c.drag, mach, reynolds, std::abs(alpha));
            axialCd = c.drag.referenceArea > 0.0 ? std::abs(dragResult.axialCd) : c.legacy_cd;
        }
    }
    const double flightMach = V / std::max(atm.speedOfSound, 1e-9);
    double dynamicCna = c.cna_curve.empty()
        ? c.cna : lerp(flightMach, c.aero_mach, c.cna_curve);
    double dynamicCp = c.cp_curve.empty()
        ? c.cp_z : lerp(flightMach, c.aero_mach, c.cp_curve);
    const double stallAngle = 20.0 * DEG2RAD;
    const double effectiveAlpha = std::clamp(alpha, -stallAngle, stallAngle);
    if (usingRasaeroTable && V > 0.1) {
        const double kinematicViscosity = atm.density > 0.0 ? atm.dynamicViscosity / atm.density : 1.0e30;
        reynolds = V * c.drag.aerodynamicLength / std::max(kinematicViscosity, 1e-30);
        const auto tableCoeffs = c.rasaeroAeroTable.interpolate(flightMach, reynolds, std::abs(alpha) / DEG2RAD);
        dynamicCna = tableCoeffs.cnAlphaPerRad;
        dynamicCp = tableCoeffs.cpM;
    }

    double bodyLiftMultiplier = 1.0;
    if (flightMach < 0.05 && std::abs(alpha) > PI/4.0) {
        bodyLiftMultiplier = std::pow(flightMach/0.05, 2.0);
    }
    const double bodyCn = usingRasaeroTable ? 0.0 : bodyLiftMultiplier * 1.1
        * c.drag.bodyLiftPlanformArea / std::max(c.ref_area, 1e-12)
        * std::sin(alpha) * std::abs(std::sin(alpha));
    const double bodyCp = c.drag.bodyLiftPlanformArea > 1e-12
        ? c.drag.bodyLiftPlanformMoment / c.drag.bodyLiftPlanformArea
        : dynamicCp;
    const double normalCoefficient = usingRasaeroTable
        ? tableNormalCoefficient : dynamicCna*effectiveAlpha + bodyCn;
    // Axial drag must oppose the body-axis velocity.  Applying it in a fixed
    // body direction makes a descending or reversed vehicle accelerate into
    // its own flow and can cause an unphysical trajectory divergence.
    const double axialDragDirection = v_ax >= 0.0 ? 1.0 : -1.0;
    double Fx_b  = thrust - axialDragDirection * q_dyn * c.ref_area * axialCd;
    // The body-normal aerodynamic force opposes the lateral body velocity.
    // With alpha's sign convention above, a positive normal coefficient must
    // act in +body-z; the previous sign injected energy into lateral motion.
    double Fz_b  = normalCoefficient * q_dyn * c.ref_area;

    // Convert to world frame
    double Fx_w  = Fx_b * sp + Fz_b * cp;  // downrange
    double Fz_w  = Fx_b * cp - Fz_b * sp;  // vertical
    const double aeroFxW = (-axialDragDirection * q_dyn * c.ref_area * axialCd)
        * sp + Fz_b * cp;
    const double aeroFzW = (-axialDragDirection * q_dyn * c.ref_area * axialCd)
        * cp - Fz_b * sp;

    // Gravity: flat-earth horizontal geometry, but g itself follows the
    // inverse-square law with true distance from Earth's center instead of
    // staying pinned to the launch-site constant.
    const double trueAltitudeAslM = c.launch_altitude_m + altitudeAgl;
    const double g_loc = g0 * std::pow(earthR / (earthR + trueAltitudeAslM), 2.0);
    Fz_w -= mass * g_loc;

    // Recovery drag is based on local wind-relative velocity and is summed
    // across independently active devices.  The force therefore remains
    // dissipative in every attitude and is coupled directly into RK4.
    double chuteDrag = 0.0;
    const double recoveryCdS = activeRecoveryCdS(c.recovery_devices, t);
    if (recoveryCdS > 0.0 && V > 0.1) {
        chuteDrag = 0.5 * atm.density * recoveryCdS * V * V;
        Fx_w -= chuteDrag * airVx / V;
        Fz_w -= chuteDrag * airVz / V;
    }

    // Accelerations
    double ax = Fx_w / mass;
    double az = Fz_w / mass;

    // Until the rocket clears the launch rail, constrain translation and
    // attitude to the rail direction.  This prevents a zero-speed launch
    // from acquiring an unphysical lateral turn before it is guided.
    const double railSp = std::sin(c.launch_angle_rad);
    const double railCp = std::cos(c.launch_angle_rad);
    const double railDistance = std::max(
        0.0,
        s.alt * railCp + s.downrange * railSp);
    const bool onRail = !s.railCleared && railDistance < effectiveRailLengthM(c);
    if (onRail) {
        double railAcceleration = ax * railSp + az * railCp;
        // The pad/rail base is a rigid support, not open air: while thrust
        // has not yet overcome the along-rail weight component (e.g. during
        // ignition ramp-up), the structure holds the vehicle at its
        // starting point instead of letting it sink through the pad.
        if (railDistance <= 0.0 && railAcceleration < 0.0) {
            railAcceleration = 0.0;
        }
        ax = railAcceleration * railSp;
        az = railAcceleration * railCp;
    }

    // Pitching moment
    double stability_margin = dynamicCp - cg_z;
    const double aeroPitchMoment = -q_dyn * c.ref_area * (
        normalCoefficient * stability_margin
        + bodyCn * (bodyCp - cg_z));
    double dampingPitchMoment = 0.0;
    // OpenRocket-style aerodynamic-rate damping: the normal force generated
    // by pitch rate acts through the squared CP-CG lever arm.
    if (V > 0.1) {
        const double dampingLever = std::max(
            std::abs(stability_margin), 0.25 * c.ref_dia);
        dampingPitchMoment = -4.0 * q_dyn * c.ref_area * dynamicCna
            * dampingLever * dampingLever * s.dpitch / V;
    }
    const double My = aeroPitchMoment + dampingPitchMoment;

    double ddpitch = (ixx > 1e-9) ? My / ixx : 0.0;

    d.alt    = s.vz;
    d.vz     = az;
    d.vx     = ax;
    d.pitch  = onRail ? 0.0 : s.dpitch;
    d.dpitch = onRail ? 0.0 : ddpitch;
    d.downrange = s.vx;
    if (telemetry != nullptr) {
        telemetry->thrustN = thrust;
        telemetry->axialCd = axialCd;
        telemetry->cnaPerRad = dynamicCna;
        telemetry->cpM = dynamicCp;
        telemetry->alphaRad = alpha;
        telemetry->reynolds = reynolds;
        telemetry->rocketCgM = cg_z;
        telemetry->ixxKgm2 = ixx;
        telemetry->cgXM = fullMassProperties.cg_x_m;
        telemetry->cgYM = fullMassProperties.cg_y_m;
        telemetry->iyyKgm2 = fullMassProperties.iyy_kgm2;
        telemetry->izzKgm2 = fullMassProperties.izz_kgm2;
        telemetry->ixyKgm2 = fullMassProperties.ixy_kgm2;
        telemetry->ixzKgm2 = fullMassProperties.ixz_kgm2;
        telemetry->iyzKgm2 = fullMassProperties.iyz_kgm2;
        telemetry->aeroDownrangeN = aeroFxW;
        telemetry->aeroVerticalN = aeroFzW;
        telemetry->normalForceN = Fz_b;
        telemetry->aeroPitchMomentNm = aeroPitchMoment;
        telemetry->dampingPitchMomentNm = dampingPitchMoment;
        telemetry->recoveryDragN = chuteDrag;
        telemetry->recoveryCdSM2 = recoveryCdS;
        telemetry->recoveryDragAccelerationMs2 = chuteDrag / std::max(mass, 1.0e-12);
        for (std::size_t i = 0; i < c.recovery_devices.size(); ++i) {
            const auto& device = c.recovery_devices[i];
            const double fraction = recoveryInflationFraction(device, t);
            if (fraction > 0.0 || device.state == RecoveryState::Inflating
                || device.state == RecoveryState::Inflated) {
                telemetry->recoveryInflationFraction += fraction;
                telemetry->activeRecoveryDeviceIndex = static_cast<int>(i);
                telemetry->recoveryState = static_cast<int>(device.state);
            }
        }
        telemetry->gravityForceVerticalN = -mass * g_loc;
        telemetry->netDownrangeN = ax * mass;
        telemetry->netVerticalN = az * mass;
        telemetry->accelerationDownrangeMs2 = ax;
        telemetry->accelerationVerticalMs2 = az;
        telemetry->pitchAccelerationRadS2 = onRail ? 0.0 : ddpitch;
        telemetry->propellantMassKg = propellantMassAt(c, t);
        telemetry->onLaunchRail = onRail;
    }
    return d;
}

static State3 rk4Step(double t, const State3& s, double dt, const SimConfig& c,
                      const env::WindModel& windModel,
                      const env::WindStep& windStep) {
    State3 k1 = deriv(t,        s,           c, windModel, windStep);
    auto   s2 = s; s2.alt+=k1.alt*dt/2; s2.vz+=k1.vz*dt/2; s2.vx+=k1.vx*dt/2;
                   s2.pitch+=k1.pitch*dt/2; s2.dpitch+=k1.dpitch*dt/2;
                   s2.downrange+=k1.downrange*dt/2;
    State3 k2 = deriv(t+dt/2,  s2,           c, windModel, windStep);
    auto   s3 = s; s3.alt+=k2.alt*dt/2; s3.vz+=k2.vz*dt/2; s3.vx+=k2.vx*dt/2;
                   s3.pitch+=k2.pitch*dt/2; s3.dpitch+=k2.dpitch*dt/2;
                   s3.downrange+=k2.downrange*dt/2;
    State3 k3 = deriv(t+dt/2,  s3,           c, windModel, windStep);
    auto   s4 = s; s4.alt+=k3.alt*dt; s4.vz+=k3.vz*dt; s4.vx+=k3.vx*dt;
                   s4.pitch+=k3.pitch*dt; s4.dpitch+=k3.dpitch*dt;
                   s4.downrange+=k3.downrange*dt;
    State3 k4 = deriv(t+dt,    s4,           c, windModel, windStep);

    State3 next = s;
    next.alt      += (dt/6.0)*(k1.alt      +2*k2.alt      +2*k3.alt      +k4.alt);
    next.vz       += (dt/6.0)*(k1.vz       +2*k2.vz       +2*k3.vz       +k4.vz);
    next.vx       += (dt/6.0)*(k1.vx       +2*k2.vx       +2*k3.vx       +k4.vx);
    next.pitch    += (dt/6.0)*(k1.pitch    +2*k2.pitch    +2*k3.pitch    +k4.pitch);
    next.dpitch   += (dt/6.0)*(k1.dpitch   +2*k2.dpitch   +2*k3.dpitch   +k4.dpitch);
    next.downrange+= (dt/6.0)*(k1.downrange+2*k2.downrange+2*k3.downrange+k4.downrange);
    // The pad is a rigid floor while the vehicle is still on the rail (e.g.
    // thrust below weight during ignition ramp-up): it cannot sink below
    // its starting altitude.  This is a numerical safety net on top of the
    // along-rail support in deriv(), catching any residual downward drift
    // from the RK4 stage blend.  Scoped to the on-rail phase only, so it
    // never masks a genuine ground-impact landing after flight.
    const double startRailDistance = std::max(0.0,
        s.alt * std::cos(c.launch_angle_rad) + s.downrange * std::sin(c.launch_angle_rad));
    if (!s.railCleared && startRailDistance < effectiveRailLengthM(c) && next.alt < 0.0) {
        next.alt = 0.0;
        if (next.vz < 0.0) next.vz = 0.0;
    }
    // Keep the reduced-order attitude state within its valid operating range.
    // This is not a tumbling model; after a large attitude excursion the
    // Barrowman small-angle assumptions no longer apply.
    const double maxPitchDeviation = 30.0 * DEG2RAD;
    constexpr double maxPitchRate = 3.0;
    next.pitch = c.launch_angle_rad + std::clamp(
        next.pitch - c.launch_angle_rad,
        -maxPitchDeviation, maxPitchDeviation);
    next.dpitch = std::clamp(next.dpitch, -maxPitchRate, maxPitchRate);
    return next;
}

// Switches the CURRENTLY active vehicle configuration: mutates c's base
// mass/CG/inertia and aero fields to the given phase's precomputed
// attached-vehicle dry snapshot, and restricts c.active_motor_ids to that
// phase's motors. getMass()/getCg()/getIxx()/deriv() read these fields
// directly and need no phase-index branching of their own -- a dropped
// stage's motors and structural mass therefore stop contributing to every
// subsequent derivative evaluation from this call onward. Propellant
// depletion inside the surviving motors remains a runtime function of t via
// the unchanged propellantMassAt() path.
static void applyVehiclePhase(SimConfig& c, const staging::VehiclePhase& phase) {
    c.base_origin_static = phase.static_origin;
    syncLegacyBaselineScalars(c);
    c.ref_dia = phase.ref_dia;
    c.ref_area = phase.ref_area;
    c.cna = phase.cna;
    c.cp_z = phase.cp_z;
    c.aero_mach = phase.aero_mach;
    c.cna_curve = phase.cna_curve;
    c.cp_curve = phase.cp_curve;
    c.drag = phase.drag;
    // Always enabled once staging is live -- even a phase with zero motors
    // (phase.active_motor_instance_ids empty) must NOT fall back to
    // "unfiltered" and resurrect a dropped stage's motors.
    c.motor_filter_enabled = true;
    c.active_motor_ids.clear();
    c.active_motor_ids.insert(
        phase.active_motor_instance_ids.begin(), phase.active_motor_instance_ids.end());
}

// ── main ────────────────────────────────────────────────────────────────
int main(int argc, char** argv) {
    if (argc >= 2 && std::string(argv[1]) == "--datcom-classify") {
        if (argc < 4) { std::cerr << "Usage: roketsim_cli --datcom-classify input.json output.json\n"; return 2; }
        try {
            std::ifstream input(argv[2]); if (!input.is_open()) throw std::runtime_error("Cannot open input");
            json request; input >> request;
            std::ofstream output(argv[3]); if (!output.is_open()) throw std::runtime_error("Cannot write output");
            output << datcom_equivalent::roketsim_adapter::classify(request).dump(2);
            return 0;
        } catch (const std::exception& error) { std::cerr << "DATCOM classification error: " << error.what() << std::endl; return 1; }
    }
    if (argc >= 2 && std::string(argv[1]) == "--motor-catalog") {
        if (argc < 4) {
            std::cerr << "Usage: roketsim_cli --motor-catalog motors.db output.json\n";
            return 2;
        }
        try {
            std::ofstream output(argv[3]);
            if (!output.is_open()) throw std::runtime_error("Cannot write " + std::string(argv[3]));
            output << motor_catalog::catalog(argv[2]).dump(2);
            return 0;
        } catch (const std::exception& error) {
            std::cerr << "Motor catalog error: " << error.what() << std::endl;
            return 1;
        }
    }
    if (argc >= 2 && std::string(argv[1]) == "--motor-curve") {
        if (argc < 5) {
            std::cerr << "Usage: roketsim_cli --motor-curve motors.db motor_id output.json\n";
            return 2;
        }
        try {
            std::ofstream output(argv[4]);
            if (!output.is_open()) throw std::runtime_error("Cannot write " + std::string(argv[4]));
            output << motor_catalog::curve(argv[2], std::stoi(argv[3])).dump(2);
            return 0;
        } catch (const std::exception& error) {
            std::cerr << "Motor curve error: " << error.what() << std::endl;
            return 1;
        }
    }
    if (argc >= 2 && std::string(argv[1]) == "--materials") {
        if (argc < 3) {
            std::cerr << "Usage: roketsim_cli --materials output.json\n";
            return 2;
        }
        try {
            json materials = json::array();
            const auto append = [&](Malzeme::Type type, const char* typeName,
                                    const char* unit) {
                for (const Malzeme& material
                     : VeriTabanimalzeme::alVeritabani(type)) {
                    const MalzemeGrubu group = material.alGrup();
                    materials.push_back({
                        {"key", std::string(typeName) + ":" + material.alIsim()},
                        {"name", material.alIsim()},
                        {"type", typeName},
                        {"density", material.alYogunluk()},
                        {"density_unit", unit},
                        {"shear_modulus", material.alKaymaModulu()},
                        {"group", group.isim},
                        {"group_key", group.veritabanIsim},
                        {"group_priority", group.oncelik},
                        {"user_defined", material.egerKullaniciTanimli()},
                    });
                }
            };
            append(Malzeme::Type::YIGIN, "BULK", "kg/m3");
            append(Malzeme::Type::YUZEY, "SURFACE", "kg/m2");
            append(Malzeme::Type::CIZGI, "LINE", "kg/m");
            append(Malzeme::Type::OZEL, "CUSTOM", "custom");
            std::ofstream output(argv[2]);
            if (!output.is_open()) {
                std::cerr << "Error: Cannot write " << argv[2] << std::endl;
                return 1;
            }
            output << json{{"materials", materials}}.dump(2);
            std::cout << "Material catalog complete." << std::endl;
            return 0;
        } catch (const std::exception& error) {
            std::cerr << "Material catalog error: " << error.what() << std::endl;
            return 1;
        }
    }
    if (argc >= 2 && std::string(argv[1]) == "--motor") {
        if (argc < 4) {
            std::cerr << "Usage: roketsim_cli --motor input.json output.json\n";
            return 2;
        }
        try {
            std::ifstream input(argv[2]);
            if (!input.is_open()) {
                std::cerr << "Error: Cannot open " << argv[2] << std::endl;
                return 1;
            }
            json request;
            input >> request;
            const json result = motor_analysis::process(request);
            std::ofstream output(argv[3]);
            if (!output.is_open()) {
                std::cerr << "Error: Cannot write " << argv[3] << std::endl;
                return 1;
            }
            output << result.dump(2);
            std::cout << "Motor analysis complete." << std::endl;
            return 0;
        } catch (const std::exception& error) {
            std::cerr << "Motor analysis error: " << error.what() << std::endl;
            return 1;
        }
    }
    if (argc >= 2 && std::string(argv[1]) == "--analyze") {
        if (argc < 4) {
            std::cerr << "Usage: roketsim_cli --analyze input.json output.json\n";
            return 2;
        }
        try {
            std::ifstream input(argv[2]);
            if (!input.is_open()) {
                std::cerr << "Error: Cannot open " << argv[2] << std::endl;
                return 1;
            }
            json request;
            input >> request;
            const json result = request.value("provider", std::string{}) == "datcom_equivalent"
                ? datcom_equivalent::roketsim_adapter::analyze(request)
                : design::analyze(request, productionAeroExtensionPipeline());
            std::ofstream output(argv[3]);
            if (!output.is_open()) {
                std::cerr << "Error: Cannot write " << argv[3] << std::endl;
                return 1;
            }
            output << result.dump(2);
            std::cout << (request.value("provider", std::string{}) == "datcom_equivalent"
                ? "DATCOM Equivalent analysis complete." : "Design analysis complete.") << std::endl;
            return 0;
        } catch (const std::exception& error) {
            std::cerr << "Design analysis error: " << error.what() << std::endl;
            return 1;
        }
    }
    if (argc >= 2 && std::string(argv[1]) == "--stage-plan") {
        // Diagnostic mode: parses "stages"/"motor_instances" and reports the
        // precomputed per-phase attached-vehicle snapshots and any
        // unsupported-trigger warnings, without running a flight.  Lets
        // Checkpoint 1's vehicle-phase representation be inspected/tested in
        // isolation, ahead of Checkpoint 2 wiring it into live integration.
        if (argc < 4) {
            std::cerr << "Usage: roketsim_cli --stage-plan input.json output.json\n";
            return 2;
        }
        try {
            std::ifstream input(argv[2]);
            if (!input.is_open()) {
                std::cerr << "Error: Cannot open " << argv[2] << std::endl;
                return 1;
            }
            json j;
            input >> j;
            if (!j.contains("components")) {
                throw std::runtime_error("--stage-plan requires a components array.");
            }
            std::vector<staging::StagingWarning> warnings;
            std::vector<staging::MotorInstance> motorInstances =
                staging::parseMotorInstances(j, warnings);
            staging::StagePlan plan = staging::parseStagePlan(j, warnings);
            std::vector<staging::VehiclePhase> phases =
                staging::buildVehiclePhases(
                    j, plan, motorInstances, productionAeroExtensionPipeline());

            json phasesJson = json::array();
            for (const auto& phase : phases) phasesJson.push_back(staging::vehiclePhaseToJson(phase));
            json warningsJson = json::array();
            for (const auto& warning : warnings) warningsJson.push_back(warning.toJson());

            std::ofstream output(argv[3]);
            if (!output.is_open()) {
                std::cerr << "Error: Cannot write " << argv[3] << std::endl;
                return 1;
            }
            output << json{{"phases", phasesJson}, {"warnings", warningsJson}}.dump(2);
            std::cout << "Stage plan analysis complete." << std::endl;
            return 0;
        } catch (const std::exception& error) {
            std::cerr << "Stage plan analysis error: " << error.what() << std::endl;
            return 1;
        }
    }
    if (argc >= 2 && std::string(argv[1]) == "--environment-preview") {
        if (argc < 4) {
            std::cerr << "Usage: roketsim_cli --environment-preview input.json output.json\n";
            return 2;
        }
        try {
            std::ifstream input(argv[2]);
            if (!input.is_open()) throw std::runtime_error("Cannot open preview input");
            json request;
            input >> request;
            env::WindConfig wind;
            wind.meanSpeedMs = request.value("wind_speed_ms", 0.0);
            wind.fromDirectionDeg = request.value(
                "wind_direction_from_deg", request.value("wind_direction_deg", 0.0));
            wind.directionGradientDegPerKm =
                request.value("wind_direction_gradient_deg_per_km", 0.0);
            wind.launchAzimuthDeg = request.value("launch_azimuth_deg", 0.0);
            wind.shearModel = env::WindModel::parseShearModel(
                request.value("wind_shear_model", std::string("power_law")));
            wind.referenceHeightM = request.value("wind_reference_height_m", 10.0);
            wind.minimumEffectiveHeightM =
                request.value("wind_min_effective_height_m", 1.0);
            wind.shearExponent = request.value("wind_shear_exponent", 0.14);
            wind.roughnessLengthM = request.value("wind_roughness_length_m", 0.03);
            wind.turbulenceEnabled = request.value("turbulence_enabled", false);
            wind.turbulenceRmsMs = request.value("turbulence_rms_ms", 0.0);
            wind.turbulenceAlpha = request.value("turbulence_alpha", 1.0);
            wind.turbulencePoles = std::clamp(
                request.value("turbulence_poles", 5), 1, 16);
            wind.turbulenceSampleHz = request.value("turbulence_sample_hz", 20.0);
            wind.turbulenceSeed = request.value("turbulence_seed", std::uint32_t{1});
            env::WindModel model(wind);

            const double launchAsl = request.value("launch_altitude_m", 0.0);
            const double surfaceTemperatureK =
                request.value("temperature_c", 15.0) + 273.15;
            const double surfacePressurePa =
                request.value("pressure_hpa", 1013.25) * 100.0;
            const double maxAltitude = std::clamp(
                request.value("profile_max_altitude_m", 6000.0), 0.0, 30000.0);
            const double altitudeStep = std::clamp(
                request.value("profile_step_m", 500.0), 1.0, 5000.0);
            const double controlSpeed = std::max(
                request.value("dynamic_pressure_check_speed_ms", 120.0), 0.0);
            const double gustFactor = std::max(
                request.value("wind_gust_factor", 1.35), 1.0);
            json profile = json::array();
            const env::WindStep steadyStep = model.prepareStep(0.0, 0.0);
            for (double h = 0.0; h <= maxAltitude + 1.0e-9; h += altitudeStep) {
                const auto atmosphere = env::Atmosphere::getPropertiesFromSurface(
                    h, surfaceTemperatureK, surfacePressurePa);
                const auto sample = model.sample(steadyStep, 0.0, h);
                const double meanSpeed = std::hypot(sample.meanEastMs, sample.meanNorthMs);
                profile.push_back({
                    {"altitude_agl_m", h},
                    {"altitude_asl_m", launchAsl + h},
                    {"temperature_k", atmosphere.temperature},
                    {"temperature_c", atmosphere.temperature - 273.15},
                    {"pressure_pa", atmosphere.pressure},
                    {"pressure_hpa", atmosphere.pressure / 100.0},
                    {"density_kg_m3", atmosphere.density},
                    {"speed_of_sound_ms", atmosphere.speedOfSound},
                    {"dynamic_viscosity_pa_s", atmosphere.dynamicViscosity},
                    {"dynamic_pressure_check_pa",
                        0.5 * atmosphere.density * controlSpeed * controlSpeed},
                    {"mean_wind_speed_ms", meanSpeed},
                    {"gust_limit_ms", gustFactor * meanSpeed},
                    {"wind_direction_from_deg", sample.directionFromDeg},
                    {"wind_heading_to_deg", sample.directionToDeg},
                    {"wind_u_east_ms", sample.meanEastMs},
                    {"wind_v_north_ms", sample.meanNorthMs},
                });
            }
            if (profile.empty() || profile.back()["altitude_agl_m"].get<double>()
                    < maxAltitude - 1.0e-9) {
                const double h = maxAltitude;
                const auto atmosphere = env::Atmosphere::getPropertiesFromSurface(
                    h, surfaceTemperatureK, surfacePressurePa);
                const auto sample = model.sample(steadyStep, 0.0, h);
                const double meanSpeed = std::hypot(sample.meanEastMs, sample.meanNorthMs);
                profile.push_back({
                    {"altitude_agl_m", h}, {"altitude_asl_m", launchAsl + h},
                    {"temperature_k", atmosphere.temperature},
                    {"temperature_c", atmosphere.temperature - 273.15},
                    {"pressure_pa", atmosphere.pressure},
                    {"pressure_hpa", atmosphere.pressure / 100.0},
                    {"density_kg_m3", atmosphere.density},
                    {"speed_of_sound_ms", atmosphere.speedOfSound},
                    {"dynamic_viscosity_pa_s", atmosphere.dynamicViscosity},
                    {"dynamic_pressure_check_pa",
                        0.5 * atmosphere.density * controlSpeed * controlSpeed},
                    {"mean_wind_speed_ms", meanSpeed},
                    {"gust_limit_ms", gustFactor * meanSpeed},
                    {"wind_direction_from_deg", sample.directionFromDeg},
                    {"wind_heading_to_deg", sample.directionToDeg},
                    {"wind_u_east_ms", sample.meanEastMs},
                    {"wind_v_north_ms", sample.meanNorthMs},
                });
            }

            json turbulence = json::array();
            const double duration = std::clamp(
                request.value("turbulence_preview_duration_s", 30.0), 1.0, 300.0);
            const double dtPreview = 1.0 / wind.turbulenceSampleHz;
            const double previewAltitude = std::max(
                request.value("turbulence_preview_altitude_m", wind.referenceHeightM), 0.0);
            for (double t = 0.0; t <= duration + 1.0e-9; t += dtPreview) {
                const auto step = model.prepareStep(t, dtPreview);
                const auto sample = model.sample(step, t, previewAltitude);
                const double directionRadians =
                    sample.directionFromDeg * DEG2RAD;
                const double unit = wind.turbulenceRmsMs > 0.0
                    ? (-sample.turbulenceEastMs * std::sin(directionRadians)
                       - sample.turbulenceNorthMs * std::cos(directionRadians))
                        / wind.turbulenceRmsMs
                    : 0.0;
                turbulence.push_back({
                    {"time_s", t},
                    {"pink_noise_unit", unit},
                    {"turbulence_east_ms", sample.turbulenceEastMs},
                    {"turbulence_north_ms", sample.turbulenceNorthMs},
                    {"turbulence_vertical_ms", sample.verticalMs},
                    {"mean_wind_east_ms", sample.meanEastMs},
                    {"mean_wind_north_ms", sample.meanNorthMs},
                    {"instantaneous_wind_east_ms", sample.eastMs},
                    {"instantaneous_wind_north_ms", sample.northMs},
                    {"seed", wind.turbulenceSeed},
                });
            }
            std::ofstream output(argv[3]);
            if (!output.is_open()) throw std::runtime_error("Cannot write preview output");
            output << json({
                {"source", "C++ env::Atmosphere + env::WindModel"},
                {"profile", profile},
                {"turbulence", turbulence},
                {"wind_model", env::WindModel::shearModelName(wind.shearModel)},
                {"minimum_effective_height_m", wind.minimumEffectiveHeightM},
                {"turbulence_application_mode", "speed_magnitude_only"},
            }).dump(2);
            return 0;
        } catch (const std::exception& error) {
            std::cerr << "Environment preview error: " << error.what() << '\n';
            return 1;
        }
    }

    std::string input_file  = "sim_input.json";
    std::string output_file = "sim_output.csv";
    std::string summary_file;
    if (argc > 1) input_file  = argv[1];
    if (argc > 2) output_file = argv[2];
    if (argc > 3) summary_file = argv[3];

    SimConfig c;
    RecoveryConfig recovery;
    double dt = 0.01, t_end = 120.0, launch_alt_m = 0.0;
    // The static design total (design::analyze's mass_kg, captured before the
    // motor-instance loop below repurposes c.mass_dry into a post-burn dry
    // value).  This is the launch-mass ground truth the flight-initial-state
    // guard checks the solver against.
    double staticLaunchMassKg = 0.0;

    // Parse JSON in a separate scope so the json object is destroyed before
    // we open the output file (avoids nlohmann heap corruption on exit with MinGW)
    {
        std::ifstream fin(input_file);
        if (!fin.is_open()) {
            std::cerr << "Error: Cannot open " << input_file << std::endl;
            return 1;
        }
        json j;
        fin >> j;
        fin.close();
        std::cout << "Loaded " << input_file << std::endl;

        json rocket;
        if (j.contains("components")) {
            rocket = design::analyze(j, productionAeroExtensionPipeline());
        } else {
            rocket = j["rocket"];
        }

        // The rail guides the vehicle only until its aft-most (lowest) rail
        // button clears the top of the rail -- find that button's distance
        // from the vehicle's tail so effectiveRailLengthM() can subtract it.
        // Left at 0.0 (full rail_length_m, prior behavior) if no rail button
        // is present in the design.
        {
            const double totalLength = rocket.value("total_length_m", 0.0);
            double lowestButtonPositionM = -1.0;
            for (const auto& component : rocket.value("components", json::array())) {
                if (component.value("type", std::string()) != "Rail Button") continue;
                double maxInstanceOffset = 0.0;
                for (const auto& offset : component.value(
                        "instance_axial_offsets_m", json::array())) {
                    maxInstanceOffset = std::max(maxInstanceOffset, offset.get<double>());
                }
                lowestButtonPositionM = std::max(
                    lowestButtonPositionM,
                    component.value("position_m", 0.0) + maxInstanceOffset);
            }
            if (lowestButtonPositionM >= 0.0 && totalLength > 0.0) {
                c.rail_button_offset_from_tail_m = std::max(
                    0.0, totalLength - lowestButtonPositionM);
            }
        }

        c.mass_dry       = rocket["mass_kg"].get<double>();
        staticLaunchMassKg = c.mass_dry;
        c.cg_z           = rocket["cg_m"][2].get<double>();
        c.ref_dia        = rocket["reference_diameter_m"].get<double>();
        c.ref_area       = PI * std::pow(c.ref_dia / 2.0, 2);
        c.cna            = rocket["cna"].get<double>();
        c.cp_z           = rocket["cp_m"].get<double>();
        if (rocket.contains("mach_curve"))
            c.aero_mach = rocket["mach_curve"].get<std::vector<double>>();
        if (rocket.contains("cna_curve"))
            c.cna_curve = rocket["cna_curve"].get<std::vector<double>>();
        if (rocket.contains("cp_curve"))
            c.cp_curve = rocket["cp_curve"].get<std::vector<double>>();
        if (rocket.contains("drag_model")) {
            c.drag = barrowman_drag::fromJson(rocket["drag_model"]);
        }
        c.legacy_cd = rocket.contains("cd_power_on")
            ? rocket["cd_power_on"].get<double>()
            : (rocket.contains("cd_power_off")
                ? rocket["cd_power_off"].get<double>() : 0.5);
        c.Ixx            = rocket["inertia_kgm2"][0][0].get<double>();
        // Whole-vehicle static snapshot, general (full CG vector + full
        // inertia tensor, not just cg_m[2]/inertia_kgm2[0][0]) -- the ONE
        // baseline every branch below subtracts its own motor(s) out of.
        // See runtime_mass_properties.hpp / evaluateMassProperties().
        {
            const auto& cgJson = rocket["cg_m"];
            const auto& inertiaJson = rocket["inertia_kgm2"];
            c.base_origin_static = runtime_mass::OriginAccumulator{};
            c.base_origin_static.add({c.mass_dry, cgJson[0].get<double>(),
                cgJson[1].get<double>(), cgJson[2].get<double>(),
                inertiaJson[0][0].get<double>(), inertiaJson[1][1].get<double>(),
                inertiaJson[2][2].get<double>(), inertiaJson[0][1].get<double>(),
                inertiaJson[0][2].get<double>(), inertiaJson[1][2].get<double>()});
        }
        syncLegacyBaselineScalars(c);

        const bool hasMotorInstances = j.contains("motor_instances")
            && j["motor_instances"].is_array() && !j["motor_instances"].empty();

        c.prop_mass_init = 0.0;
        if (!hasMotorInstances) {
            // Legacy single-motor path: extract the one "Motor" component's
            // mass/CG/inertia design::analyze aggregated, so the flight
            // solver's own propellant-depletion model can own the
            // time-varying part.  Skipped when motor_instances is present --
            // that path below owns motor mass/CG/inertia directly from its
            // own (possibly multiple) per-motor records instead.
            for (const auto& component : rocket.value("components", json::array())) {
                if (component.value("type", std::string()) != "Motor") continue;
                c.motor_count = std::max(
                    component.value("motor_cluster_count", 1), 1);
                c.motor_initial_mass = component.value("mass_kg", 0.0);
                c.motor_dry_mass = component.value("motor_dry_mass_kg", 0.0);
                c.prop_mass_init = component.value("motor_propellant_mass_kg", 0.0);
                c.motor_cg_z = component.value("absolute_cg_m", 0.0);
                const double motorI = component.value("inertia_transverse_kgm2", 0.0);
                c.motor_i_per_mass = c.motor_initial_mass > 0.0
                    ? motorI / c.motor_initial_mass : 0.0;
                c.base_origin_static.subtract({c.motor_initial_mass, 0.0, 0.0, c.motor_cg_z,
                    motorI, motorI, 0.0, 0.0, 0.0, 0.0});
                // Defensive clamp matching the pre-refactor scalar code
                // (mass only -- see the equivalent clamp/comment in
                // staging.hpp's buildVehiclePhases()).
                c.base_origin_static.mass_kg = std::max(0.0, c.base_origin_static.mass_kg);
                syncLegacyBaselineScalars(c);
                c.mass_dry = c.base_mass + c.motor_dry_mass;
                break;
            }
        }

        if (j.contains("motor")) {
            c.mot_t = j["motor"]["time_s"].get<std::vector<double>>();
            c.mot_f = j["motor"]["thrust_n"].get<std::vector<double>>();
            // The supplied curve is per motor.  Cluster mass, propellant and
            // inertia already come from design::analyze as totals, so thrust
            // must use the same multiplicity during flight integration.
            for (double& thrust : c.mot_f) {
                thrust *= static_cast<double>(c.motor_count);
            }
        }
        if (hasMotorInstances) {
            // The importer may hand the flight engine several independently
            // ignited canonical motor instances.  Curves are mandatory here:
            // no thrust/mass profile is guessed from designation or impulse.
            c.motor_instances = staging::parseMotorInstances(j, c.staging_warnings);
            // design::analyze includes motor components in rocket mass.  Move
            // each one's initial (full wet) contribution out of the fixed
            // airframe, at its OWN position, so the event-driven instances
            // own all time-varying mass/CG/inertia and evaluateMassProperties()
            // never double-counts them. c.base_origin_static here is still
            // the untouched whole-vehicle snapshot set above -- the legacy
            // single-motor branch that would otherwise mutate it is skipped
            // whenever hasMotorInstances is true.
            for (const auto& motor : c.motor_instances) {
                const double transverse = motor.wet_mass_kg * motor.transverse_inertia_per_mass;
                c.base_origin_static.subtract({motor.wet_mass_kg, 0.0, 0.0, motor.cg_z_m,
                    transverse, transverse, 0.0, 0.0, 0.0, 0.0});
            }
            // Defensive clamp matching the pre-refactor scalar code (mass
            // only -- see buildVehiclePhases()'s equivalent clamp).
            c.base_origin_static.mass_kg = std::max(0.0, c.base_origin_static.mass_kg);
            syncLegacyBaselineScalars(c);
        }
        if (j.contains("stages") && j["stages"].is_array() && j.contains("components")) {
            // Each phase re-runs the canonical calculator from the original
            // (pre-analysis) request, filtered to that phase's attached
            // component ids -- not from ``rocket``, which is already
            // design::analyze's OUTPUT shape and not valid re-input.
            c.stage_plan = staging::parseStagePlan(j, c.staging_warnings);
            c.vehicle_phases = staging::buildVehiclePhases(
                j, c.stage_plan, c.motor_instances,
                productionAeroExtensionPipeline());
        }
        if (hasMotorInstances) {
            // Resolves every motor's absolute resolved_ignition_time_s
            // (getMass()/getCg()/getIxx()/deriv() read it unconditionally
            // now) and the MOTOR_IGNITION/MOTOR_BURNOUT/STAGE_SEPARATION
            // schedule the flight loop below clamps its RK4 steps to. With
            // no stage plan, every motor resolves as "launch" -- unchanged
            // from Checkpoint 1's non-staged motor_instances behavior.
            c.event_schedule = staging::resolveEventSchedule(
                c.stage_plan, c.motor_instances, c.staging_warnings);
        }
        if (!c.vehicle_phases.empty()) {
            applyVehiclePhase(c, c.vehicle_phases.front());
        }
        const json& simulation = j["simulation"];
        const std::string rasaeroTablePath = simulation.value(
            "rasaero_aero_table_path", std::string());
        if (!rasaeroTablePath.empty()) {
            c.rasaeroAeroTable.loadCsv(rasaeroTablePath);
            c.useRasaeroAeroTable = true;
            std::cout << "Loaded RASAero table: " << rasaeroTablePath << std::endl;
        }
        c.launch_angle_deg = simulation["launch_angle_deg"].get<double>();
        c.launch_angle_rad = c.launch_angle_deg * DEG2RAD;
        dt                 = j["simulation"]["step_size_ms"].get<double>() / 1000.0;
        t_end              = j["simulation"]["t_end_s"].get<double>();
        launch_alt_m       = j["simulation"]["launch_altitude_m"].get<double>();
        c.launch_altitude_m = launch_alt_m;
        c.launch_latitude_deg =
            simulation.value("launch_latitude_deg", 0.0);
        c.launch_longitude_deg =
            simulation.value("launch_longitude_deg", 0.0);
        c.requested_dof = simulation.value("dof", std::string("3-DoF"));
        c.earth_model = simulation.value(
            "earth_model", std::string("WGS84 local tangent plane"));
        c.surface_temperature_k =
            simulation.value("temperature_c", 15.0) + 273.15;
        c.surface_pressure_pa =
            simulation.value("pressure_hpa", 1013.25) * 100.0;
        c.rail_length_m = simulation.value("rail_length_m", 5.0);
        c.approximate_stage_separation_enabled = simulation.value(
            "approx_stage_separation_enabled", false);
        c.approximate_stage_separation_time_s = simulation.value(
            "approx_stage_separation_time_s", 0.0);
        c.approximate_stage_dropped_mass_kg = std::max(0.0, simulation.value(
            "approx_stage_dropped_mass_kg", 0.0));
        if (!c.vehicle_phases.empty() && c.approximate_stage_separation_enabled) {
            c.staging_warnings.push_back({
                "APPROXIMATE_STAGING_IGNORED", "", "", "", "",
                "Real stages/motor_instances are present; the deprecated "
                "scalar approximate-staging mass drop is ignored to avoid "
                "double-dropping mass. Real staging is authoritative.",
            });
        }
        c.wind.meanSpeedMs = simulation.value("wind_speed_ms", 0.0);
        c.wind.fromDirectionDeg = simulation.value(
            "wind_direction_from_deg",
            simulation.value("wind_direction_deg", 0.0));
        c.wind.directionGradientDegPerKm =
            simulation.value("wind_direction_gradient_deg_per_km", 0.0);
        c.wind.launchAzimuthDeg = simulation.value("launch_azimuth_deg", 0.0);
        c.wind.shearModel = env::WindModel::parseShearModel(
            simulation.value("wind_shear_model", std::string("power_law")));
        c.wind.referenceHeightM = simulation.value("wind_reference_height_m", 10.0);
        c.wind.minimumEffectiveHeightM =
            simulation.value("wind_min_effective_height_m", 1.0);
        c.wind.shearExponent = simulation.value("wind_shear_exponent", 0.14);
        c.wind.roughnessLengthM = simulation.value("wind_roughness_length_m", 0.03);
        c.wind.turbulenceEnabled = simulation.value("turbulence_enabled", false);
        c.wind.turbulenceRmsMs = simulation.value("turbulence_rms_ms", 0.0);
        c.wind.turbulenceAlpha = simulation.value("turbulence_alpha", 1.0);
        c.wind.turbulencePoles = std::clamp(
            simulation.value("turbulence_poles", 5), 1, 16);
        c.wind.turbulenceSampleHz = simulation.value("turbulence_sample_hz", 20.0);
        c.wind.turbulenceSeed = simulation.value("turbulence_seed", std::uint32_t{1});

        if (j.contains("recovery")) {
            const json& r = j["recovery"];
            recovery.enabled = r.value("recovery_enabled", true);
            recovery.mode = r.value("recovery_mode", std::string("Apogee"));
            recovery.altitude_threshold =
                r.value("recovery_mode_code", std::string()) == "altitude_threshold";
            recovery.main_deploy_altitude_m = r.value("main_deploy_altitude_m", 300.0);
            recovery.drogue_deploy_delay_s = r.value("drogue_deploy_delay_s", 0.0);
            recovery.main_deploy_delay_s = r.value("main_deploy_delay_s", 0.0);
            recovery.parachute_diameter_m = r.value("parachute_diameter_m", 1.2);
            recovery.parachute_cd = r.value("parachute_cd", 1.5);
            recovery.parachute_area_m2 = r.value("parachute_area_m2", 1.13);
            recovery.target_descent_rate_ms = r.value("target_descent_rate_ms", 6.0);
            recovery.mass_margin_pct = r.value("recovery_mass_margin_pct", 10.0);

            if (r.contains("devices") && r["devices"].is_array()) {
                std::size_t deviceIndex = 0;
                for (const auto& raw : r["devices"]) {
                    RecoveryDevice device;
                    device.id = raw.value("id", std::string("recovery-") + std::to_string(deviceIndex));
                    device.name = raw.value("name", device.id);
                    device.device_type = raw.value("device_type", std::string("generic"));
                    device.trigger = raw.value("trigger", std::string("APOGEE"));
                    device.trigger_altitude_agl_m = raw.value("trigger_altitude_agl_m", 0.0);
                    device.trigger_time_s = raw.value("trigger_time_s", 0.0);
                    device.deployment_delay_s = std::max(0.0, raw.value("deployment_delay_s", 0.0));
                    device.inflation_time_s = std::max(0.0, raw.value("inflation_time_s", 1.0));
                    device.enabled = raw.value("enabled", true);
                    if (raw.contains("CdS_m2")) {
                        device.cd_s_m2 = std::max(0.0, raw["CdS_m2"].get<double>());
                    } else {
                        device.cd_s_m2 = std::max(0.0, raw.value("Cd", 0.0))
                            * std::max(0.0, raw.value("reference_area_m2", 0.0));
                    }
                    c.recovery_devices.push_back(device);
                    ++deviceIndex;
                }
            }
            // Backward-compatible single device mapping for existing project
            // files/UI.  New callers should send canonical recovery.devices.
            if (c.recovery_devices.empty() && recovery.enabled) {
                RecoveryDevice device;
                device.id = "legacy-recovery";
                device.name = "Recovery";
                device.device_type = recovery.altitude_threshold ? "main" : "generic";
                device.trigger = recovery.altitude_threshold ? "ALTITUDE_AGL_DESCENDING" : "APOGEE";
                device.trigger_altitude_agl_m = recovery.main_deploy_altitude_m;
                device.deployment_delay_s = recovery.altitude_threshold
                    ? recovery.main_deploy_delay_s : recovery.drogue_deploy_delay_s;
                device.inflation_time_s = std::max(0.0, r.value("inflation_time_s", 1.0));
                device.cd_s_m2 = std::max(0.0, recovery.parachute_cd * recovery.parachute_area_m2);
                c.recovery_devices.push_back(device);
            }
        }
    }  // json j is destroyed here — before any file I/O


    // Approximate transverse inertia from JSON (row 0 col 0 = Ixx)

    c.t_burn = c.mot_t.empty() ? 0.0 : c.mot_t.back();
    c.dm_dt  = (c.t_burn > 0 && c.prop_mass_init > 0)
               ? (c.prop_mass_init / c.t_burn) : 0.0;
    for (std::size_t i = 1; i < c.mot_t.size(); ++i) {
        c.motor_total_impulse += 0.5 * (c.mot_f[i - 1] + c.mot_f[i])
            * std::max(c.mot_t[i] - c.mot_t[i - 1], 0.0);
    }


    // Sanity check
    double stability_margin = c.cp_z - c.cg_z;
    std::cout << "CNa=" << c.cna
              << "  CG=" << c.cg_z << "m"
              << "  CP=" << c.cp_z << "m"
              << "  SM=" << stability_margin / c.ref_dia << " cal" << std::endl;

    if (stability_margin < 0) {
        std::cerr << "WARNING: Rocket is UNSTABLE (CP ahead of CG). Expect divergence." << std::endl;
    }

    // Flight-initial-state guard: the mass the EOM integrator actually starts
    // from must equal the static launch mass design::analyze reported for
    // the identical component list (which includes the motor's full wet
    // mass).  A silent mismatch here means the flight vehicle and the
    // static-analysis vehicle have diverged upstream (e.g. an unmerged
    // motor), and every downstream force/acceleration would be wrong without
    // any visible symptom in this tool.
    const double solverInitialMassKg = getMass(c, 0.0);
    const double initialMassErrorKg = std::abs(solverInitialMassKg - staticLaunchMassKg);
    const double initialMassToleranceKg = std::max(1e-6, 1e-4 * staticLaunchMassKg);
    if (initialMassErrorKg > initialMassToleranceKg) {
        std::cerr << "ERROR: flight initial mass (" << solverInitialMassKg
                   << " kg) does not match static launch mass ("
                   << staticLaunchMassKg << " kg); diff=" << initialMassErrorKg
                   << " kg. The flight vehicle is missing mass present in the"
                      " static design (commonly an unmerged motor)."
                   << std::endl;
    }

    // Initial state
    State3 s;
    s.alt      = 0.0;  // Solver altitude is AGL; launch altitude remains ASL metadata.
    s.vz       = 0.0;
    s.vx       = 0.0;
    s.pitch    = c.launch_angle_rad;  // angle from vertical
    s.dpitch   = 0.0;
    s.downrange= 0.0;

    env::WindModel windModel(c.wind);
    const geodesy::GeodeticPosition launchGeodetic{
        c.launch_latitude_deg,
        c.launch_longitude_deg,
        c.launch_altitude_m,
    };
    // Validate the launch coordinate before producing any output.
    (void)geodesy::Wgs84::geodeticToEcef(launchGeodetic);

    FILE* out = fopen(output_file.c_str(), "w");
    if (!out) {
        std::cerr << "Failed to open output: " << output_file << std::endl;
        return 1;
    }
    // The first 38 columns are the legacy v1 contract.  Appended columns are
    // the canonical v2 solver telemetry contract; no legacy names are renamed.
    // The final 4 columns are the Checkpoint 4 staging telemetry contract --
    // "attached" (physically present) is distinct from "firing" (currently
    // producing thrust); see firingMotorCount(). Meaningful even for a
    // legacy/non-staged run (active_vehicle_phase=0, attached_stage_count=1).
    fprintf(out, "t,altitude_m,downrange_m,crossrange_m,vx,vy,vz,roll,pitch,yaw,mass,wind_downrange_ms,wind_vertical_ms,wind_mean_downrange_ms,wind_turbulence_downrange_ms,air_velocity_downrange_ms,air_velocity_vertical_ms,airspeed_ms,mach,dynamic_pressure_pa,altitude_agl_m,wind_east_ms,wind_north_ms,wind_mean_east_ms,wind_mean_north_ms,wind_turbulence_east_ms,wind_turbulence_north_ms,wind_direction_from_deg,temperature_k,pressure_pa,density_kg_m3,speed_of_sound_ms,dynamic_viscosity_pa_s,east_m,north_m,latitude_deg,longitude_deg,altitude_wgs84_m,velocity_downrange_ms,velocity_crossrange_ms,velocity_vertical_ms,ground_speed_ms,horizontal_speed_ms,vertical_speed_ms,relative_air_velocity_downrange_ms,relative_air_velocity_vertical_ms,acceleration_downrange_ms2,acceleration_vertical_ms2,acceleration_magnitude_ms2,pitch_rad,pitch_rate_rad_s,pitch_acceleration_rad_s2,angle_of_attack_rad,angle_of_attack_deg,reynolds_number,CD,CNa_per_rad,CP_m,static_margin_cal,thrust_N,motor_burning,burn_fraction,propellant_mass_kg,rocket_total_mass_kg,rocket_cg_m,Ixx_kgm2,aero_force_downrange_N,aero_force_vertical_N,aerodynamic_force_N,drag_force_N,normal_force_N,aero_pitch_moment_Nm,damping_pitch_moment_Nm,net_pitch_moment_Nm,gravity_force_vertical_N,recovery_drag_force_N,recovery_deployed,recovery_state,recovery_active_device_index,recovery_total_cds_m2,recovery_inflation_fraction,recovery_drag_acceleration_ms2,horizontal_drift_distance_m,on_launch_rail,active_vehicle_phase,attached_stage_count,attached_motor_count,firing_motor_count,rocket_cg_x_m,rocket_cg_y_m,Iyy_kgm2,Izz_kgm2,Ixy_kgm2,Ixz_kgm2,Iyz_kgm2\n");

    double t           = 0.0;
    // Export every computed step: the CSV must carry the same points the
    // integrator actually produced, not a coarser 0.1 s resample of them.
    double out_interval= dt;
    double next_out    = 0.0;
    bool   nan_abort   = false;
    double apogee_m = 0.0;
    double apogee_t = 0.0;
    bool apogee_detected = false;
    bool recovery_deployed = false;
    double recovery_deploy_t = 0.0;
    double recovery_deploy_alt = 0.0;
    double max_wind_speed = 0.0;
    double max_turbulence = 0.0;
    double max_airspeed = 0.0;
    bool ground_impact_detected = false;
    bool rail_exit_recorded = false, burnout_recorded = false;
    bool ignition_recorded = false;
    double previous_thrust = 0.0;
    json events = json::array();
    events.push_back({{"type", "simulation_start"}, {"time_s", 0.0},
        {"sample_index", 0}, {"state", {{"altitude_m", 0.0}, {"vertical_speed_ms", 0.0}}}});

    double maxRecoveryDragN = 0.0;
    double maxRecoveryDecelerationMs2 = 0.0;
    auto recoveryEvent = [&](const char* type, const RecoveryDevice& device, double eventTime) {
        const double agl = std::max(s.alt, 0.0);
        const auto step = windModel.prepareStep(eventTime, std::max(dt, 1.0e-6));
        const auto wind = windModel.sample(step, eventTime, agl);
        const double airVx = s.vx - wind.alongRangeMs;
        const double airVz = s.vz - wind.verticalMs;
        const double relAirspeed = std::hypot(airVx, airVz);
        const auto atm = env::Atmosphere::getPropertiesFromSurface(
            agl, c.surface_temperature_k, c.surface_pressure_pa);
        events.push_back({{"type", type}, {"time_s", eventTime},
            {"sample_index", static_cast<int>(std::llround(eventTime / out_interval))},
            {"device_id", device.id}, {"device_name", device.name},
            {"device_type", device.device_type}, {"recovery_state", recoveryStateName(device.state)},
            {"altitude_agl_m", agl}, {"vehicle_velocity", {{"downrange_ms", s.vx}, {"vertical_ms", s.vz}}},
            {"relative_airspeed_ms", relAirspeed},
            {"dynamic_pressure_pa", 0.5 * atm.density * relAirspeed * relAirspeed}});
    };
    auto advanceRecoveryStates = [&](double eventTime) {
        for (auto& device : c.recovery_devices) {
            if (!device.enabled) continue;
            if (device.state == RecoveryState::Triggered
                && eventTime + 1.0e-9 >= device.triggered_at_s + device.deployment_delay_s) {
                device.state = RecoveryState::Deploying;
                recoveryEvent("RECOVERY_DEPLOYMENT_START", device, eventTime);
                device.state = RecoveryState::Inflating;
                device.inflation_start_time_s = eventTime;
                recoveryEvent("RECOVERY_INFLATION_START", device, eventTime);
                if (device.inflation_time_s <= 1.0e-9) {
                    device.state = RecoveryState::Inflated;
                    recoveryEvent("RECOVERY_FULLY_INFLATED", device, eventTime);
                }
            }
            if (device.state == RecoveryState::Inflating && device.inflation_time_s > 1.0e-9
                && eventTime + 1.0e-9 >= device.inflation_start_time_s + device.inflation_time_s) {
                device.state = RecoveryState::Inflated;
                recoveryEvent("RECOVERY_FULLY_INFLATED", device, eventTime);
            }
        }
    };
    auto triggerRecovery = [&](RecoveryDevice& device, double eventTime) {
        if (!device.enabled || device.state != RecoveryState::Stowed) return;
        device.state = RecoveryState::Triggered;
        device.triggered_at_s = eventTime;
        recoveryEvent("RECOVERY_TRIGGER", device, eventTime);
        events.push_back({{"type", "recovery_deployment_command"}, {"time_s", eventTime},
            {"device_id", device.id}, {"state", {{"altitude_m", std::max(s.alt, 0.0)}}}});
    };

    // ── Checkpoint 2: axial event state machine ─────────────────────────
    // c.event_schedule is already sorted by time (staging::resolveEventSchedule).
    // scheduleCursor is the index of the next unprocessed event.
    std::size_t scheduleCursor = 0;
    std::unordered_map<std::string, std::size_t> stageIndexById;
    for (std::size_t i = 0; i < c.stage_plan.stages.size(); ++i) {
        stageIndexById[c.stage_plan.stages[i].id] = i;
    }
    std::size_t currentPhaseIndex = 0;

    // Groups every unprocessed event at exactly the next scheduled time and
    // applies them in the required semantic order -- burnouts, then
    // separations (+ the vehicle-configuration change they cause), then
    // ignitions -- regardless of the order they appear in the sorted
    // schedule. Called only when t has just reached that exact time (the
    // RK4 step immediately before this call was clamped to stop there), so
    // no derivative evaluation ever blends the dropped stage's mass/aero
    // with the newly active stage's thrust.
    auto processEventGroup = [&](double eventTime, const State3& state) {
        constexpr double kEventTimeTolerance = 1e-9;
        // Captured once, before AND after the phase switch below, and
        // included verbatim in STAGE_SEPARATION -- proves position/velocity/
        // pitch/pitch-rate continuity at the exact event instant (zero-
        // impulse separation, Phase 1's documented assumption), rather than
        // approximating "continuity" from the nearest regular-grid samples.
        const json stateJson = {
            {"downrange_m", state.downrange}, {"altitude_m", state.alt},
            {"vx_ms", state.vx}, {"vz_ms", state.vz},
            {"pitch_rad", state.pitch}, {"pitch_rate_rad_s", state.dpitch},
        };
        std::size_t groupEnd = scheduleCursor;
        while (groupEnd < c.event_schedule.events.size()
            && c.event_schedule.events[groupEnd].time_s <= eventTime + kEventTimeTolerance) {
            ++groupEnd;
        }
        // Same-timestamp events are ordered by resolution_order (the order
        // their dependency was resolved in -- see staging.hpp's
        // resolveEventSchedule()), not by a fixed burnout/separation/
        // ignition type sequence. This matters as of Checkpoint 4.5: a
        // next_stage_ignition-triggered STAGE_SEPARATION depends on an upper
        // stage's MOTOR_IGNITION, which must therefore be processed (and
        // logged) first -- the reverse of the classic own_motor_burnout
        // ordering (MOTOR_BURNOUT -> STAGE_SEPARATION -> MOTOR_IGNITION),
        // which this same sort still reproduces exactly because that
        // ordering IS the dependency order there too. Ties (only possible
        // between a motor's own ignition and its own burnout) break by type.
        std::vector<std::size_t> order(groupEnd - scheduleCursor);
        for (std::size_t i = 0; i < order.size(); ++i) order[i] = scheduleCursor + i;
        std::stable_sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
            const auto& ea = c.event_schedule.events[a];
            const auto& eb = c.event_schedule.events[b];
            if (ea.resolution_order != eb.resolution_order)
                return ea.resolution_order < eb.resolution_order;
            auto typeRank = [](staging::ResolvedEvent::Type type) {
                switch (type) {
                    case staging::ResolvedEvent::MotorIgnition: return 0;
                    case staging::ResolvedEvent::MotorBurnout: return 1;
                    default: return 2;
                }
            };
            return typeRank(ea.type) < typeRank(eb.type);
        });
        const int sampleIndex = static_cast<int>(std::llround(eventTime / out_interval));

        // A single pass in dependency order (not three fixed by-type passes)
        // -- required as of Checkpoint 4.5 so a next_stage_ignition
        // separation's causing MOTOR_IGNITION is genuinely processed first,
        // while an own_motor_burnout separation's causing MOTOR_BURNOUT still
        // is too (both fall out of the same sort, see above).
        for (const std::size_t i : order) {
            const staging::ResolvedEvent& ev = c.event_schedule.events[i];
            if (ev.type == staging::ResolvedEvent::MotorBurnout) {
                events.push_back({{"type", "MOTOR_BURNOUT"}, {"time_s", eventTime},
                    {"sample_index", sampleIndex}, {"stage_id", ev.stage_id}, {"motor_id", ev.motor_id},
                    {"vehicle_phase", currentPhaseIndex}});
            } else if (ev.type == staging::ResolvedEvent::StageSeparation) {
                const auto found = stageIndexById.find(ev.stage_id);
                if (found == stageIndexById.end()) continue;
                const std::size_t separatingIndex = found->second;
                const std::size_t newPhaseIndex = separatingIndex + 1;
                if (newPhaseIndex >= c.vehicle_phases.size()) continue;
                const std::size_t phaseBefore = currentPhaseIndex;
                const double massBefore = getMass(c, eventTime);
                const double cgBefore = getCg(c, eventTime);
                const double ixxBefore = getIxx(c, eventTime, cgBefore);
                const std::size_t attachedBefore = c.vehicle_phases[phaseBefore].attached_stage_ids.size();
                applyVehiclePhase(c, c.vehicle_phases[newPhaseIndex]);
                currentPhaseIndex = newPhaseIndex;
                const double massAfter = getMass(c, eventTime);
                const double cgAfter = getCg(c, eventTime);
                const double ixxAfter = getIxx(c, eventTime, cgAfter);
                const std::size_t attachedAfter = c.vehicle_phases[currentPhaseIndex].attached_stage_ids.size();
                events.push_back({{"type", "STAGE_SEPARATION"}, {"time_s", eventTime},
                    {"sample_index", sampleIndex}, {"stage_id", ev.stage_id},
                    {"phase_before", phaseBefore}, {"phase_after", currentPhaseIndex},
                    {"mass_before_kg", massBefore}, {"mass_after_kg", massAfter},
                    {"cg_before_m", cgBefore}, {"cg_after_m", cgAfter},
                    {"ixx_before_kgm2", ixxBefore}, {"ixx_after_kgm2", ixxAfter},
                    {"attached_stage_count_before", attachedBefore},
                    {"attached_stage_count_after", attachedAfter},
                    {"state_before", stateJson}, {"state_after", stateJson}});
                const std::size_t attachedMotorsAfter =
                    c.vehicle_phases[currentPhaseIndex].active_motor_instance_ids.size();
                events.push_back({{"type", "VEHICLE_CONFIGURATION_CHANGE"}, {"time_s", eventTime},
                    {"sample_index", sampleIndex},
                    // phase_before/phase_after are the Checkpoint 4 canonical
                    // names; active_vehicle_phase/active_motor_count are kept
                    // verbatim for existing consumers (Checkpoint 2/3 tests).
                    {"phase_before", phaseBefore}, {"phase_after", currentPhaseIndex},
                    {"active_vehicle_phase", currentPhaseIndex},
                    {"attached_stage_count", attachedAfter},
                    {"active_motor_count", attachedMotorsAfter},
                    {"attached_motor_count", attachedMotorsAfter},
                    {"firing_motor_count", firingMotorCount(c, eventTime)}});
            } else if (ev.type == staging::ResolvedEvent::MotorIgnition) {
                events.push_back({{"type", "MOTOR_IGNITION"}, {"time_s", eventTime},
                    {"sample_index", sampleIndex}, {"stage_id", ev.stage_id}, {"motor_id", ev.motor_id},
                    {"vehicle_phase", currentPhaseIndex}});
            }
        }
        scheduleCursor = groupEnd;
    };

    while (t <= t_end) {
        if (s.alt < -1.0 && t > 1.0) {
            std::cout << "Ground impact at t = " << t << "s" << std::endl;
            ground_impact_detected = true;
            break;
        }

        const env::WindStep windStep = windModel.prepareStep(t, dt);
        const double altitudeAgl = std::max(s.alt, 0.0);
        const env::WindSample wind = windModel.sample(windStep, t, altitudeAgl);
        const double airVx = s.vx - wind.alongRangeMs;
        const double airVz = s.vz - wind.verticalMs;
        const double airspeed = std::sqrt(airVx * airVx + airVz * airVz);
        const auto outputAtmosphere = env::Atmosphere::getPropertiesFromSurface(
            altitudeAgl, c.surface_temperature_k, c.surface_pressure_pa);
        const double outputMach = airspeed
            / std::max(outputAtmosphere.speedOfSound, 1.0e-9);
        const double outputDynamicPressure = 0.5 * outputAtmosphere.density
            * airspeed * airspeed;
        max_wind_speed = std::max(max_wind_speed, std::abs(wind.alongRangeMs));
        max_turbulence = std::max(
            max_turbulence, std::abs(wind.turbulenceAlongRangeMs));
        max_airspeed = std::max(max_airspeed, airspeed);

        if (t >= next_out - 1e-9) {
            TelemetryMetrics metrics;
            (void)deriv(t, s, c, windModel, windStep, &metrics);
            maxRecoveryDragN = std::max(maxRecoveryDragN, metrics.recoveryDragN);
            maxRecoveryDecelerationMs2 = std::max(
                maxRecoveryDecelerationMs2, metrics.recoveryDragAccelerationMs2);
            const int sampleIndex = static_cast<int>(std::llround(t / out_interval));
            if (!ignition_recorded && metrics.thrustN > 1e-6) {
                events.push_back({{"type", "ignition"}, {"time_s", t},
                    {"sample_index", sampleIndex}, {"state", {{"thrust_N", metrics.thrustN}}}});
                ignition_recorded = true;
            }
            if (!rail_exit_recorded && !metrics.onLaunchRail) {
                events.push_back({{"type", "rail_exit"}, {"time_s", t},
                    {"sample_index", sampleIndex}, {"state", {{"altitude_m", s.alt}}}});
                rail_exit_recorded = true;
            }
            if (!burnout_recorded && previous_thrust > 1e-6 && metrics.thrustN <= 1e-6) {
                events.push_back({{"type", "motor_burnout"}, {"time_s", t},
                    {"sample_index", sampleIndex}, {"state", {{"altitude_m", s.alt}}}});
                burnout_recorded = true;
            }
            previous_thrust = metrics.thrustN;
            double mass = getMass(c, t);
            double V    = std::sqrt(s.vz*s.vz + s.vx*s.vx);
            // pitch from vertical in degrees
            double pitch_deg = s.pitch * 180.0 / PI;
            const auto enu = geodesy::Wgs84::launchFrameToEnu(
                s.downrange, 0.0, altitudeAgl, c.wind.launchAzimuthDeg);
            const auto geographic = geodesy::Wgs84::enuToGeodetic(
                enu, launchGeodetic);
            fprintf(out, "%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f",
                    t,
                    s.alt,        // altitude_m
                    s.downrange,  // downrange_m
                    0.0,          // crossrange_m
                    s.vx,         // vx (downrange)
                    0.0,          // vy
                    s.vz,         // vz (vertical)
                    0.0,          // roll
                    pitch_deg,    // pitch (from vertical)
                    0.0,          // yaw
                    mass,
                    wind.alongRangeMs,
                    wind.verticalMs,
                    wind.meanAlongRangeMs,
                    wind.turbulenceAlongRangeMs,
                    airVx,
                    airVz,
                    airspeed,
                    outputMach,
                    outputDynamicPressure,
                    altitudeAgl,
                    wind.eastMs,
                    wind.northMs,
                    wind.meanEastMs,
                    wind.meanNorthMs,
                    wind.turbulenceEastMs,
                    wind.turbulenceNorthMs,
                    wind.directionFromDeg,
                    outputAtmosphere.temperature,
                    outputAtmosphere.pressure,
                    outputAtmosphere.density,
                    outputAtmosphere.speedOfSound,
                    outputAtmosphere.dynamicViscosity,
                    enu.eastM,
                    enu.northM,
                    geographic.latitudeDeg,
                    geographic.longitudeDeg,
                    geographic.altitudeM);
            const double groundSpeed = std::sqrt(s.vx * s.vx + s.vz * s.vz);
            const double initialPropellant = std::max(propellantMassAt(c, 0.0), 0.0);
            const double burnFraction = initialPropellant > 0.0
                ? std::clamp(1.0 - metrics.propellantMassKg / initialPropellant, 0.0, 1.0)
                : 0.0;
            const bool stagingActive = !c.vehicle_phases.empty();
            const double activeVehiclePhase = stagingActive
                ? static_cast<double>(currentPhaseIndex) : 0.0;
            const double attachedStageCount = stagingActive
                ? static_cast<double>(c.vehicle_phases[currentPhaseIndex].attached_stage_ids.size())
                : 1.0;
            const double attachedMotorCount = stagingActive
                ? static_cast<double>(c.vehicle_phases[currentPhaseIndex].active_motor_instance_ids.size())
                : (c.motor_instances.empty()
                    ? (c.mot_t.size() < 2 ? 0.0 : 1.0)
                    : static_cast<double>(c.motor_instances.size()));
            const double firingMotors = static_cast<double>(firingMotorCount(c, t));
            const std::vector<double> appendedTelemetry{
                    s.vx, 0.0, s.vz, groundSpeed, std::abs(s.vx), s.vz,
                    airVx, airVz, metrics.accelerationDownrangeMs2,
                    metrics.accelerationVerticalMs2,
                    std::sqrt(metrics.accelerationDownrangeMs2 * metrics.accelerationDownrangeMs2 + metrics.accelerationVerticalMs2 * metrics.accelerationVerticalMs2),
                    s.pitch, s.dpitch, metrics.pitchAccelerationRadS2,
                    metrics.alphaRad, metrics.alphaRad / DEG2RAD, metrics.reynolds,
                    metrics.axialCd, metrics.cnaPerRad, metrics.cpM,
                    c.ref_dia > 1e-12 ? (metrics.cpM - metrics.rocketCgM) / c.ref_dia : 0.0,
                    metrics.thrustN, metrics.thrustN > 1e-6 ? 1.0 : 0.0,
                    burnFraction, metrics.propellantMassKg, mass, metrics.rocketCgM,
                    metrics.ixxKgm2, metrics.aeroDownrangeN, metrics.aeroVerticalN,
                    std::sqrt(metrics.aeroDownrangeN * metrics.aeroDownrangeN + metrics.aeroVerticalN * metrics.aeroVerticalN),
                    std::abs(outputDynamicPressure * c.ref_area * metrics.axialCd),
                    metrics.normalForceN, metrics.aeroPitchMomentNm,
                    metrics.dampingPitchMomentNm,
                    metrics.aeroPitchMomentNm + metrics.dampingPitchMomentNm,
                    metrics.gravityForceVerticalN, metrics.recoveryDragN,
                    metrics.recoveryCdSM2 > 0.0 ? 1.0 : 0.0,
                    static_cast<double>(metrics.recoveryState),
                    static_cast<double>(metrics.activeRecoveryDeviceIndex),
                    metrics.recoveryCdSM2, metrics.recoveryInflationFraction,
                    metrics.recoveryDragAccelerationMs2, std::abs(s.downrange),
                    metrics.onLaunchRail ? 1.0 : 0.0,
                    activeVehiclePhase, attachedStageCount, attachedMotorCount, firingMotors,
                    metrics.cgXM, metrics.cgYM, metrics.iyyKgm2, metrics.izzKgm2,
                    metrics.ixyKgm2, metrics.ixzKgm2, metrics.iyzKgm2};
            for (const double value : appendedTelemetry) fprintf(out, ",%f", value);
            fprintf(out, "\n");
            next_out += out_interval;
        }

        const double previousVz = s.vz;
        // Exact event stepping: clamp this step so it never integrates past
        // a scheduled MOTOR_IGNITION/MOTOR_BURNOUT/STAGE_SEPARATION time.
        // t0 -> t_event is integrated with the OLD vehicle only; the event
        // is applied exactly at t_event (below); the remaining t_event ->
        // t0+dt interval is a fresh, separately-clamped step on the next
        // loop iteration. No root-finding: every staging event time was
        // already resolved analytically before the loop started.
        double stepDt = dt;
        const double nextStagingEventTime = scheduleCursor < c.event_schedule.events.size()
            ? c.event_schedule.events[scheduleCursor].time_s
            : std::numeric_limits<double>::infinity();
        double nextRecoveryEventTime = std::numeric_limits<double>::infinity();
        for (const auto& device : c.recovery_devices) {
            if (!device.enabled) continue;
            if (device.state == RecoveryState::Triggered) {
                nextRecoveryEventTime = std::min(nextRecoveryEventTime,
                    device.triggered_at_s + device.deployment_delay_s);
            } else if (device.state == RecoveryState::Inflating && device.inflation_time_s > 1.0e-9) {
                nextRecoveryEventTime = std::min(nextRecoveryEventTime,
                    device.inflation_start_time_s + device.inflation_time_s);
            }
            // Locate a descending AGL crossing inside the pending RK step.
            // The actual trigger still executes only after the partial step,
            // preserving the one-shot event state machine.  Linear velocity
            // extrapolation is a bounded V1 root estimate; a later adaptive
            // integrator can replace it without changing the device contract.
            if (device.state == RecoveryState::Stowed
                && device.trigger == "ALTITUDE_AGL_DESCENDING" && apogee_detected
                && s.vz < -1.0e-9 && s.alt > device.trigger_altitude_agl_m) {
                const double crossingTime = t + (s.alt - device.trigger_altitude_agl_m) / -s.vz;
                // Floor at 1 microsecond: without it, a linear-extrapolation
                // crossing estimate that lands a hair short of the threshold
                // (nonlinear drag deceleration, or plain floating-point
                // rounding on s.alt) re-clamps to an ever-smaller stepDt on
                // the next iteration -- a Zeno's-paradox stall that
                // approaches, but numerically never reaches, the threshold.
                // Skipping the clamp below this floor lets that step run at
                // full dt instead, which reliably crosses the threshold (dt
                // is orders of magnitude larger than any realistic sub-
                // microsecond remaining gap) so the post-step trigger check
                // fires normally, at a physically negligible (<1us) timing
                // cost.
                if (crossingTime < t + dt - 1.0e-9 && crossingTime > t + 1.0e-6) {
                    nextRecoveryEventTime = std::min(nextRecoveryEventTime, crossingTime);
                }
            }
        }
        const double nextEventTime = std::min(nextStagingEventTime, nextRecoveryEventTime);
        const bool willHitEvent = nextEventTime < t + dt - 1e-9;
        if (willHitEvent) stepDt = std::max(0.0, nextEventTime - t);

        if (stepDt > 1e-9) {
            const env::WindStep integrationWindStep = willHitEvent
                ? windModel.prepareStep(t, stepDt) : windStep;
            s = rk4Step(t, s, stepDt, c, windModel, integrationWindStep);
            t += stepDt;
            if (!s.railCleared) {
                const double railDistance = std::max(0.0,
                    s.alt * std::cos(c.launch_angle_rad)
                    + s.downrange * std::sin(c.launch_angle_rad));
                s.railCleared = railDistance >= effectiveRailLengthM(c);
            }
        } else {
            t = nextEventTime;
        }
        if (willHitEvent) {
            if (nextStagingEventTime <= t + 1.0e-9) processEventGroup(t, s);
            advanceRecoveryStates(t);
        }
        if (s.alt > apogee_m) {
            apogee_m = s.alt;
            apogee_t = t;
        }

        if (!apogee_detected && previousVz > 0.0 && s.vz <= 0.0) {
            apogee_detected = true;
            apogee_t = t;
            apogee_m = std::max(apogee_m, s.alt);
            events.push_back({{"type", "apogee"}, {"time_s", t},
                {"sample_index", static_cast<int>(std::llround(t / out_interval))},
                {"state", {{"altitude_m", s.alt}, {"vertical_speed_ms", s.vz}}}});
        }
        // Trigger each stowed device once.  The canonical apogee event above
        // remains the sole apogee detector; altitude triggers require descent
        // and use AGL.  TIME is intentionally deterministic, while MANUAL
        // awaits a future raw-event bridge and never self-fires.
        for (auto& device : c.recovery_devices) {
            if (!device.enabled || device.state != RecoveryState::Stowed) continue;
            const bool apogeeTrigger = device.trigger == "APOGEE" && apogee_detected;
            const bool altitudeTrigger = device.trigger == "ALTITUDE_AGL_DESCENDING"
                && apogee_detected && s.vz < 0.0
                && std::max(s.alt, 0.0) <= device.trigger_altitude_agl_m;
            const bool timeTrigger = device.trigger == "TIME" && t >= device.trigger_time_s;
            if (apogeeTrigger || altitudeTrigger || timeTrigger) {
                triggerRecovery(device, t);
                recovery_deployed = true;
                recovery_deploy_t = t;
                recovery_deploy_alt = std::max(s.alt, 0.0);
                const double descentRate = std::sqrt(2.0 * getMass(c, t) * g0
                    / std::max(1.225 * device.cd_s_m2, 1e-9));
                t_end = std::max(t_end, t + recovery_deploy_alt / std::max(descentRate, 0.1) + 5.0);
            }
        }

        if (std::isnan(s.alt) || std::isnan(s.vz) || std::isnan(s.pitch)) {
            std::cerr << "NaN at t=" << t
                      << " alt=" << s.alt
                      << " vz=" << s.vz
                      << " pitch=" << s.pitch << std::endl;
            nan_abort = true;
            break;
        }
    }

    fflush(out);
    fclose(out);
    const auto landingEnu = geodesy::Wgs84::launchFrameToEnu(
        s.downrange, 0.0, 0.0, c.wind.launchAzimuthDeg);
    const auto landingGeodetic = geodesy::Wgs84::enuToGeodetic(
        landingEnu, launchGeodetic);
    if (ground_impact_detected) {
        events.push_back({{"type", "ground_impact"}, {"time_s", t},
            {"state", {{"altitude_m", 0.0}}}});
    }
    events.push_back({{"type", "simulation_end"}, {"time_s", t},
        {"state", {{"altitude_m", std::max(s.alt, 0.0)}}}});
    if (!summary_file.empty()) {
        json recoverySummary = recoveryResult(
            recovery, c.mass_dry, apogee_m, apogee_t);
        recoverySummary["model"] = "Recovery Model V1 finite-mass RK4";
        recoverySummary["inflation_law"] = "smoothstep (numerical approximation; not a validated canopy filling law)";
        recoverySummary["maximum_recovery_drag_force_N"] = maxRecoveryDragN;
        recoverySummary["maximum_recovery_deceleration_ms2"] = maxRecoveryDecelerationMs2;
        recoverySummary["total_descent_time_s"] = apogee_detected ? std::max(0.0, t - apogee_t) : 0.0;
        if (ground_impact_detected) {
            recoverySummary["touchdown_vertical_velocity_ms"] = s.vz;
            recoverySummary["touchdown_total_velocity_ms"] = std::hypot(s.vx, s.vz);
            recoverySummary["average_stabilized_descent_rate_ms"] = std::abs(s.vz);
        }
        recoverySummary["horizontal_drift_distance_m"] = std::abs(s.downrange);
        json deviceSummaries = json::array();
        for (const auto& device : c.recovery_devices) {
            json item = {{"id", device.id}, {"name", device.name},
                {"device_type", device.device_type}, {"trigger", device.trigger},
                {"CdS_m2", device.cd_s_m2}, {"final_state", recoveryStateName(device.state)}};
            for (const auto& event : events) {
                if (event.value("device_id", std::string()) != device.id) continue;
                const std::string type = event.value("type", std::string());
                if (type == "RECOVERY_TRIGGER") {
                    item["trigger_time_s"] = event.value("time_s", 0.0);
                    item["trigger_altitude_agl_m"] = event.value("altitude_agl_m", 0.0);
                } else if (type == "RECOVERY_DEPLOYMENT_START") {
                    item["deployment_start_time_s"] = event.value("time_s", 0.0);
                } else if (type == "RECOVERY_FULLY_INFLATED") {
                    item["fully_inflated_time_s"] = event.value("time_s", 0.0);
                }
            }
            deviceSummaries.push_back(item);
        }
        recoverySummary["devices"] = deviceSummaries;
        if (recovery_deployed) {
            recoverySummary["deploy_time_s"] = recovery_deploy_t;
            recoverySummary["deploy_altitude_m"] = recovery_deploy_alt;
            recoverySummary["deployed_in_flight"] = true;
        }
        json stagingWarningsJson = json::array();
        for (const auto& warning : c.staging_warnings) stagingWarningsJson.push_back(warning.toJson());
        json summary = {
            {"telemetry_schema_version", 2},
            {"events", events},
            // Unsupported-trigger and deprecated-staging diagnostics must be
            // visible on a real flight run, not only through the --stage-plan
            // probe mode (Checkpoint 4 Part E).
            {"staging_warnings", stagingWarningsJson},
            {"apogee_m", apogee_m},
            {"apogee_t", apogee_t},
            {"recovery", recoverySummary},
            {"geodesy", {
                {"model", "WGS84"},
                {"dynamics_frame", "local launch tangent plane"},
                {"launch", {
                    {"latitude_deg", launchGeodetic.latitudeDeg},
                    {"longitude_deg", launchGeodetic.longitudeDeg},
                    {"altitude_m", launchGeodetic.altitudeM},
                }},
                {"launch_azimuth_deg", c.wind.launchAzimuthDeg},
                {"requested_dof", c.requested_dof},
                {"actual_dynamics_dof", "3-DoF launch-plane"},
                {"earth_model_ui", c.earth_model},
            }},
            {"landing", {
                {"ground_impact_detected", ground_impact_detected},
                {"time_s", t},
                {"latitude_deg", landingGeodetic.latitudeDeg},
                {"longitude_deg", landingGeodetic.longitudeDeg},
                {"altitude_wgs84_m", landingGeodetic.altitudeM},
                {"east_m", landingEnu.eastM},
                {"north_m", landingEnu.northM},
                {"downrange_m", s.downrange},
                {"crossrange_m", 0.0},
                {"horizontal_distance_m",
                    geodesy::Wgs84::horizontalDistanceM(landingEnu)},
                {"bearing_deg", geodesy::Wgs84::bearingDeg(landingEnu)},
                {"source", ground_impact_detected
                    ? "ground impact state" : "final simulated state"},
            }},
            {"wind", {
                {"mean_speed_ms", c.wind.meanSpeedMs},
                {"from_direction_deg", c.wind.fromDirectionDeg},
                {"direction_gradient_deg_per_km", c.wind.directionGradientDegPerKm},
                {"launch_azimuth_deg", c.wind.launchAzimuthDeg},
                {"shear_model", env::WindModel::shearModelName(c.wind.shearModel)},
                {"minimum_effective_height_m", c.wind.minimumEffectiveHeightM},
                {"turbulence_enabled", c.wind.turbulenceEnabled},
                {"turbulence_rms_ms", c.wind.turbulenceRmsMs},
                {"turbulence_alpha", c.wind.turbulenceAlpha},
                {"turbulence_poles", c.wind.turbulencePoles},
                {"turbulence_sample_hz", c.wind.turbulenceSampleHz},
                {"turbulence_seed", c.wind.turbulenceSeed},
                {"turbulence_application_mode", "speed_magnitude_only"},
                {"max_projected_wind_ms", max_wind_speed},
                {"max_turbulence_ms", max_turbulence},
                {"max_airspeed_ms", max_airspeed},
                {"solver_dimension", "2D launch-plane projection"},
            }},
            {"atmosphere", {
                {"model", "surface_anchored_dry_air_troposphere"},
                {"surface_temperature_k", c.surface_temperature_k},
                {"surface_pressure_pa", c.surface_pressure_pa},
                {"launch_altitude_asl_m", c.launch_altitude_m},
                {"lapse_rate_k_per_m", 0.0065},
            }},
        };
        std::ofstream summaryOutput(summary_file);
        if (!summaryOutput.is_open()) {
            std::cerr << "Failed to write summary: " << summary_file << std::endl;
            return 1;
        }
        summaryOutput << summary.dump(2);
    }
    std::cout << "Done." << std::endl;
    return nan_abort ? 1 : 0;
}
