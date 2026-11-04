#pragma once

// Faithful port of the x86/x87 instruction interpreter that executes the
// recovered Van Dyke Hybrid / SOSE pressure program, read directly from:
//   third_party/datcom_equivalent_oracle/datcom_equivalent/body/_hybrid_pressure_math.py
//     (class HybridPressureMachine, class X87, module-level helpers)
//
// This is NOT a hand-derived Van Dyke Hybrid implementation. The oracle's
// own "algorithm" for this owner is: interpret the disassembly-recovered
// operation ledger in HybridProgramData.hpp/.cpp exactly as the original
// binary executed it. SOSE is not a separate machine or program -- it is
// entry point 0x004F9E80 into the SAME operation ledger, reached when the
// Hybrid entry point (0x004A6520) signals failure. Porting a "textbook"
// algorithm instead of this interpreter would not be a port of the
// oracle's specification.
//
// Deliberately NOT ported (pure introspection in the Python source, never
// read by any caller that consumes the numeric result): the `helper_entries`
// / `function_entries` diagnostic capture at two specific program counters,
// `read_before_write`, and `nan_stores` bookkeeping. Omitting these changes
// no register, memory, or x87 state and therefore no numeric output.

#include <array>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "datcom_equivalent/body/HybridProgramData.hpp"

namespace datcom_equivalent::body {

class HybridPressureMachine;
using LedgerExternalCall = bool (*)(HybridPressureMachine&, std::uint32_t);

class HybridPressureMachineError : public std::runtime_error {
public:
    explicit HybridPressureMachineError(const std::string& what) : std::runtime_error(what) {}
};

class HybridPressureMachine {
public:
    // Hybrid/SOSE and low/transonic BODY-wave use the same recovered x86/x87
    // interpreter with separate generated operation ledgers.
    struct OperationLedger {
        const std::vector<std::pair<std::uint32_t, std::uint32_t>>& functions;
        const std::vector<std::pair<std::uint32_t, std::uint32_t>>& initialDwords;
        const std::vector<std::pair<std::uint32_t, std::uint32_t>>& tableDwords;
        const std::vector<hybrid_program::Operation>& operations;
        const std::vector<std::pair<std::uint32_t, std::uint32_t>>& thunks;
        const std::vector<std::pair<std::uint32_t, hybrid_program::MathIntrinsic>>& mathIntrinsics;
        LedgerExternalCall externalCall = nullptr;
    };
    struct Result {
        std::unordered_map<std::uint32_t, std::uint32_t> memory;
        std::size_t instructionCount = 0;

        double f32(std::uint32_t address) const;
    };


    // `extraMemory` overlays (and can override) the program's fixed initial
    // dwords + invariant table dwords -- mirrors
    // HybridPressureMachine.__init__(memory=...) in the oracle.
    explicit HybridPressureMachine(
        std::unordered_map<std::uint32_t, std::uint32_t> extraMemory = {},
        const OperationLedger* ledger = nullptr);

    // Mirrors HybridPressureMachine.run(entry, arguments, limit). Pushes a
    // cdecl-style argument list (reversed, then a zero return address) and
    // interprets from `entry` until a `ret` pops that zero return address
    // (pc becomes 0), an unsupported operation is hit, or `limit`
    // instructions have executed (matching the oracle's own runaway guard).
    Result run(std::uint32_t entry, const std::vector<std::uint32_t>& arguments,
               std::size_t limit = 20'000'000);

    // Recovered external helpers receive pointer arguments on the live cdecl
    // stack.  These narrow accessors let a ledger-specific handler consume
    // only those REAL*4 operands and write only its documented return slot.
    double indirectStackF32(std::size_t argumentIndex);
    void writeIndirectStackF32(std::size_t argumentIndex, double value);

private:
    struct X87 {
        double values[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        bool valid[8] = {false, false, false, false, false, false, false, false};
        int top = 0;

        int ix(int relative = 0) const { return (top + relative) & 7; }
        double get(int relative = 0) const { return values[ix(relative)]; }
        void set(double value, int relative = 0) {
            int i = ix(relative);
            values[i] = value;
            valid[i] = true;
        }
        void push(double value) {
            top = (top - 1) & 7;
            set(value);
        }
        void pop() {
            valid[top] = false;
            top = (top + 1) & 7;
        }
        void storePop(int relative) {
            double value = get();
            int destination = ix(relative);
            values[destination] = value;
            valid[destination] = true;
            valid[top] = false;
            top = (top + 1) & 7;
        }
    };

    std::unordered_map<std::uint32_t, std::uint32_t> memory_;
    std::unordered_map<std::string, std::uint32_t> reg_;
    X87 x87_;
    bool zf_ = false, sf_ = false, of_ = false, cf_ = false;
    std::uint32_t currentPc_ = 0;
    std::uint32_t x87ControlWord_ = 0x037F;

    // The oracle rebuilds its equivalent of these two maps in every
    // HybridPressureMachine.__init__ call. That rebuild is a pure,
    // deterministic function of the constant generated program data
    // (kOperations/kFunctions) with no per-instance state, so this port
    // builds it once (function-local static, see .cpp) and every instance
    // shares a reference -- a performance-only change (the oracle's own
    // Python interpreter is far slower per-instruction regardless; this
    // just avoids ~700K redundant map-construction operations per
    // machine instance in the C++ port, which otherwise dominated runtime
    // for a several-hundred-case parity run). No numeric behavior differs.
    const std::unordered_map<std::uint32_t, std::pair<std::string, std::string>>& operations_;
    const std::unordered_map<std::uint32_t, std::uint32_t>& nextAddress_;
    const std::vector<std::pair<std::uint32_t, std::uint32_t>>& thunks_;
    const std::vector<std::pair<std::uint32_t, hybrid_program::MathIntrinsic>>& mathIntrinsics_;
    LedgerExternalCall externalCall_ = nullptr;

    std::uint32_t readDword(std::uint32_t address);
    void writeDword(std::uint32_t address, std::uint32_t value);

    std::uint32_t getReg(const std::string& name) const;
    void setReg(const std::string& name, std::uint32_t value);

    std::uint32_t addressExpr(const std::string& expression) const;
    std::uint32_t memoryAddress(const std::string& operand) const;
    std::uint32_t readInt(const std::string& operand);
    void writeInt(const std::string& operand, std::uint32_t value);
    double readFloat(const std::string& operand);

    void push(std::uint32_t value);
    std::uint32_t pop();

    void setSubFlags(std::uint32_t left, std::uint32_t right, std::uint32_t result);
    void setLogicFlags(std::uint32_t result);
};

}  // namespace datcom_equivalent::body
