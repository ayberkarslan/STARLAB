#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

enum class ComponentCategory : std::uint32_t {
    None              = 0,
    BodyComponent     = 1u << 0,
    InternalComponent = 1u << 1,
    ExternalComponent = 1u << 2,
    ComponentAssembly = 1u << 3,
    LeafComponent     = 1u << 4,
    AxialStage        = 1u << 5,
    ParallelStage     = 1u << 6,
    PodSet            = 1u << 7,
    FreeformFinSet    = 1u << 8,
    Motor             = 1u << 9,
    Rocket            = 1u << 10,
    MotorMountCapable = 1u << 11
};

using ComponentTrait = ComponentCategory;

constexpr ComponentCategory operator|(ComponentCategory lhs, ComponentCategory rhs) noexcept {
    return static_cast<ComponentCategory>(
        static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs));
}

constexpr ComponentCategory operator&(ComponentCategory lhs, ComponentCategory rhs) noexcept {
    return static_cast<ComponentCategory>(
        static_cast<std::uint32_t>(lhs) & static_cast<std::uint32_t>(rhs));
}

constexpr bool hasAnyCategory(ComponentCategory value, ComponentCategory query) noexcept {
    return static_cast<std::uint32_t>(value & query) != 0u;
}

constexpr bool hasTrait(ComponentTrait value, ComponentTrait query) noexcept {
    return hasAnyCategory(value, query);
}

class IncompatibleComponentError : public std::invalid_argument {
public:
    IncompatibleComponentError(const std::string& parentName, const std::string& childName)
        : std::invalid_argument(
              "Component '" + childName + "' is not compatible with parent '" + parentName + "'.") {}
};
