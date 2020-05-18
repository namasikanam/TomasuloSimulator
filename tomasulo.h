#pragma once

#include <bits/stdc++.h>
using namespace std;

struct Inst
{
    string op;
    int issueTime = -1, readyTime = -1, finishTime = -1, writeTime = -1;

    string log()
    {
        return to_string(issueTime) + " " + to_string(finishTime) + " " + to_string(writeTime);
    }
    Inst(string op) : op(op) {}
};

struct LoadInst : Inst
{
    int reg;
    int imm;

    LoadInst(int reg, int imm) : Inst("LD"), reg(reg), imm(imm) {}
};

struct ArgInst : Inst
{
    int reg[3];

    ArgInst(string op, int reg0, int reg1, int reg2) : Inst(op)
    {
        reg[0] = reg0, reg[1] = reg1, reg[2] = reg2;
    }
};

struct ReservationStation
{
    bool busy = false;
    Inst *inst;
};

struct ArithmeticBuffer : ReservationStation
{
    ReservationStation *Qj = nullptr, *Qk = nullptr;
    int Vj = 0, Vk = 0;

    ArgInst *getInst() {
        return (ArgInst *)inst;
    }
};

struct LoadBuffer : ReservationStation
{
    int imm = 0;
    
    LoadInst *getInst() {
        return (ArgInst *)inst;
    }
};

struct FunctionUnit
{
    bool need_wait;
    int remain;
    ReservationStation *rs;
};

struct ArithmeticUnit : FunctionUnit
{
    ArithmeticBuffer *getRs() {
        return (ArithmeticBuffer *)rs;
    }
};

struct LoadUnit : FunctionUnit
{
    LoadBuffer *getRs() {
        return (LoadBuffer *)rs;
    }
};

struct Register
{
    ReservationStation *rs;
    int stat;
};