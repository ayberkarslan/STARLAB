// staging.hpp — Axial multi-stage flight runtime contracts (Phase 1).
//
// Represents the stage plan and the precomputed per-phase "attached vehicle"
// dry/structural snapshots used by the flight engine to switch mass/CG/
// inertia/aero/active-motor-set discontinuously at a stage separation, while
// leaving propellant-depletion mass/CG/inertia a runtime function of time
// (see roketsim_cli.cpp's getMass()/getCg()/getIxx()).
//
// Reuses the canonical design::analyze() calculator for every phase's dry
// snapshot instead of a second mass/aero implementation. See
// docs/superpowers/specs/2026-08-20-multistage-flight-runtime-design.md.
#pragma once

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "nlohmann/json.hpp"
#include "design_analysis.hpp"
#include "barrowman_drag.hpp"
#include "runtime_mass_properties.hpp"

namespace staging {

using json = nlohmann::json;

// A single independently-ignited motor belonging to one stage. Mirrors the
// previously-unwired SimConfig::MotorInstance JSON shape one-for-one; moved
// here so the staging runtime contracts and the flight solver share one
// definition.
struct MotorInstance {
    std::string id;
    std::string stage_id;
    std::vector<double> time_s;
    std::vector<double> thrust_n;
    double wet_mass_kg = 0.0;
    double dry_mass_kg = 0.0;
    double cg_z_m = 0.0;
    double transverse_inertia_per_mass = 0.0;
    double ignition_delay_s = 0.0;
    // Normalized trigger type: "launch", "previous_motor_burnout",
    // "previous_stage_separation", or an unrecognized/unsupported raw value
    // preserved verbatim for diagnostics. This is the ONLY field the state
    // machine itself ever branches on (Checkpoint 4's single normalization
    // boundary lives in ork_parser.py, upstream of this contract).
    std::string ignition_event = "launch";
    // Opaque diagnostic passthrough of the true originating trigger string
    // (e.g. OpenRocket's own "automatic"/"burnout" spelling) -- never
    // inspected for meaning here, only echoed back in warnings so a user
    // reading a diagnostic doesn't need to know RoketSiM's internal
    // vocabulary. Defaults to ignition_event when the importer/caller
    // supplies no separate raw value (e.g. hand-built payloads, where the
    // supplied trigger string already IS the "raw" one).
    std::string ignition_raw_trigger;
    bool launch_stage = true;
    // Absolute flight time this motor ignites, resolved by
    // resolveEventSchedule() from ignition_event + ignition_delay_s (and,
    // for a non-launch trigger, the referenced stage's already-resolved
    // burnout/separation time). +infinity means "never ignites" -- an
    // unsupported trigger, or (with no StagePlan at all) untouched at its
    // default until resolveEventSchedule() runs. A motor_instances payload
    // with no "stages" array resolves every motor as "launch" regardless of
    // its ignition_event string, matching the only semantics that were ever
    // reachable before Checkpoint 2 (see resolveEventSchedule()).
    double resolved_ignition_time_s = 0.0;
};

inline const std::unordered_set<std::string>& supportedIgnitionTriggerTypes() {
    static const std::unordered_set<std::string> kSupported = {
        "launch", "previous_motor_burnout", "previous_stage_separation",
    };
    return kSupported;
}

inline const std::unordered_set<std::string>& supportedSeparationTriggerTypes() {
    // Checkpoint 4.5: next_stage_ignition (OpenRocket's "upperignition") --
    // deterministic, resolved from the next stage's own ignition time, no
    // root finder. ejection_charge/altitude remain deliberately unsupported.
    static const std::unordered_set<std::string> kSupported = {
        "own_motor_burnout", "next_stage_ignition",
    };
    return kSupported;
}

// One axial stage. ``component_ids``/``motor_instance_ids`` are this stage's
// OWN structural components and motors only (not cumulative) -- the
// continuing-vehicle attached set for a phase is the union over the
// remaining stages, computed by buildVehiclePhases().
struct StageDefinition {
    std::string id;
    std::vector<std::string> component_ids;
    std::vector<std::string> motor_instance_ids;
    // Normalized trigger type ("own_motor_burnout", "next_stage_ignition",
    // or an unrecognized/unsupported value) -- the only field the state
    // machine itself ever branches on.
    std::string separation_type = "own_motor_burnout";
    double separation_delay_s = 0.0;
    // Opaque diagnostic passthrough of the true originating trigger string
    // (e.g. OpenRocket's "upperignition"/"ejection"), never inspected for
    // meaning here. Defaults to separation_type when the caller supplies no
    // separate raw value.
    std::string separation_raw_value;
};

// Stages ordered by separation sequence: stages[0] is the first to separate
// (typically the aft-most/first-ignited stage), stages.back() never
// separates from itself (it is whatever survives to the end of flight).
struct StagePlan {
    std::vector<StageDefinition> stages;
};

struct StagingWarning {
    std::string code;  // UNSUPPORTED_STAGE_TRIGGER | UNSUPPORTED_IGNITION_TRIGGER | ...
    std::string stage_id;
    std::string motor_id;
    // The trigger value as received at this layer (already normalized by an
    // upstream importer, if any -- see Checkpoint 4's single normalization
    // boundary in ork_parser.py). Empty when the warning isn't trigger-related.
    std::string raw_trigger;
    // Set only when this layer itself performed a further normalization step
    // that is still unsupported (currently unused here; reserved for
    // importer-side warnings that choose to report through this same shape).
    std::string normalized_trigger;
    std::string detail;

    json toJson() const {
        json result = {
            {"code", code}, {"stage_id", stage_id}, {"motor_id", motor_id},
            {"raw_trigger", raw_trigger}, {"detail", detail}, {"message", detail},
        };
        if (!normalized_trigger.empty()) result["normalized_trigger"] = normalized_trigger;
        return result;
    }
};

inline std::vector<MotorInstance> parseMotorInstances(
    const json& input, std::vector<StagingWarning>& warnings) {
    std::vector<MotorInstance> result;
    if (!input.contains("motor_instances") || !input["motor_instances"].is_array()) {
        return result;
    }
    for (const auto& source : input["motor_instances"]) {
        if (!source.contains("time_s") || !source.contains("thrust_n")) {
            throw std::runtime_error(
                "motor_instances entries require time_s and thrust_n curves");
        }
        MotorInstance motor;
        motor.id = source.value("id", std::string());
        motor.stage_id = source.value("stage_id", std::string());
        motor.time_s = source["time_s"].get<std::vector<double>>();
        motor.thrust_n = source["thrust_n"].get<std::vector<double>>();
        if (motor.time_s.size() < 2 || motor.time_s.size() != motor.thrust_n.size()) {
            throw std::runtime_error("motor_instances curve dimensions are invalid");
        }
        motor.wet_mass_kg = source.value("wet_mass_kg", 0.0);
        motor.dry_mass_kg = source.value("dry_mass_kg", 0.0);
        motor.cg_z_m = source.value("cg_z_m", 0.0);
        motor.transverse_inertia_per_mass = source.value(
            "transverse_inertia_per_mass_kgm2_per_kg", 0.0);
        motor.ignition_delay_s = std::max(0.0, source.value("ignition_delay_s", 0.0));
        motor.ignition_event = source.value("ignition_event", std::string("launch"));
        motor.ignition_raw_trigger = source.value("ignition_raw_trigger", motor.ignition_event);
        motor.launch_stage = source.value("launch_stage", true);
        if (motor.wet_mass_kg < motor.dry_mass_kg || motor.dry_mass_kg < 0.0) {
            throw std::runtime_error("motor_instances masses are invalid");
        }
        if (!supportedIgnitionTriggerTypes().count(motor.ignition_event)) {
            warnings.push_back({
                "UNSUPPORTED_IGNITION_TRIGGER", motor.stage_id, motor.id,
                motor.ignition_raw_trigger,
                motor.ignition_raw_trigger != motor.ignition_event ? motor.ignition_event
                                                                    : std::string(),
                "Unsupported ignition_event '" + motor.ignition_event
                    + "' -- motor left un-ignited by this trigger.",
            });
        }
        result.push_back(std::move(motor));
    }
    return result;
}

inline StagePlan parseStagePlan(const json& input, std::vector<StagingWarning>& warnings) {
    StagePlan plan;
    if (!input.contains("stages") || !input["stages"].is_array()) {
        return plan;
    }
    for (const auto& source : input["stages"]) {
        StageDefinition stage;
        stage.id = source.value("id", std::string());
        if (source.contains("component_ids") && source["component_ids"].is_array()) {
            stage.component_ids = source["component_ids"].get<std::vector<std::string>>();
        }
        if (source.contains("motor_instance_ids") && source["motor_instance_ids"].is_array()) {
            stage.motor_instance_ids =
                source["motor_instance_ids"].get<std::vector<std::string>>();
        }
        stage.separation_type = source.value("separation_type", std::string("own_motor_burnout"));
        stage.separation_raw_value = source.value("separation_raw_trigger", stage.separation_type);
        stage.separation_delay_s = std::max(0.0, source.value("separation_delay_s", 0.0));
        if (!supportedSeparationTriggerTypes().count(stage.separation_type)) {
            warnings.push_back({
                "UNSUPPORTED_STAGE_TRIGGER", stage.id, "",
                stage.separation_raw_value,
                stage.separation_raw_value != stage.separation_type ? stage.separation_type
                                                                     : std::string(),
                "Unsupported separation_type '" + stage.separation_type
                    + "' -- stage will not separate on this trigger.",
            });
        }
        plan.stages.push_back(std::move(stage));
    }
    return plan;
}

// One precomputed "attached vehicle" dry/structural snapshot. Total runtime
// mass/CG/inertia remain functions of propellant depletion -- see
// roketsim_cli.cpp's getMass()/getCg()/getIxx(), which combine this snapshot
// with the active_motor_instance_ids' live propellant state.
struct VehiclePhase {
    std::vector<std::string> attached_stage_ids;
    // Canonical general baseline for this phase's static airframe (no
    // axisymmetry assumed) -- see runtime_mass_properties.hpp. base_mass/
    // base_first_moment/base_i_origin below are thin, derived read-outs of
    // this same accumulator (its total mass, its z-first-moment, and its
    // xx term), kept only so an existing direct reader of those three
    // scalar fields is unaffected.
    runtime_mass::OriginAccumulator static_origin;
    double base_mass = 0.0;
    double base_first_moment = 0.0;
    double base_i_origin = 0.0;
    double ref_dia = 0.0;
    double ref_area = 0.0;
    double cna = 0.0;
    double cp_z = 0.0;
    std::vector<double> aero_mach, cna_curve, cp_curve;
    barrowman_drag::Model drag;
    std::vector<std::string> active_motor_instance_ids;
};

// fullInput must already contain the whole rocket's "components" array (the
// same payload design::analyze() is normally called with); active component
// ids per phase are derived from the stage plan, not from the caller.
inline std::vector<VehiclePhase> buildVehiclePhases(
    const json& fullInput, const StagePlan& plan,
    const std::vector<MotorInstance>& motorInstances,
    const RoketSiM::AeroExtensions::AeroExtensionPipeline& extensionPipeline) {
    std::vector<VehiclePhase> phases;
    if (plan.stages.empty()) return phases;

    std::unordered_map<std::string, const MotorInstance*> motorsById;
    for (const auto& motor : motorInstances) motorsById[motor.id] = &motor;

    for (std::size_t i = 0; i < plan.stages.size(); ++i) {
        VehiclePhase phase;
        std::vector<std::string> activeComponentIds;
        for (std::size_t j = i; j < plan.stages.size(); ++j) {
            const StageDefinition& stage = plan.stages[j];
            phase.attached_stage_ids.push_back(stage.id);
            activeComponentIds.insert(activeComponentIds.end(),
                stage.component_ids.begin(), stage.component_ids.end());
            phase.active_motor_instance_ids.insert(phase.active_motor_instance_ids.end(),
                stage.motor_instance_ids.begin(), stage.motor_instance_ids.end());
        }

        json phaseInput = fullInput;
        phaseInput["active_component_ids"] = activeComponentIds;
        // effectiveReferenceDiameter() takes the MAX of a requested
        // reference_diameter_m and whatever the components imply. A
        // caller-supplied top-level value legitimately describes the WHOLE
        // launch vehicle (Python's effective_reference_diameter() always
        // computes it that way) -- left in place here, it would floor
        // every later phase at the full vehicle's diameter even after its
        // largest-diameter stage has separated, silently reintroducing a
        // dropped stage's geometry into the surviving phase's aero. Each
        // phase's reference geometry must come from only its own attached
        // components, matching OpenRocket's per-FlightConfiguration
        // reference-diameter convention (and this module's own
        // VehiclePhase contract) -- so it is dropped here and recomputed
        // from the filtered component set alone (falling back to
        // effectiveReferenceDiameter()'s own 0.06 m default only in the
        // edge case where the surviving phase has no component with an
        // explicit diameter at all, same as the whole-rocket path would).
        //
        // Set to a real 0.0, NOT erased: effectiveReferenceDiameter()
        // defaults an ABSENT reference_diameter_m to a hardcoded 0.06 m
        // (meant for a caller that supplied no geometry hint at all), which
        // would silently floor every phase at that unrelated constant
        // instead of letting its own components decide.
        phaseInput["reference_diameter_m"] = 0.0;
        phaseInput.erase("reference_diameter");
        const json analyzed = design::analyze(phaseInput, extensionPipeline);

        const double totalMass = analyzed["mass_kg"].get<double>();
        const auto& cgJson = analyzed["cg_m"];
        const auto& inertiaJson = analyzed["inertia_kgm2"];
        // design::analyze() includes every currently-attached motor's FULL
        // WET mass/CG/inertia in this phase's total (it has no concept of
        // "motor" vs. "airframe" -- see design_analysis.hpp). Subtract each
        // attached motor's own full-wet contribution back out, at its own
        // position, so static_origin is airframe-only; evaluateMassProperties()
        // then adds each motor's LIVE (dry + time-varying propellant)
        // contribution back on top every call. General: uses the analyzer's
        // full CG vector and full inertia tensor, not just cg_m[2]/[0][0].
        runtime_mass::OriginAccumulator origin;
        origin.add({totalMass, cgJson[0].get<double>(), cgJson[1].get<double>(),
                    cgJson[2].get<double>(),
                    inertiaJson[0][0].get<double>(), inertiaJson[1][1].get<double>(),
                    inertiaJson[2][2].get<double>(), inertiaJson[0][1].get<double>(),
                    inertiaJson[0][2].get<double>(), inertiaJson[1][2].get<double>()});
        for (const auto& motorId : phase.active_motor_instance_ids) {
            const auto found = motorsById.find(motorId);
            if (found == motorsById.end()) continue;
            const MotorInstance& motor = *found->second;
            const double transverse = motor.wet_mass_kg * motor.transverse_inertia_per_mass;
            origin.subtract({motor.wet_mass_kg, 0.0, 0.0, motor.cg_z_m,
                              transverse, transverse, 0.0, 0.0, 0.0, 0.0});
        }
        // Defensive clamp (matches the pre-refactor scalar code): a
        // pathological/synthetic design could declare less "airframe"
        // component mass than the motor(s) being subtracted back out. Only
        // the mass total is clamped, not the moment/inertia terms -- the
        // same asymmetry the original scalar code had (it clamped base_mass
        // alone), preserved rather than "fixed" here since this is a
        // degenerate-input guard, not a real physical case.
        origin.mass_kg = std::max(0.0, origin.mass_kg);
        phase.static_origin = origin;
        phase.base_mass = origin.mass_kg;
        phase.base_first_moment = origin.mz;
        phase.base_i_origin = origin.ixx;
        phase.ref_dia = analyzed["reference_diameter_m"].get<double>();
        phase.ref_area = 3.14159265358979323846 * std::pow(phase.ref_dia / 2.0, 2);
        phase.cna = analyzed["cna"].get<double>();
        phase.cp_z = analyzed["cp_m"].get<double>();
        if (analyzed.contains("mach_curve"))
            phase.aero_mach = analyzed["mach_curve"].get<std::vector<double>>();
        if (analyzed.contains("cna_curve"))
            phase.cna_curve = analyzed["cna_curve"].get<std::vector<double>>();
        if (analyzed.contains("cp_curve"))
            phase.cp_curve = analyzed["cp_curve"].get<std::vector<double>>();
        if (analyzed.contains("drag_model"))
            phase.drag = barrowman_drag::fromJson(analyzed["drag_model"]);

        phases.push_back(std::move(phase));
    }
    return phases;
}

inline std::vector<VehiclePhase> buildVehiclePhases(
    const json& fullInput, const StagePlan& plan,
    const std::vector<MotorInstance>& motorInstances) {
    static const RoketSiM::AeroExtensions::AeroExtensionPipeline noExtensions;
    return buildVehiclePhases(fullInput, plan, motorInstances, noExtensions);
}

inline json vehiclePhaseToJson(const VehiclePhase& phase) {
    return {
        {"attached_stage_ids", phase.attached_stage_ids},
        {"active_motor_instance_ids", phase.active_motor_instance_ids},
        {"base_mass_kg", phase.base_mass},
        {"base_first_moment_kgm", phase.base_first_moment},
        {"base_i_origin_kgm2", phase.base_i_origin},
        {"reference_diameter_m", phase.ref_dia},
        // Checkpoint 6: diagnostic-only field. The C++ flight loop already
        // reads phase.ref_area directly (see applyVehiclePhase()); this
        // just exposes the same already-computed value through the
        // --stage-plan diagnostic endpoint for parity comparison.
        {"reference_area_m2", phase.ref_area},
        {"cna", phase.cna},
        {"cp_m", phase.cp_z},
        // Checkpoint 6 diagnostic-only: the exact barrowman_drag::Model this
        // phase's flight-loop deriv() calls calculate() against (set via
        // applyVehiclePhase's c.drag = phase.drag), for direct comparison
        // against the same geometry's static --analyze drag_model output --
        // isolates whether the design::analyze() -> toJson() -> fromJson()
        // round trip that builds phase.drag (see buildVehiclePhases()) loses
        // or alters anything relative to the static analyzer's own model.
        {"drag_model", barrowman_drag::toJson(phase.drag)},
    };
}

// ── Checkpoint 2: deterministic event scheduling ───────────────────────
//
// Every Phase 1 trigger (launch[+delay], previous_motor_burnout[+delay],
// previous_stage_separation[+delay], own_motor_burnout[+delay]) resolves to
// an absolute time analytically, from data already known before flight:
// a motor's own thrust-curve duration, and the stages array's strictly
// sequential separation order (stage i's triggers may only reference stage
// i-1, never itself or a later stage). No root-finding is needed -- the
// flight loop just clamps its RK4 step so it never integrates across a
// resolved event time.
struct ResolvedEvent {
    enum Type { MotorIgnition, MotorBurnout, StageSeparation };
    double time_s = 0.0;
    Type type = MotorIgnition;
    std::string stage_id;
    std::string motor_id;
    // The order this event's own dependency was RESOLVED in (see
    // resolveEventSchedule()'s dependency-graph walk) -- monotonically
    // increasing across every motor-ignition/stage-separation node, in
    // dependency (topological) order, not JSON array order. A MOTOR_BURNOUT
    // event shares its own motor's ignition resolution_order (burnout is a
    // pure function of ignition + curve duration, not a separate graph
    // node). Same-timestamp events are ordered by this value at the flight
    // loop's processEventGroup() -- see roketsim_cli.cpp -- so that, for
    // example, an upper stage's MOTOR_IGNITION (which a lower stage's
    // next_stage_ignition separation depends ON) is always grouped/logged
    // before that separation, never after.
    int resolution_order = 0;
};

struct EventSchedule {
    // Sorted ascending by time_s. Events sharing a timestamp are ordered by
    // resolution_order at the flight loop's processEventGroup(), not by
    // this array's order.
    std::vector<ResolvedEvent> events;
};

inline double motorCurveDuration(const MotorInstance& motor) {
    if (motor.time_s.size() < 2) return 0.0;
    return std::max(0.0, motor.time_s.back() - motor.time_s.front());
}

// Resolves every motor's resolved_ignition_time_s in place and returns the
// sorted MOTOR_IGNITION/MOTOR_BURNOUT/STAGE_SEPARATION schedule.
//
// Checkpoint 4.5: a stage's separation can now depend FORWARD on the next
// stage's ignition (next_stage_ignition / OpenRocket's "upperignition"), not
// only backward on its own or the previous stage's events, so this resolves
// each motor-ignition/stage-separation node as an explicit memoized
// recursive dependency graph (DFS with in-progress marking for cycle
// detection) rather than one forward pass over stage index. Any
// ignition/separation trigger already flagged unsupported by
// parseMotorInstances()/parseStagePlan() resolves to +infinity ("never") --
// consistent with the UNSUPPORTED_*_TRIGGER warning text ("left
// un-ignited"/"will not separate"), not silently reinterpreted. A genuine
// circular dependency (only constructible by hand -- real OpenRocket UI
// cannot express one) resolves every node in the cycle to +infinity and
// emits one CYCLIC_STAGING_DEPENDENCY warning rather than looping forever.
inline EventSchedule resolveEventSchedule(
    const StagePlan& plan, std::vector<MotorInstance>& motorInstances,
    std::vector<StagingWarning>& warnings) {
    EventSchedule schedule;
    constexpr double kInfinity = std::numeric_limits<double>::infinity();

    std::unordered_map<std::string, std::vector<std::size_t>> motorsByStage;
    for (std::size_t i = 0; i < motorInstances.size(); ++i) {
        motorsByStage[motorInstances[i].stage_id].push_back(i);
    }

    if (plan.stages.empty()) {
        // No stage plan: every motor is a "launch" motor, regardless of its
        // ignition_event string -- the only semantics reachable before a
        // StagePlan existed at all (Checkpoint 1's non-staged
        // motor_instances payload, and every legacy payload).
        int order = 0;
        for (auto& motor : motorInstances) {
            motor.resolved_ignition_time_s = motor.ignition_delay_s;
            schedule.events.push_back({
                motor.resolved_ignition_time_s, ResolvedEvent::MotorIgnition,
                motor.stage_id, motor.id, order});
            schedule.events.push_back({
                motor.resolved_ignition_time_s + motorCurveDuration(motor),
                ResolvedEvent::MotorBurnout, motor.stage_id, motor.id, order});
            ++order;
        }
        std::sort(schedule.events.begin(), schedule.events.end(),
            [](const ResolvedEvent& a, const ResolvedEvent& b) { return a.time_s < b.time_s; });
        return schedule;
    }

    std::unordered_map<std::string, std::size_t> stageIndexById;
    for (std::size_t i = 0; i < plan.stages.size(); ++i) stageIndexById[plan.stages[i].id] = i;

    enum class NodeState { Unvisited, InProgress, Done };
    std::vector<NodeState> motorState(motorInstances.size(), NodeState::Unvisited);
    std::vector<double> motorIgnitionTime(motorInstances.size(), kInfinity);
    std::vector<NodeState> stageState(plan.stages.size(), NodeState::Unvisited);
    std::vector<double> stageSeparationTime(plan.stages.size(), kInfinity);
    int resolutionCounter = 0;
    bool cycleWarned = false;

    auto warnCycle = [&](const std::string& stageId, const std::string& motorId) {
        if (cycleWarned) return;
        cycleWarned = true;
        warnings.push_back({
            "CYCLIC_STAGING_DEPENDENCY", stageId, motorId, "", "",
            "A circular dependency was detected while resolving the staging "
            "event schedule (e.g. a stage's separation and another stage's "
            "ignition each waiting on the other). Every event in the cycle "
            "resolves to 'never' rather than looping or guessing a time.",
        });
    };

    std::function<double(std::size_t)> resolveMotorIgnition;
    std::function<double(std::size_t)> resolveStageSeparation;

    resolveMotorIgnition = [&](std::size_t motorIndex) -> double {
        if (motorState[motorIndex] == NodeState::Done) return motorIgnitionTime[motorIndex];
        MotorInstance& motor = motorInstances[motorIndex];
        if (motorState[motorIndex] == NodeState::InProgress) {
            warnCycle(motor.stage_id, motor.id);
            return kInfinity;
        }
        motorState[motorIndex] = NodeState::InProgress;
        const auto stageIt = stageIndexById.find(motor.stage_id);
        const bool hasKnownStage = stageIt != stageIndexById.end();
        const std::size_t stageIdx = hasKnownStage ? stageIt->second : 0;
        double ignitionTime = kInfinity;
        if (motor.ignition_event == "launch") {
            ignitionTime = motor.ignition_delay_s;
        } else if (motor.ignition_event == "previous_motor_burnout"
            || motor.ignition_event == "previous_stage_separation") {
            if (!hasKnownStage || stageIdx == 0) {
                warnings.push_back({
                    "UNSUPPORTED_IGNITION_TRIGGER", motor.stage_id, motor.id,
                    motor.ignition_raw_trigger,
                    motor.ignition_raw_trigger != motor.ignition_event ? motor.ignition_event
                                                                        : std::string(),
                    "ignition_event '" + motor.ignition_event
                        + "' requested for the first stage, which has no"
                          " previous stage -- motor left un-ignited.",
                });
            } else if (motor.ignition_event == "previous_motor_burnout") {
                double burnout = -kInfinity;
                bool any = false;
                for (const std::size_t idx : motorsByStage[plan.stages[stageIdx - 1].id]) {
                    const double ignition = resolveMotorIgnition(idx);
                    if (std::isfinite(ignition)) {
                        burnout = std::max(
                            burnout, ignition + motorCurveDuration(motorInstances[idx]));
                        any = true;
                    }
                }
                if (any) ignitionTime = burnout + motor.ignition_delay_s;
            } else {
                const double separation = resolveStageSeparation(stageIdx - 1);
                if (std::isfinite(separation)) ignitionTime = separation + motor.ignition_delay_s;
            }
        }
        // Else: already flagged UNSUPPORTED_IGNITION_TRIGGER by
        // parseMotorInstances(); stays at +infinity ("never ignites").
        motor.resolved_ignition_time_s = ignitionTime;
        motorIgnitionTime[motorIndex] = ignitionTime;
        motorState[motorIndex] = NodeState::Done;
        const int order = resolutionCounter++;
        if (std::isfinite(ignitionTime)) {
            schedule.events.push_back({
                ignitionTime, ResolvedEvent::MotorIgnition, motor.stage_id, motor.id, order});
            schedule.events.push_back({
                ignitionTime + motorCurveDuration(motor), ResolvedEvent::MotorBurnout,
                motor.stage_id, motor.id, order});
        }
        return ignitionTime;
    };

    resolveStageSeparation = [&](std::size_t stageIndex) -> double {
        if (stageState[stageIndex] == NodeState::Done) return stageSeparationTime[stageIndex];
        const StageDefinition& stage = plan.stages[stageIndex];
        if (stageState[stageIndex] == NodeState::InProgress) {
            warnCycle(stage.id, "");
            return kInfinity;
        }
        stageState[stageIndex] = NodeState::InProgress;
        double separationTime = kInfinity;
        if (stage.separation_type == "own_motor_burnout") {
            double burnout = -kInfinity;
            bool any = false;
            for (const std::size_t idx : motorsByStage[stage.id]) {
                const double ignition = resolveMotorIgnition(idx);
                if (std::isfinite(ignition)) {
                    burnout = std::max(burnout, ignition + motorCurveDuration(motorInstances[idx]));
                    any = true;
                }
            }
            // A stage with own_motor_burnout but literally no motor of its
            // own separates immediately (matches Checkpoint 1-4's behavior
            // for this degenerate/edge-case input): there is nothing to burn
            // out, so there is nothing to wait for.
            separationTime = (any ? burnout : 0.0) + stage.separation_delay_s;
        } else if (stage.separation_type == "next_stage_ignition") {
            if (stageIndex + 1 < plan.stages.size()) {
                double earliestUpperIgnition = kInfinity;
                bool any = false;
                for (const std::size_t idx : motorsByStage[plan.stages[stageIndex + 1].id]) {
                    const double ignition = resolveMotorIgnition(idx);
                    if (std::isfinite(ignition)) {
                        earliestUpperIgnition = std::min(earliestUpperIgnition, ignition);
                        any = true;
                    }
                }
                if (any) separationTime = earliestUpperIgnition + stage.separation_delay_s;
                // Else: the next stage has no resolvable motor of its own to
                // trigger this separation -- stays at +infinity, same
                // "unresolvable dependency" treatment as any other trigger
                // that cannot be evaluated.
            }
        }
        // Else: already flagged UNSUPPORTED_STAGE_TRIGGER by parseStagePlan();
        // stays at +infinity ("never separates").
        stageSeparationTime[stageIndex] = separationTime;
        stageState[stageIndex] = NodeState::Done;
        const int order = resolutionCounter++;
        if (std::isfinite(separationTime)) {
            schedule.events.push_back({
                separationTime, ResolvedEvent::StageSeparation, stage.id, "", order});
        }
        return separationTime;
    };

    // Same overall stage-index traversal shape as before Checkpoint 4.5 (so
    // every previously-supported combination keeps its exact resolution
    // order/timing) -- own_motor_burnout and previous_*-triggered ignitions
    // never need to look beyond what's already resolved by this point.
    // next_stage_ignition is the only trigger that reaches forward past this
    // sequential walk, via the recursive resolveMotorIgnition() call inside
    // resolveStageSeparation() above.
    for (std::size_t i = 0; i < plan.stages.size(); ++i) {
        for (const std::size_t motorIndex : motorsByStage[plan.stages[i].id]) {
            resolveMotorIgnition(motorIndex);
        }
        if (i + 1 < plan.stages.size()) resolveStageSeparation(i);
    }

    std::sort(schedule.events.begin(), schedule.events.end(),
        [](const ResolvedEvent& a, const ResolvedEvent& b) { return a.time_s < b.time_s; });
    return schedule;
}

}  // namespace staging
