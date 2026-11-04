#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace FinPlanform {

struct Point {
    double x = 0.0;
    double y = 0.0;
};

struct Metrics {
    bool valid = false;
    double area = 0.0;
    double span = 0.0;
    double axialLength = 0.0;
    double centroidX = 0.0;
    double centroidY = 0.0;
    double macLength = 0.0;
    double macLeadingEdge = 0.0;
    double quarterChordCp = 0.0;
    double leadingEdgeCosine = 1.0;
    double midChordCosine = 1.0;
    std::vector<double> stripY;
    std::vector<double> stripChord;
};

inline Metrics calculate(
    std::vector<Point> points,
    int integrationSlices = 48
) {
    Metrics result;
    if (points.size() < 3) {
        return result;
    }

    std::vector<Point> cleaned;
    cleaned.reserve(points.size());
    for (const Point& point : points) {
        if (!std::isfinite(point.x) || !std::isfinite(point.y)) {
            continue;
        }
        if (cleaned.empty()
            || std::abs(point.x - cleaned.back().x) > 1.0e-12
            || std::abs(point.y - cleaned.back().y) > 1.0e-12) {
            cleaned.push_back(point);
        }
    }
    if (cleaned.size() > 2
        && std::abs(cleaned.front().x - cleaned.back().x) <= 1.0e-12
        && std::abs(cleaned.front().y - cleaned.back().y) <= 1.0e-12) {
        cleaned.pop_back();
    }
    if (cleaned.size() < 3) {
        return result;
    }

    const auto minX = std::min_element(
        cleaned.begin(), cleaned.end(),
        [](const Point& a, const Point& b) { return a.x < b.x; })->x;
    const auto minY = std::min_element(
        cleaned.begin(), cleaned.end(),
        [](const Point& a, const Point& b) { return a.y < b.y; })->y;
    for (Point& point : cleaned) {
        point.x -= minX;
        point.y -= minY;
        result.axialLength = std::max(result.axialLength, point.x);
        result.span = std::max(result.span, point.y);
    }
    if (result.span <= 1.0e-12 || result.axialLength <= 1.0e-12) {
        return Metrics{};
    }

    double twiceArea = 0.0;
    double centroidXNumerator = 0.0;
    double centroidYNumerator = 0.0;
    for (std::size_t index = 0; index < cleaned.size(); ++index) {
        const Point& first = cleaned[index];
        const Point& second = cleaned[(index + 1) % cleaned.size()];
        const double cross = first.x * second.y - second.x * first.y;
        twiceArea += cross;
        centroidXNumerator += (first.x + second.x) * cross;
        centroidYNumerator += (first.y + second.y) * cross;
    }
    if (std::abs(twiceArea) <= 1.0e-14) {
        return Metrics{};
    }
    if (twiceArea < 0.0) {
        std::reverse(cleaned.begin(), cleaned.end());
        twiceArea = -twiceArea;
        centroidXNumerator = -centroidXNumerator;
        centroidYNumerator = -centroidYNumerator;
    }
    result.area = 0.5 * twiceArea;
    result.centroidX = centroidXNumerator / (3.0 * twiceArea);
    result.centroidY = centroidYNumerator / (3.0 * twiceArea);

    // This is deliberately the same 48-station chord construction used by
    // OpenRocket's FinSetCalc.  The physical polygon area/centroid above are
    // exact mass properties; MAC and sweep are aerodynamic discretizations.
    // Mixing midpoint-strip geometry with FinSetCalc's endpoint stations was
    // enough to shift FreeformFinSet CNa at M=0.3.
    const int stations = std::max(integrationSlices, 2);
    const double dy = result.span / static_cast<double>(stations - 1);
    std::vector<double> chordLead(stations, std::numeric_limits<double>::infinity());
    std::vector<double> chordTrail(stations, -std::numeric_limits<double>::infinity());
    std::vector<double> chordLength(stations, 0.0);
    for (std::size_t index = 0; index < cleaned.size(); ++index) {
        const Point& first = cleaned[index];
        const Point& second = cleaned[(index + 1) % cleaned.size()];
        if (std::abs(first.y - second.y) <= 0.001) continue;
        int firstStation = static_cast<int>(first.y * 1.0001 / result.span * (stations - 1));
        int secondStation = static_cast<int>(second.y * 1.0001 / result.span * (stations - 1));
        firstStation = std::clamp(firstStation, 0, stations - 1);
        secondStation = std::clamp(secondStation, 0, stations - 1);
        if (firstStation > secondStation) std::swap(firstStation, secondStation);
        for (int station = firstStation; station <= secondStation; ++station) {
            const double y = station * dy;
            const double x = std::clamp(
                (y - second.y) / (first.y - second.y) * first.x
                    + (first.y - y) / (first.y - second.y) * second.x,
                std::min(first.x, second.x), std::max(first.x, second.x));
            chordLead[station] = std::min(chordLead[station], x);
            chordTrail[station] = std::max(chordTrail[station], x);
            chordLength[station] += first.y < second.y ? -x : x;
        }
    }
    for (int station = 0; station < stations; ++station) {
        if (!std::isfinite(chordLead[station]) || !std::isfinite(chordTrail[station])) {
            chordLead[station] = chordTrail[station] = chordLength[station] = 0.0;
        }
        chordLength[station] = std::clamp(chordLength[station], 0.0,
                                           chordTrail[station] - chordLead[station]);
        result.stripY.push_back(station * dy);
        result.stripChord.push_back(chordLength[station]);
    }
    double sampledArea = 0.0;
    double cosGamma = 0.0;
    double cosGammaLead = 0.0;
    for (int station = 0; station < stations; ++station) {
        const double chord = chordTrail[station] - chordLead[station];
        const double y = station * dy;
        result.macLength += chord * chord;
        result.macLeadingEdge += chordLead[station] * chord;
        sampledArea += chord;
        if (station > 0) {
            const double midDelta = (chordTrail[station] + chordLead[station]
                - chordTrail[station - 1] - chordLead[station - 1]) / 2.0;
            const double leadingDelta = chordLead[station] - chordLead[station - 1];
            cosGamma += dy / std::hypot(midDelta, dy);
            cosGammaLead += dy / std::hypot(leadingDelta, dy);
        }
    }
    sampledArea *= dy;
    result.macLength *= dy;
    result.macLeadingEdge *= dy;
    if (sampledArea <= 1.0e-14) return Metrics{};
    result.macLength /= sampledArea;
    result.macLeadingEdge /= sampledArea;
    result.quarterChordCp = result.macLeadingEdge + 0.25 * result.macLength;
    result.midChordCosine = cosGamma / static_cast<double>(stations - 1);
    result.leadingEdgeCosine = cosGammaLead / static_cast<double>(stations - 1);
    result.valid = result.area > 1.0e-14
        && result.macLength > 1.0e-14;
    return result;
}

}  // namespace FinPlanform
