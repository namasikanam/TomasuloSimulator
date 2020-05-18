#pragma once

struct Inst
{
    std::string op;
    int issueTime = -1, readyTime = -1, finishTime = -1, writeTime = -1;

    std::string log()
    {
        return std::to_string(issueTime) + " " + std::to_string(finishTime) + " " + std::to_string(writeTime);
    }
    Inst(std::string op) : op(op) {}

    virtual std::string to_string()
    {
        return "";
    }
};

struct LoadInst : Inst
{
    int reg;
    unsigned imm;

    LoadInst(int reg, unsigned imm) : Inst("LD"), reg(reg), imm(imm) {}

    std::string to_string()
    {
        return "LD,R" + std::to_string(reg) + "," + std::to_string(imm);
    }
};

struct ArithmeticInst : Inst
{
    int reg[3];

    ArithmeticInst(std::string op, int reg0, int reg1, int reg2) : Inst(op)
    {
        reg[0] = reg0, reg[1] = reg1, reg[2] = reg2;
    }

    std::string to_string()
    {
        return op + ",R" + std::to_string(reg[0]) + ",R" + std::to_string(reg[1]) + ",R" + std::to_string(reg[2]);
    }
};

struct ReservationStation
{
    string op = "";
    bool busy = false;
    bool executed = false;
    Inst *inst;
};

struct ArithmeticBuffer : ReservationStation
{
    ReservationStation *Qj = nullptr, *Qk = nullptr;
    unsigned Vj = 0, Vk = 0;

    ArithmeticInst *getInst()
    {
        return (ArithmeticInst *)inst;
    }
};

struct LoadBuffer : ReservationStation
{
    unsigned imm = 0;

    LoadBuffer()
    {
        op = "LD";
    }

    LoadInst *getInst()
    {
        return (LoadInst *)inst;
    }
};

struct FunctionUnit
{
    bool need_wait;
    int remain;
    ReservationStation *rs;
};

struct Register
{
    ReservationStation *rs;
    unsigned stat = 0;
};