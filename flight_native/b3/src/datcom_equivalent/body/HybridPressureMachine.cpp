#include "datcom_equivalent/body/HybridPressureMachine.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <limits>
#include <sstream>

#include "datcom_equivalent/body/HybridProgramData.hpp"

namespace datcom_equivalent::body {

namespace {

using hybrid_program::kFunctions;
using hybrid_program::kInitialDwords;
using hybrid_program::kMathIntrinsics;
using hybrid_program::kOperations;
using hybrid_program::kTableDwords;
using hybrid_program::kThunks;
using hybrid_program::MathIntrinsic;

constexpr std::uint32_t kMask = 0xFFFFFFFFu;

std::string trim(const std::string& s) {
    std::size_t begin = s.find_first_not_of(" \t");
    if (begin == std::string::npos) return "";
    std::size_t end = s.find_last_not_of(" \t");
    return s.substr(begin, end - begin + 1);
}

std::string toLower(std::string s) {
    for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

std::vector<std::string> splitChar(const std::string& s, char delim) {
    std::vector<std::string> parts;
    std::string current;
    for (char c : s) {
        if (c == delim) {
            parts.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    parts.push_back(current);
    return parts;
}

// Mirrors Python's int(text, 0): "0x.."/"0X.." -> hex, otherwise decimal;
// optional leading '-'.
std::int64_t parseIntAuto(const std::string& textIn) {
    std::string text = trim(textIn);
    bool negative = false;
    if (!text.empty() && text[0] == '-') {
        negative = true;
        text = trim(text.substr(1));
    }
    std::int64_t value = 0;
    if (text.size() > 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
        value = std::stoll(text.substr(2), nullptr, 16);
    } else {
        value = std::stoll(text, nullptr, 10);
    }
    return negative ? -value : value;
}

bool isFullRegisterName(const std::string& name) {
    return name == "eax" || name == "ebx" || name == "ecx" || name == "edx" || name == "esi" ||
           name == "edi" || name == "ebp" || name == "esp";
}

struct MemoryOperand {
    bool isMemory = false;
    std::string sizePrefix;  // "", "byte", "word", "dword", "qword"
    std::string expr;
};

// Mirrors MEMORY = re.compile(r"^(?:(byte|word|dword|qword) ptr )?\[(.+)\]$")
// applied to an already trim+lower operand string.
MemoryOperand parseMemoryOperand(const std::string& operand) {
    MemoryOperand result;
    if (operand.empty() || operand.back() != ']') return result;
    const std::size_t open = operand.find('[');
    if (open == std::string::npos) return result;
    const std::string beforeBracket = trim(operand.substr(0, open));
    const std::string inside = operand.substr(open + 1, operand.size() - open - 2);
    if (!beforeBracket.empty()) {
        const std::size_t sp = beforeBracket.find(' ');
        if (sp == std::string::npos) return result;
        const std::string size = beforeBracket.substr(0, sp);
        const std::string rest = trim(beforeBracket.substr(sp + 1));
        if (rest != "ptr") return result;
        if (size != "byte" && size != "word" && size != "dword" && size != "qword") return result;
        result.sizePrefix = size;
    }
    result.isMemory = true;
    result.expr = inside;
    return result;
}

bool isStOperand(const std::string& operand, int& index) {
    if (operand.size() < 4) return false;
    if (operand.compare(0, 3, "st(") != 0) return false;
    if (operand.back() != ')') return false;
    const std::string digits = operand.substr(3, operand.size() - 4);
    if (digits.size() != 1 || !std::isdigit(static_cast<unsigned char>(digits[0]))) return false;
    index = digits[0] - '0';
    return true;
}

double as_float(std::uint32_t bits) {
    float f;
    std::memcpy(&f, &bits, sizeof(f));
    return static_cast<double>(f);
}

std::uint32_t fbitsOf(double value) {
    float f = static_cast<float>(value);
    std::uint32_t bits;
    std::memcpy(&bits, &f, sizeof(bits));
    return bits;
}

std::int32_t toSigned(std::uint32_t value) { return static_cast<std::int32_t>(value); }

double x87Divide(double numerator, double denominator) {
    if (denominator != 0.0) return numerator / denominator;
    if (numerator == 0.0) return std::nan("");
    return std::copysign(std::numeric_limits<double>::infinity(),
                          numerator * std::copysign(1.0, denominator));
}

double applyMathIntrinsic(MathIntrinsic which, double x) {
    switch (which) {
        case MathIntrinsic::Cos: return std::cos(x);
        case MathIntrinsic::Sin: return std::sin(x);
        case MathIntrinsic::Tan: return std::tan(x);
        case MathIntrinsic::Sqrt: return std::sqrt(x);
        case MathIntrinsic::Log: return std::log(x);
        case MathIntrinsic::Asin: return std::asin(x);
        case MathIntrinsic::Acos: return std::acos(x);
        case MathIntrinsic::Atan: return std::atan(x);
        case MathIntrinsic::Exp: return std::exp(x);
    }
    throw HybridPressureMachineError("unreachable math intrinsic");
}

}  // namespace

double HybridPressureMachine::Result::f32(std::uint32_t address) const {
    auto it = memory.find(address);
    return as_float(it == memory.end() ? 0u : it->second);
}

namespace {

struct LedgerRuntime {
    std::unordered_map<std::uint32_t, std::pair<std::string, std::string>> operations;
    std::unordered_map<std::uint32_t, std::uint32_t> nextAddress;
};

const HybridPressureMachine::OperationLedger& hybridLedger() {
    static const HybridPressureMachine::OperationLedger ledger{
        kFunctions, kInitialDwords, kTableDwords, kOperations, kThunks, kMathIntrinsics};
    return ledger;
}

const LedgerRuntime& runtimeFor(const HybridPressureMachine::OperationLedger& ledger) {
    // Immutable generated ledgers have static lifetime.  Decode each one once
    // so the existing Hybrid/SOSE parity campaign remains fast.
    static std::unordered_map<const void*, LedgerRuntime> runtimes;
    const void* key = static_cast<const void*>(&ledger.operations);
    auto found = runtimes.find(key);
    if (found != runtimes.end()) return found->second;
    LedgerRuntime runtime;
    runtime.operations.reserve(ledger.operations.size());
    for (const auto& op : ledger.operations) runtime.operations[op.address] = {op.opcode, op.operands};
    runtime.nextAddress.reserve(ledger.operations.size());
    for (std::size_t i = 0; i + 1 < ledger.operations.size(); ++i) {
        runtime.nextAddress[ledger.operations[i].address] = ledger.operations[i + 1].address;
    }
    for (const auto& [start, end] : ledger.functions) {
        std::vector<std::uint32_t> addresses;
        for (const auto& op : ledger.operations) {
            if (op.address >= start && op.address < end) addresses.push_back(op.address);
        }
        for (std::size_t i = 0; i + 1 < addresses.size(); ++i) {
            runtime.nextAddress[addresses[i]] = addresses[i + 1];
        }
    }
    return runtimes.emplace(key, std::move(runtime)).first->second;
}

}  // namespace

HybridPressureMachine::HybridPressureMachine(
    std::unordered_map<std::uint32_t, std::uint32_t> extraMemory,
    const OperationLedger* requestedLedger)
    : operations_(runtimeFor(requestedLedger ? *requestedLedger : hybridLedger()).operations),
      nextAddress_(runtimeFor(requestedLedger ? *requestedLedger : hybridLedger()).nextAddress),
      thunks_((requestedLedger ? *requestedLedger : hybridLedger()).thunks),
      mathIntrinsics_((requestedLedger ? *requestedLedger : hybridLedger()).mathIntrinsics),
      externalCall_((requestedLedger ? *requestedLedger : hybridLedger()).externalCall) {
    const auto& ledger = requestedLedger ? *requestedLedger : hybridLedger();
    for (const auto& [addr, value] : ledger.initialDwords) memory_[addr] = value;
    for (const auto& [addr, value] : ledger.tableDwords) memory_[addr] = value;
    for (const auto& [addr, value] : extraMemory) memory_[addr] = value & kMask;

    for (const auto& name : {"eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp"}) {
        reg_[name] = 0;
    }
    reg_["esp"] = 0x70000000u;
}

double HybridPressureMachine::indirectStackF32(std::size_t argumentIndex) {
    const std::uint32_t pointer = readDword(
        reg_["esp"] + static_cast<std::uint32_t>(argumentIndex * sizeof(std::uint32_t)));
    return as_float(readDword(pointer));
}

void HybridPressureMachine::writeIndirectStackF32(std::size_t argumentIndex, double value) {
    const std::uint32_t pointer = readDword(
        reg_["esp"] + static_cast<std::uint32_t>(argumentIndex * sizeof(std::uint32_t)));
    writeDword(pointer, fbitsOf(value));
}

std::uint32_t HybridPressureMachine::readDword(std::uint32_t address) {
    auto it = memory_.find(address);
    return it == memory_.end() ? 0u : it->second;
}

void HybridPressureMachine::writeDword(std::uint32_t address, std::uint32_t value) {
    memory_[address] = value & kMask;
}

std::uint32_t HybridPressureMachine::getReg(const std::string& name) const {
    auto it = reg_.find(name);
    if (it != reg_.end()) return it->second;
    if (name == "ax") return reg_.at("eax") & 0xFFFFu;
    if (name == "ah") return (reg_.at("eax") >> 8) & 0xFFu;
    static const std::unordered_map<std::string, std::string> kByteToFull = {
        {"al", "eax"}, {"bl", "ebx"}, {"cl", "ecx"}, {"dl", "edx"}};
    auto full = kByteToFull.find(name);
    if (full != kByteToFull.end()) return reg_.at(full->second) & 0xFFu;
    throw HybridPressureMachineError("unsupported register " + name);
}

void HybridPressureMachine::setReg(const std::string& name, std::uint32_t value) {
    value &= kMask;
    auto it = reg_.find(name);
    if (it != reg_.end()) {
        it->second = value;
        return;
    }
    if (name == "ax") {
        reg_["eax"] = (reg_["eax"] & 0xFFFF0000u) | (value & 0xFFFFu);
        return;
    }
    if (name == "ah") {
        reg_["eax"] = (reg_["eax"] & 0xFFFF00FFu) | ((value & 0xFFu) << 8);
        return;
    }
    static const std::unordered_map<std::string, std::string> kByteToFull = {
        {"al", "eax"}, {"bl", "ebx"}, {"cl", "ecx"}, {"dl", "edx"}};
    auto full = kByteToFull.find(name);
    if (full != kByteToFull.end()) {
        reg_[full->second] = (reg_[full->second] & 0xFFFFFF00u) | (value & 0xFFu);
        return;
    }
    throw HybridPressureMachineError("unsupported register " + name);
}

std::uint32_t HybridPressureMachine::addressExpr(const std::string& expressionIn) const {
    std::string expression = toLower(trim(expressionIn));
    std::string replaced;
    replaced.reserve(expression.size() * 2);
    for (char c : expression) {
        if (c == '-') {
            replaced += "+-";
        } else {
            replaced += c;
        }
    }
    std::int64_t total = 0;
    for (const auto& termRaw : splitChar(replaced, '+')) {
        std::string term = trim(termRaw);
        if (term.empty()) continue;
        int sign = 1;
        if (term[0] == '-') {
            sign = -1;
            term = trim(term.substr(1));
        }
        const std::size_t star = term.find('*');
        if (star != std::string::npos) {
            const std::string regName = trim(term.substr(0, star));
            const std::string scaleStr = trim(term.substr(star + 1));
            total += static_cast<std::int64_t>(sign) *
                     static_cast<std::int64_t>(getReg(regName)) * parseIntAuto(scaleStr);
        } else if (isFullRegisterName(term)) {
            total += static_cast<std::int64_t>(sign) * static_cast<std::int64_t>(getReg(term));
        } else {
            total += static_cast<std::int64_t>(sign) * parseIntAuto(term);
        }
    }
    return static_cast<std::uint32_t>(static_cast<std::uint64_t>(total) & kMask);
}

std::uint32_t HybridPressureMachine::memoryAddress(const std::string& operand) const {
    const MemoryOperand parsed = parseMemoryOperand(trim(toLower(operand)));
    if (!parsed.isMemory) throw HybridPressureMachineError("not a memory operand: " + operand);
    return addressExpr(parsed.expr);
}

std::uint32_t HybridPressureMachine::readInt(const std::string& operandIn) {
    const std::string operand = trim(toLower(operandIn));
    const MemoryOperand parsed = parseMemoryOperand(operand);
    if (parsed.isMemory) {
        const std::uint32_t value = readDword(addressExpr(parsed.expr));
        std::uint32_t mask = kMask;
        if (parsed.sizePrefix == "byte") mask = 0xFFu;
        else if (parsed.sizePrefix == "word") mask = 0xFFFFu;
        return value & mask;
    }
    if (isFullRegisterName(operand) || operand == "ax" || operand == "ah") {
        return getReg(operand);
    }
    return static_cast<std::uint32_t>(static_cast<std::uint64_t>(parseIntAuto(operand)) & kMask);
}

void HybridPressureMachine::writeInt(const std::string& operandIn, std::uint32_t value) {
    const std::string operand = trim(toLower(operandIn));
    const MemoryOperand parsed = parseMemoryOperand(operand);
    if (parsed.isMemory) {
        const std::uint32_t address = addressExpr(parsed.expr);
        std::uint32_t mask = kMask;
        if (parsed.sizePrefix == "byte") mask = 0xFFu;
        else if (parsed.sizePrefix == "word") mask = 0xFFFFu;
        writeDword(address, (readDword(address) & ~mask) | (value & mask));
        return;
    }
    setReg(operand, value);
}

double HybridPressureMachine::readFloat(const std::string& operandIn) {
    const std::string operand = trim(toLower(operandIn));
    int stIndex = 0;
    if (isStOperand(operand, stIndex)) return x87_.get(stIndex);
    return as_float(readInt(operand));
}

void HybridPressureMachine::push(std::uint32_t value) {
    reg_["esp"] = (reg_["esp"] - 4) & kMask;
    writeDword(reg_["esp"], value);
}

std::uint32_t HybridPressureMachine::pop() {
    const std::uint32_t value = readDword(reg_["esp"]);
    reg_["esp"] = (reg_["esp"] + 4) & kMask;
    return value;
}

void HybridPressureMachine::setSubFlags(std::uint32_t left, std::uint32_t right,
                                         std::uint32_t result) {
    zf_ = (result == 0);
    sf_ = (result & 0x80000000u) != 0;
    cf_ = left < right;
    of_ = (((left ^ right) & (left ^ result)) & 0x80000000u) != 0;
}

void HybridPressureMachine::setLogicFlags(std::uint32_t result) {
    zf_ = (result == 0);
    sf_ = (result & 0x80000000u) != 0;
    cf_ = false;
    of_ = false;
}

HybridPressureMachine::Result HybridPressureMachine::run(
    std::uint32_t entry, const std::vector<std::uint32_t>& arguments, std::size_t limit) {
    for (auto it = arguments.rbegin(); it != arguments.rend(); ++it) push(*it);
    push(0);

    std::uint32_t pc = entry;
    std::size_t visitedCount = 0;

    while (pc != 0) {
        if (visitedCount >= limit) {
            throw HybridPressureMachineError("Hybrid pressure instruction limit exceeded");
        }
        ++visitedCount;
        currentPc_ = pc;

        const auto opIt = operations_.find(pc);
        if (opIt == operations_.end()) {
            std::ostringstream oss;
            oss << "no translated operation at 0x" << std::hex << pc;
            throw HybridPressureMachineError(oss.str());
        }
        const std::string& opcode = opIt->second.first;
        const std::string& text = opIt->second.second;

        std::vector<std::string> operands;
        for (const auto& part : splitChar(text, ',')) {
            const std::string trimmed = trim(toLower(part));
            if (!trimmed.empty()) operands.push_back(trimmed);
        }

        auto nextIt = nextAddress_.find(pc);
        std::uint32_t nextPc = (nextIt == nextAddress_.end()) ? 0u : nextIt->second;

        if (opcode == "push") {
            push(readInt(operands[0]));
        } else if (opcode == "pop") {
            writeInt(operands[0], pop());
        } else if (opcode == "mov") {
            writeInt(operands[0], readInt(operands[1]));
        } else if (opcode == "lea") {
            const std::string& bracketed = operands[1];
            writeInt(operands[0], addressExpr(bracketed.substr(1, bracketed.size() - 2)));
        } else if (opcode == "xor") {
            const std::uint32_t value = readInt(operands[0]) ^ readInt(operands[1]);
            writeInt(operands[0], value);
            setLogicFlags(value);
        } else if (opcode == "and" || opcode == "or") {
            const std::uint32_t left = readInt(operands[0]);
            const std::uint32_t right = readInt(operands[1]);
            const std::uint32_t value = (opcode == "and") ? (left & right) : (left | right);
            writeInt(operands[0], value);
            setLogicFlags(value);
        } else if (opcode == "add" || opcode == "sub") {
            const std::uint32_t left = readInt(operands[0]);
            const std::uint32_t right = readInt(operands[1]);
            const std::uint32_t value =
                (opcode == "add") ? ((left + right) & kMask) : ((left - right) & kMask);
            writeInt(operands[0], value);
            if (opcode == "sub") setSubFlags(left, right, value);
            else setLogicFlags(value);
        } else if (opcode == "dec") {
            const std::uint32_t left = readInt(operands[0]);
            const std::uint32_t value = (left - 1) & kMask;
            writeInt(operands[0], value);
            setSubFlags(left, 1, value);
        } else if (opcode == "neg") {
            const std::uint32_t value = (~readInt(operands[0]) + 1) & kMask;
            writeInt(operands[0], value);
            setLogicFlags(value);
        } else if (opcode == "sahf") {
            const std::uint32_t ah = getReg("ah");
            cf_ = (ah & 0x01u) != 0;
            zf_ = (ah & 0x40u) != 0;
            sf_ = (ah & 0x80u) != 0;
        } else if (opcode == "seta") {
            writeInt(operands[0], (!cf_ && !zf_) ? 1u : 0u);
        } else if (opcode == "cdq") {
            reg_["edx"] = (toSigned(reg_["eax"]) < 0) ? kMask : 0u;
        } else if (opcode == "idiv") {
            const std::int64_t divisor = static_cast<std::int64_t>(toSigned(readInt(operands[0])));
            std::int64_t dividend = (static_cast<std::int64_t>(reg_["edx"]) << 32) |
                                     static_cast<std::int64_t>(reg_["eax"]);
            // dividend above was built from unsigned 32-bit halves; reinterpret
            // the full 64 bits as signed, matching Python's explicit
            // "subtract 2**64 if bit 63 set" step.
            std::uint64_t udividend = (static_cast<std::uint64_t>(reg_["edx"]) << 32) |
                                       static_cast<std::uint64_t>(reg_["eax"]);
            std::int64_t signedDividend;
            if (udividend & (1ULL << 63)) {
                signedDividend = static_cast<std::int64_t>(udividend - (1ULL << 63)) -
                                  static_cast<std::int64_t>(1ULL << 63);
            } else {
                signedDividend = static_cast<std::int64_t>(udividend);
            }
            (void)dividend;
            const double q =
                std::trunc(static_cast<double>(signedDividend) / static_cast<double>(divisor));
            const std::int64_t quotient = static_cast<std::int64_t>(q);
            const std::int64_t remainder = signedDividend - quotient * divisor;
            reg_["eax"] = static_cast<std::uint32_t>(static_cast<std::uint64_t>(quotient) & kMask);
            reg_["edx"] =
                static_cast<std::uint32_t>(static_cast<std::uint64_t>(remainder) & kMask);
        } else if (opcode == "imul") {
            std::int64_t value;
            if (operands.size() == 3) {
                value = static_cast<std::int64_t>(toSigned(readInt(operands[1]))) *
                        static_cast<std::int64_t>(toSigned(readInt(operands[2])));
            } else {
                value = static_cast<std::int64_t>(toSigned(readInt(operands[0]))) *
                        static_cast<std::int64_t>(toSigned(readInt(operands[1])));
            }
            writeInt(operands[0], static_cast<std::uint32_t>(static_cast<std::uint64_t>(value) & kMask));
        } else if (opcode == "cmp") {
            const std::uint32_t left = readInt(operands[0]);
            const std::uint32_t right = readInt(operands[1]);
            setSubFlags(left, right, (left - right) & kMask);
        } else if (opcode == "bound") {
            // Native-valid campaign inputs never take the bounds-failure trap.
        } else if (opcode == "fld") {
            x87_.push(readFloat(operands[0]));
        } else if (opcode == "fild") {
            x87_.push(static_cast<double>(toSigned(readInt(operands[0]))));
        } else if (opcode == "fnstcw") {
            writeInt(operands[0], x87ControlWord_);
        } else if (opcode == "fldcw") {
            x87ControlWord_ = readInt(operands[0]) & 0xFFFFu;
        } else if (opcode == "fistp") {
            const double value = x87_.get();
            const std::uint32_t mode = (x87ControlWord_ >> 10) & 3u;
            std::int64_t integer;
            if (mode == 3) {
                integer = static_cast<std::int64_t>(std::trunc(value));
            } else {
                integer = static_cast<std::int64_t>(std::nearbyint(value));
            }
            writeInt(operands[0], static_cast<std::uint32_t>(static_cast<std::uint64_t>(integer) & kMask));
            x87_.pop();
        } else if (opcode == "fstp") {
            int stIndex = 0;
            if (isStOperand(operands[0], stIndex)) {
                x87_.storePop(stIndex);
            } else {
                writeInt(operands[0], fbitsOf(x87_.get()));
                x87_.pop();
            }
        } else if (opcode == "ffree") {
            int stIndex = 0;
            isStOperand(operands[0], stIndex);
            x87_.valid[x87_.ix(stIndex)] = false;
        } else if (opcode == "fadd" || opcode == "fmul" || opcode == "fsub" || opcode == "fsubr" ||
                   opcode == "fdiv" || opcode == "fdivr") {
            const double left = x87_.get();
            const double right = readFloat(operands[0]);
            double value;
            if (opcode == "fadd") value = left + right;
            else if (opcode == "fmul") value = left * right;
            else if (opcode == "fsub") value = left - right;
            else if (opcode == "fsubr") value = right - left;
            else if (opcode == "fdiv") value = x87Divide(left, right);
            else value = x87Divide(right, left);
            x87_.set(value);
        } else if (opcode == "fabs") {
            x87_.set(std::fabs(x87_.get()));
        } else if (opcode == "fchs") {
            x87_.set(-x87_.get());
        } else if (opcode == "fcom" || opcode == "fcomp") {
            const double left = x87_.get();
            const double right = readFloat(operands[0]);
            std::uint32_t ah;
            if (std::isnan(left) || std::isnan(right)) ah = 0x45u;
            else if (left < right) ah = 0x01u;
            else if (left == right) ah = 0x40u;
            else ah = 0u;
            if (opcode == "fcomp") x87_.pop();
            setReg("ah", ah);
        } else if (opcode == "fnstsw") {
            setReg(operands[0], getReg("ah") << 8);
        } else if (opcode == "je" || opcode == "jne" || opcode == "jg" || opcode == "jge" ||
                   opcode == "jle" || opcode == "jl") {
            bool take;
            if (opcode == "je") take = zf_;
            else if (opcode == "jne") take = !zf_;
            else if (opcode == "jg") take = !zf_ && (sf_ == of_);
            else if (opcode == "jge") take = (sf_ == of_);
            else if (opcode == "jle") take = zf_ || (sf_ != of_);
            else take = (sf_ != of_);
            if (take) nextPc = static_cast<std::uint32_t>(parseIntAuto(operands[0]));
        } else if (opcode == "jmp") {
            nextPc = static_cast<std::uint32_t>(parseIntAuto(operands[0]));
        } else if (opcode == "call") {
            const std::uint32_t target = static_cast<std::uint32_t>(parseIntAuto(operands[0]));
            auto thunkIt = std::find_if(thunks_.begin(), thunks_.end(),
                                         [&](const auto& p) { return p.first == target; });
            auto mathIt = std::find_if(mathIntrinsics_.begin(), mathIntrinsics_.end(),
                                        [&](const auto& p) { return p.first == target; });
            if (thunkIt != thunks_.end()) {
                push(nextPc);
                nextPc = thunkIt->second;
            } else if (mathIt != mathIntrinsics_.end()) {
                x87_.set(applyMathIntrinsic(mathIt->second, x87_.get()));
            } else if (externalCall_ != nullptr && externalCall_(*this, target)) {
                // The handler mirrors a recovered helper that returns
                // immediately; its caller retains ownership of stack cleanup.
            } else if (operations_.find(target) != operations_.end()) {
                // A recovered ledger may contain a direct call to another
                // recovered function instead of a thunk.  Preserve the
                // native return stack exactly and interpret that entry in the
                // same immutable program image.
                push(nextPc);
                nextPc = target;
            } else if (target == 0x00543CB3u) {
                const double exponent = x87_.get(0);
                const double base = x87_.get(1);
                const double result = std::pow(base, exponent);
                x87_.pop();
                x87_.set(result);
            } else if (target == 0x00544290u) {
                const double base = as_float(readDword(reg_["esp"]));
                const std::int32_t exponent = toSigned(readDword(reg_["esp"] + 4));
                x87_.set(std::pow(base, exponent));
            } else if (target == 0x00541360u || target == 0x00541990u) {
                // no-op
            } else {
                std::ostringstream oss;
                oss << "unsupported call at 0x" << std::hex << pc << " to 0x" << target;
                throw HybridPressureMachineError(oss.str());
            }
        } else if (opcode == "ret") {
            nextPc = pop();
            reg_["esp"] =
                (reg_["esp"] + (operands.empty() ? 0u : static_cast<std::uint32_t>(
                                                             parseIntAuto(operands[0])))) &
                kMask;
        } else if (opcode == "int3") {
            std::ostringstream oss;
            oss << "unexpected padding at 0x" << std::hex << pc;
            throw HybridPressureMachineError(oss.str());
        } else {
            std::ostringstream oss;
            oss << "unsupported 0x" << std::hex << pc << ": " << opcode << " " << text;
            throw HybridPressureMachineError(oss.str());
        }

        pc = nextPc;
    }

    Result result;
    result.memory = memory_;
    result.instructionCount = visitedCount;
    return result;
}

}  // namespace datcom_equivalent::body
