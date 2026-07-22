#pragma once

#include <string>
#include <random>
#include <cmath>
#include <algorithm>
#include "InputDevice.h"

struct HumanizationConfig {
    bool   enabled        = true;
    int    jitterPx       = 2;
    double delayVariance  = 0.25;
    int    moveDurationMs = 120;
    int    moveSteps      = 20;
    int    curveBowPx     = 15;
};

inline HumanizationConfig HumanizationPreset(const std::string& name) {
    if (name == "subtle") return { true, 1, 0.10, 60,  10, 6  };
    if (name == "human")  return { true, 3, 0.35, 180, 25, 25 };
    return { false, 0, 0.0, 0, 1, 0 }; // "off"
}

class Humanizer : public IMouseInput, public IKeyboardInput {
public:
    Humanizer(IMouseInput* mouse, IKeyboardInput* keyboard, HumanizationConfig config)
        : mouse_(mouse), keyboard_(keyboard), config_(config), rng_(std::random_device{}()) {}

    void LeftClick() override { JitterInPlace(); mouse_->LeftClick(); }
    void RightClick() override { JitterInPlace(); mouse_->RightClick(); }

    void MoveTo(int x, int y) override {
        if (!config_.enabled) { mouse_->MoveTo(x, y); return; }

        int fromX, fromY;
        mouse_->GetPosition(fromX, fromY);
        GlideAlongCurve(fromX, fromY, x + JitterAmount(), y + JitterAmount());
    }

    void GetPosition(int& x, int& y) override { mouse_->GetPosition(x, y); }

    void Wait(int ms) override { mouse_->Wait(VariableDelay(ms)); }

    void PressChar(char c) override { keyboard_->PressChar(c); }

    bool AbortRequested() override { return keyboard_->AbortRequested(); }

private:
    IMouseInput*        mouse_;
    IKeyboardInput*     keyboard_;
    HumanizationConfig  config_;
    std::mt19937        rng_;

    int RandRange(int lo, int hi) {
        std::uniform_int_distribution<int> d(lo, hi);
        return d(rng_);
    }

    int JitterAmount() {
        if (!config_.enabled || config_.jitterPx <= 0) return 0;
        return RandRange(-config_.jitterPx, config_.jitterPx);
    }

    int VariableDelay(int ms) {
        if (!config_.enabled || config_.delayVariance <= 0.0) return ms;
        int span = static_cast<int>(ms * config_.delayVariance);
        if (span <= 0) return ms;
        return (std::max)(0, ms + RandRange(-span, span));
    }

    // Distinct from GlideAlongCurve: a near-instant tremor right before a click,
    // not deliberate travel toward a distant point.
    void JitterInPlace() {
        if (!config_.enabled || config_.jitterPx <= 0) return;
        int x, y;
        mouse_->GetPosition(x, y);
        mouse_->MoveTo(x + JitterAmount(), y + JitterAmount());
    }

    void GlideAlongCurve(int x0, int y0, int x3, int y3) {
        double dx = x3 - x0;
        double dy = y3 - y0;
        double len = std::sqrt(dx * dx + dy * dy);
        double nx = (len > 0.001) ? -dy / len : 0.0; // perpendicular unit vector,
        double ny = (len > 0.001) ?  dx / len : 0.0; // bows the control points off the line

        double bow1 = RandRange(-config_.curveBowPx, config_.curveBowPx);
        double bow2 = RandRange(-config_.curveBowPx, config_.curveBowPx);

        double p1x = x0 + dx * 0.33 + nx * bow1, p1y = y0 + dy * 0.33 + ny * bow1;
        double p2x = x0 + dx * 0.66 + nx * bow2, p2y = y0 + dy * 0.66 + ny * bow2;

        int steps = (std::max)(1, config_.moveSteps);
        int perStepDelay = config_.moveDurationMs / steps;

        for (int i = 1; i <= steps; i++) {
            double t = static_cast<double>(i) / steps;
            double e = t * t * (3.0 - 2.0 * t); // smoothstep easing

            double omt = 1.0 - e;
            double px = omt * omt * omt * x0 + 3 * omt * omt * e * p1x
                      + 3 * omt * e * e * p2x + e * e * e * x3;
            double py = omt * omt * omt * y0 + 3 * omt * omt * e * p1y
                      + 3 * omt * e * e * p2y + e * e * e * y3;

            mouse_->MoveTo(static_cast<int>(std::lround(px)), static_cast<int>(std::lround(py)));
            mouse_->Wait(VariableDelay(perStepDelay));
        }
    }
};
