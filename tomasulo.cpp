#include <bits/stdc++.h>
using namespace std;

#include "tomasulo.h"

int t = 1;

vector<Inst *> insts;
int cur;

ArithmeticBuffer arss[6];
ArithmeticBuffer mrss[3];
LoadBuffer lbs[3];
const int arss_length = 6, mrss_length = 3, lbs_length = 3;
string to_string(ReservationStation *rs)
{
    if (rs == nullptr)
        return "";
    if (rs->op == "LD")
    {
        int i = (LoadBuffer *)rs - lbs;
        assert(i >= 0 && i < lbs_length);
        return "LB " + to_string(i + 1);
    }
    else
    {
        int i = (ArithmeticBuffer *)rs - arss;
        int j = (ArithmeticBuffer *)rs - mrss;
        // printf("i = %d, j = %d\n", i, j);
        assert(i >= 0 && i < arss_length || j >= 0 && j < mrss_length);
        if (i >= 0 && i < arss_length)
            return "Ars " + to_string(i + 1);
        else
            return "Mrs " + to_string(j + 1);
    }
}

FunctionUnit fus[7];
FunctionUnit *adds = fus;
FunctionUnit *mults = fus + 3;
FunctionUnit *loads = fus + 5;
const int adds_length = 3, mults_length = 2, loads_length = 2;
bool isArithmeticUnit(FunctionUnit *fu)
{
    int i = fu - fus;
    assert(i >= 0);
    assert(i < adds_length + mults_length + loads_length);

    printf("The fus[%d] is finishing.\n", i);

    return i < adds_length + mults_length;
}
string to_string(FunctionUnit *fu)
{
    int i = fu - fus;
    if (i < 0 || i >= 7)
        return "";
    if (i < adds_length)
        return "Add " + to_string(i + 1);
    if (i < adds_length + mults_length)
        return "Mult " + to_string(i - adds_length + 1);
    assert(i < 7);
    return "Load " + to_string(i - adds_length - mults_length + 1);
}

Register regs[32];

vector<FunctionUnit *> cdb;

void try_write()
{
    for (auto fu : cdb)
    {
        int res = 0, rid = -1;
        if (isArithmeticUnit(fu))
        {
            ArithmeticBuffer *ab = (ArithmeticBuffer *)(fu->rs);
            res = 0;
            auto inst = ab->getInst();
            if (inst->op == "ADD")
                res = ab->Vj + ab->Vk;
            else if (inst->op == "SUB")
                res = ab->Vj - ab->Vk;
            else if (inst->op == "MUL")
                res = ab->Vj * ab->Vk;
            else
            {
                assert(inst->op == "DIV");
                if (ab->Vk == 0)
                    res = ab->Vj;
                else
                    res = ab->Vj / ab->Vk;
            }
            rid = inst->reg[0];
            if (regs[rid].rs == ab)
            {
                regs[rid].stat = res;
                regs[rid].rs = nullptr;
            }

            inst->writeTime = t;

            ab->busy = false;
        }
        else
        {
            LoadBuffer *lb = (LoadBuffer *)(fu->rs);
            auto inst = lb->getInst();
            if (inst->op != "LD")
            {
                cout << inst->op << " should be LD" << endl;
            }
            assert(inst->op == "LD");
            res = inst->imm;
            rid = inst->reg;
            if (regs[rid].rs == lb)
            {
                regs[rid].stat = res;
                regs[rid].rs = nullptr;
            }

            inst->writeTime = t;

            lb->busy = false;
        }
        // notify other reservation stations
        printf("notify\n");
        for (int i = 0; i < arss_length; ++i)
            if (arss[i].busy && (arss[i].Qj == fu->rs || arss[i].Qk == fu->rs))
            {
                if (arss[i].Qj == fu->rs)
                {
                    arss[i].Vj = res;
                    arss[i].Qj = nullptr;
                }
                if (arss[i].Qk == fu->rs)
                {
                    arss[i].Vk = res;
                    arss[i].Qk = nullptr;
                }
                if (arss[i].Qj == nullptr && arss[i].Qk == nullptr)
                    arss[i].inst->readyTime = t;
            }
        for (int i = 0; i < mrss_length; ++i)
        {
            printf("mrss[%d]: busy: %d, eq (Qj): %d, eq (Qk): %d\n", i, mrss[i].busy, mrss[i].Qj == fu->rs, mrss[i].Qk == fu->rs);
            if (mrss[i].busy && (mrss[i].Qj == fu->rs || mrss[i].Qk == fu->rs))
            {
                if (mrss[i].Qj == fu->rs)
                {
                    mrss[i].Vj = res;
                    mrss[i].Qj = nullptr;
                }
                if (mrss[i].Qk == fu->rs)
                {
                    mrss[i].Vk = res;
                    mrss[i].Qk = nullptr;
                }
                if (mrss[i].Qj == nullptr && mrss[i].Qk == nullptr)
                    mrss[i].inst->readyTime = t;
            }
        }

        fu->rs = nullptr;
    }
    cdb.clear();
}

void try_issue()
{
    if (cur == insts.size())
        return;

    string op = insts[cur]->op;
    if (op == "LD")
    {
        LoadBuffer *lb = nullptr;
        for (int i = 0; i < lbs_length; ++i)
            if (!lbs[i].busy)
            {
                lb = lbs + i;
                break;
            }
        if (lb == nullptr)
            return;

        cout << "issue at " << to_string(lb) << endl;

        LoadInst *inst = (LoadInst *)(insts[cur++]);

        inst->issueTime = inst->readyTime = t;

        regs[inst->reg].rs = (ReservationStation *)lb;

        lb->busy = true;
        lb->executed = false;
        lb->inst = inst;
        lb->imm = inst->imm;
    }
    else
    {
        ArithmeticBuffer *ab = nullptr;
        if (op == "ADD" || op == "SUB")
        {
            for (int i = 0; i < arss_length; ++i)
                if (!arss[i].busy)
                {
                    ab = arss + i;
                    break;
                }
        }
        else
        {
            assert(op == "MUL" || op == "DIV");
            for (int i = 0; i < mrss_length; ++i)
                if (!mrss[i].busy)
                {
                    ab = mrss + i;
                    break;
                }
        }
        if (ab == nullptr)
            return;

        cout << "issue at " << to_string(ab) << endl;

        ArithmeticInst *inst = (ArithmeticInst *)(insts[cur++]);

        inst->issueTime = t;

        regs[inst->reg[0]].rs = (ReservationStation *)ab;

        ab->busy = true;
        ab->executed = false;
        ab->inst = inst;
        if (regs[inst->reg[1]].rs != nullptr)
        {
            ab->Vj = 0;
            ab->Qj = regs[inst->reg[1]].rs;
        }
        else
        {
            ab->Vj = regs[inst->reg[1]].stat;
            ab->Qj = nullptr;
        }
        if (regs[inst->reg[2]].rs != nullptr)
        {
            ab->Vk = 0;
            ab->Qk = regs[inst->reg[2]].rs;
        }
        else
        {
            ab->Vk = regs[inst->reg[2]].stat;
            ab->Qk = nullptr;
        }

        if (ab->Qj == nullptr && ab->Qk == nullptr)
            inst->readyTime = t;
    }
}

void try_execute()
{
    // from arss to adds
    for (int i = 0; i < adds_length; ++i)
    {
        ArithmeticBuffer *ab = (ArithmeticBuffer *)(adds[i].rs);
        if (ab == nullptr)
        {
            for (int j = 0; j < arss_length; ++j)
                if (arss[j].busy && !arss[j].executed && arss[j].Qj == nullptr && arss[j].Qk == nullptr &&
                    (ab == nullptr || arss[j].inst->readyTime < ab->inst->readyTime || arss[j].inst->readyTime == ab->inst->readyTime && arss[j].inst->issueTime < ab->inst->issueTime))
                {
                    ab = arss + j;
                }
            if (ab == nullptr)
                break;
            adds[i].rs = (ReservationStation *)ab;
            adds[i].need_wait = true;
            adds[i].remain = 3;

            ab->executed = true;
        }
    }
    // from mrss to mults
    for (int i = 0; i < mults_length; ++i)
    {
        ArithmeticBuffer *ab = (ArithmeticBuffer *)(mults[i].rs);
        if (ab == nullptr)
        {
            for (int j = 0; j < mrss_length; ++j)
            {
                printf("Try from mrss[%d] to mults[%d]\n", i, j);
                if (mrss[j].busy && !mrss[j].executed && mrss[j].Qj == nullptr && mrss[j].Qk == nullptr &&
                    (ab == nullptr || mrss[j].inst->readyTime < ab->inst->readyTime || mrss[j].inst->readyTime == ab->inst->readyTime && mrss[j].inst->issueTime < ab->inst->issueTime))
                {
                    ab = mrss + j;
                }
            }
            if (ab == nullptr)
                break;
            mults[i].rs = (ReservationStation *)ab;
            mults[i].need_wait = true;
            if (mrss[i].inst->op == "DIV" && mrss[i].Vk == 0)
                mults[i].remain = 1;
            else
                mults[i].remain = 4;

            ab->executed = true;
        }
    }
    // from lbs to loads
    for (int i = 0; i < loads_length; ++i)
    {
        LoadBuffer *lb = (LoadBuffer *)(loads[i].rs);
        if (lb == nullptr)
        {
            for (int j = 0; j < lbs_length; ++j)
                if (lbs[j].busy && !lbs[j].executed &&
                    (lb == nullptr || lbs[j].inst->readyTime < lb->inst->readyTime || lbs[j].inst->readyTime == lb->inst->readyTime && lbs[j].inst->issueTime < lb->inst->issueTime))
                {
                    lb = lbs + j;
                }
            if (lb == nullptr)
                break;
            loads[i].rs = (ReservationStation *)lb;
            loads[i].need_wait = true;
            loads[i].remain = 3;

            lb->executed = true;
        }
    }

    // let's execute!
    for (int i = 0; i < adds_length + mults_length + loads_length; ++i)
        if (fus[i].rs != nullptr)
        {
            if (fus[i].need_wait)
                fus[i].need_wait = false;
            else
            {
                assert(fus[i].remain > 0);
                --fus[i].remain;
                if (fus[i].remain == 0)
                {
                    fus[i].rs->inst->finishTime = t;
                    cdb.push_back(fus + i);
                }
            }
        }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: ./tomasulo <file>\n");
        printf("Example: ./tomasulo 0.basic\n");
        return 0;
    }
    string filename(argv[1]);
    if (freopen(("TestCase/" + filename + ".nel").c_str(), "r", stdin) == nullptr)
    {
        printf("Error: the file doesn't exist.\n");
        printf("Usage: ./tomasulo <nel file>\n");
        return 0;
    }

    // read instructions
    for (string s; cin >> s;)
    {
        // split by comma
        stringstream ss(s);
        vector<string> vs;
        while (ss.good())
        {
            string substr;
            getline(ss, substr, ',');
            vs.push_back(substr);
        }

        // parse
        if (vs[0] == "LD")
        {
            assert(vs.size() == 3);
            insts.push_back(new LoadInst(stoi(vs[1].substr(1)), stoi(vs[2], 0, 16)));
        }
        else
        {
            assert(vs.size() == 4);
            insts.push_back(new ArithmeticInst(vs[0], stoi(vs[1].substr(1)), stoi(vs[2].substr(1)), stoi(vs[3].substr(1))));
        }
    }

    printf("After input:\n");
    for (Inst *inst : insts)
        cout << inst->op << endl;

    if (freopen("output.md", "w", stdout) == nullptr)
    {
        printf("Error: cannot create output file, maybe the privillege is not enough.\n");
        return 0;
    }

    printf("After open output.md\n");

    // run!
    for (; t < 100; ++t)
    {
        printf("Before checking.\n");

        bool done = cur == insts.size();
        for (auto ars : arss)
            done &= !ars.busy;
        for (auto mrs : mrss)
            done &= !mrs.busy;
        for (auto lb : lbs)
            done &= !lb.busy;
        if (done)
            break;

        printf("A cycle is beginning.\n");

        // run one cycle
        try_write();

        printf("Tried to write.\n");

        try_issue();

        printf("Tried to issue.\n");

        try_execute();

        printf("Tried to execute.\n");
        // print
        printf("## Cycle %d\n", t);
        printf("### Reservation Stations\n");
        printf("| | Busy | Op | Vj | Vk | Qj | Qk |\n");
        printf("| --- | --- | --- | --- | --- | --- | --- |\n");
        for (int i = 0; i < arss_length; ++i)
        {
            printf("| %s |", to_string((ReservationStation *)(arss + i)).c_str());
            if (arss[i].busy)
            {
                printf(" Yes | %s |", arss[i].inst->op.c_str());
                if (arss[i].Qj == nullptr)
                    printf(" %d |", arss[i].Vj);
                else
                    printf(" |");
                if (arss[i].Qk == nullptr)
                    printf(" %d |", arss[i].Vk);
                else
                    printf(" |");
                printf(" %s | %s |", to_string(arss[i].Qj).c_str(), to_string(arss[i].Qk).c_str());
            }
            else
                printf(" No | | | | |");
            // for debug
            printf(" %c |", "NE"[arss[i].executed]);

            puts("");
        }
        for (int i = 0; i < mrss_length; ++i)
        {
            printf("| %s |", to_string((ReservationStation *)(mrss + i)).c_str());
            if (mrss[i].busy)
            {
                printf(" Yes | %s |", mrss[i].inst->op.c_str());
                if (mrss[i].Qj == nullptr)
                    printf(" %d |", mrss[i].Vj);
                else
                    printf(" |");
                if (mrss[i].Qk == nullptr)
                    printf(" %d |", mrss[i].Vk);
                else
                    printf(" |");
                printf(" %s | %s |", to_string(mrss[i].Qj).c_str(), to_string(mrss[i].Qk).c_str());
            }
            else
                printf(" No | | | | |");
            // for debug
            printf(" %c |", "NE"[mrss[i].executed]);
            puts("");
        }
        printf("### Load Buffers\n");
        printf("| | Busy | Address |\n");
        printf("| --- | --- | --- |\n");
        for (int i = 0; i < lbs_length; ++i)
        {
            // printf("\n to_string (lbs + %d)\n", i);
            printf("| %s |", to_string((ReservationStation *)(lbs + i)).c_str());
            if (lbs[i].busy)
                printf(" Yes | %d |", lbs[i].imm);
            else
                printf(" No | |");
            // for debug
            printf(" %c |", "NE"[lbs[i].executed]);
            puts("");
        }
        printf("### Registers\n");
        printf("| Register | State |\n");
        printf("| --- | --- |\n");
        for (int i = 0; i < 32; ++i)
            printf("| R%d | %s |\n", i, to_string(regs[i].rs).c_str());

        // for debug
        printf("### Function Units\n");
        printf("| | Instruction | Remaining |\n");
        for (int i = 0; i < 7; ++i)
        {
            printf("| %s | ", to_string(fus + i).c_str());
            if (fus[i].rs != nullptr)
                printf("%s | %d |\n", fus[i].rs->inst->to_string().c_str(), fus[i].remain);
            else
                printf("| |\n");
        }
    }

    if (freopen(("Log/2017011326_" + filename + ".log").c_str(), "w", stdout) == nullptr)
    {
        printf("Error: cannot create output file, maybe the privillege is not enough.\n");
        return 0;
    }
    for (auto inst : insts)
        printf("%d %d %d\n", inst->issueTime, inst->finishTime, inst->writeTime);
}