#pragma once

#include <Eigen/Dense>
#include <Eigen/Geometry>
#include <memory>
#include <string>
#include <vector>

#include "core/Transform.hpp"
#include "mass/MassProperties.hpp"
#include "aero/AeroProperties.hpp"
#include "aero/DragBreakdown.hpp"
#include "aero/FlightCondition.hpp"
#include "components/Overrides.hpp"
#include "components/ComponentCategory.hpp"
#include "components/AxialPosition.hpp"
#include "components/RadialPosition.hpp"

class Component {
public:
    Transform localTransform;
    Component* parent = nullptr;
    Overrides overrides;

    virtual ~Component() = default;

    const std::string& name() const noexcept {
        return name_;
    }

    void setAxialPosition(AxialPosition placement) noexcept {
        axialPosition_ = placement;
    }

    const AxialPosition& axialPosition() const noexcept {
        return axialPosition_;
    }

    void setRadialPosition(
        RadialPosition placement,
        double parentOuterRadius = 0.0
    ) {
        const double radius = placement.resolvedRadius(parentOuterRadius);
        radialPosition_ = placement;
        localTransform.position.x() = radius * std::cos(placement.angleRad);
        localTransform.position.y() = radius * std::sin(placement.angleRad);
        localTransform.rotation = Eigen::AngleAxisd(
            placement.angleRad, Eigen::Vector3d::UnitZ()).toRotationMatrix();
    }

    const RadialPosition& radialPosition() const noexcept {
        return radialPosition_;
    }

    void addChild(std::unique_ptr<Component> child) {
        if (!child) {
            throw std::invalid_argument("Child component must not be null.");
        }
        if (!isCompatible(*child)) {
            throw IncompatibleComponentError(name(), child->name());
        }
        child->parent = this;
        children_.push_back(std::move(child));
    }

    virtual ComponentTrait traits() const noexcept = 0;

    bool hasAnyTrait(ComponentTrait query) const noexcept {
        return ::hasTrait(traits(), query);
    }

    // Components are leaves until a concrete parent opts in explicitly.
    virtual bool isCompatible(const Component&) const noexcept {
        return false;
    }

    const std::vector<std::unique_ptr<Component>>& children() const noexcept {
        return children_;
    }

    Transform globalTransform() const {
        if (parent == nullptr) {
            return localTransform;
        }

        Transform p = parent->globalTransform();
        Transform g;
        g.rotation = p.rotation * localTransform.rotation;
        g.position = p.position + p.rotation * localTransform.position;
        return g;
    }

    Eigen::Vector3d globalPosition() const {
        return globalTransform().position;
    }

    virtual double axialLength() const = 0;
    
    // Returns the outer radius for fins or other attachments to use.
    // Defaults to 0.0 unless overridden by BodyTube, NoseCone, etc.
    virtual double getReferenceOuterRadius() const {
        return 0.0;
    }

    MassProperties massProperties() const {
        MassProperties mp = rawMassProperties();
        if (overrides.massEnabled) {
            mp.mass = overrides.mass;
        }
        if (overrides.cgEnabled) {
            mp.cgLocal.z() = overrides.cgLocalZ;
        }
        return mp;
    }

    AeroProperties aeroProperties(const FlightCondition& fc) const {
        AeroProperties ap = rawAeroProperties(fc);
        if (overrides.cpEnabled) {
            ap.cp = overrides.cpAbsolute;
        }
        if (overrides.cnaEnabled) {
            ap.cna = overrides.cna;
        }
        if (overrides.cdEnabled) {
            ap.cd = overrides.cd;
        }
        return ap;
    }

    virtual DragBreakdown dragBreakdown(const FlightCondition&) const {
        return {};
    }

protected:
    std::string name_;
    std::vector<std::unique_ptr<Component>> children_;
    AxialPosition axialPosition_;
    RadialPosition radialPosition_;
    virtual MassProperties rawMassProperties() const = 0;

    // Default: component has no aerodynamic contribution.
    virtual AeroProperties rawAeroProperties(const FlightCondition&) const {
        return {};
    }
};
