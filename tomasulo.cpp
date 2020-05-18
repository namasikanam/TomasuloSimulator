#include <bits/stdc++.h>
using namespace std;

#include "tomasulo.h"

int t = 1;

vector<Inst *> insts;
int cur;

ReservationStation rss[12];
ArithmeticBuffer *arss = (ArithmeticBuffer *)rss;
ArithmeticBuffer *mrss = (ArithmeticBuffer *)(rss + 6);
LoadBuffer *lbs = (LoadBuffer *)(rss + 9);
const int arss_length = 6, mrss_length = 3, lbs_length = 3;
bool isArithmeticBuffer(ReservationStation *rs)
{
    int i = rs - rss;
    return i >= 0 && i < arss_length + mrss_length;
}

FunctionUnit fus[7];
ArithmeticUnit *adds = (ArithmeticUnit *)fus;
ArithmeticUnit *mults = (ArithmeticUnit *)(fus + 3);
LoadUnit *loads = (LoadUnit *)(fus + 5);
const int adds_length = 3, mults_length = 2, loads_length = 2;
bool isArithmeticUnit(FunctionUnit *fu)
{
    int i = fu - fus;
    return i >= 0 && i < adds_length + mults_length;
}

Register regs[32];

vector<FunctionUnit *> cdb;

void try_write()
{
    for (auto fu : cdb)
    {
        if (isArithmeticUnit(fu))
        {
            ArithmeticUnit *au = (ArithmeticUnit *)fu;
            ArithmeticBuffer *ab = au->getRs();
            int res = 0;
            if (ab->inst->op == "ADD")
                res = ab->Vj + ab->Vk;
            else if (ab->inst->op == "SUB")
                res = ab->Vj - ab->Vk;
            else if (ab->inst->op == "MUL")
                res = ab->Vj * ab->Vk;
            else
            {
                assert(ab->inst->op == "DIV");
                if (ab->Vk == 0)
                    res = ab->Vj;
                else
                    res = ab->Vj / ab->Vk;
            }
            auto inst = ab->getInst();
            int rid = inst->reg[0];
            if (regs[rid].rs == ab)
            {
                regs[rid].stat = res;
                regs[rid].rs = nullptr;
            }

            ab->busy = false;
            au->rs = nullptr;
        }
        else
        {
            LoadUnit *lu = (LoadUnit *)fu;
            auto lb = lu->getRs();
            auto inst = lb->getInst();
            assert(inst->op == "LD");
            int rid = inst->reg;
            if (regs[rid].rs == lb)
            {
                regs[rid].stat = inst->imm;
                regs[rid].rs = nullptr;
            }

            lb->busy = false;
            lu->rs = nullptr;
        }
    }
    cdb.clear();
}

void try_issue()
{
    if (cur == insts.size())
        return;

    string op = insts.front()->op;
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

        LoadInst *inst = (LoadInst *)(insts[cur++]);

        inst->issueTime = t;

        regs[inst->reg].rs = (ReservationStation *)lb;

        lb->busy = true;
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

        ArgInst *inst = (ArgInst *)(insts[cur++]);

        inst->issueTime = t;

        regs[inst->reg[0]].rs = (ReservationStation *)ab;

        ab->busy = true;
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
    }
}

ReservationStation *findMin(ReservationStation *first, ReservationStation *last)
{
    ReservationStation *ans = nullptr;
    for (auto i = first; i < last; ++i)
        if (i->busy &&
            (ans == nullptr || i->inst->readyTime < ans->inst->readyTime || i->inst->readyTime == ans->inst->readyTime && i->inst->issueTime < ans->inst->issueTime))
        {
            ans = i;
        }
    return ans;
}

void try_execute()
{
    // from arss to adds
    for (int i = 0; i < adds_length; ++i)
    {
        auto ab = adds[i].getRs();
        if (ab == nullptr)
        {
            for (int j = 0; j < arss_length; ++j)
                if (arss[j].busy && arss[j].Qj == nullptr && arss[j].Qk == nullptr &&
                    (ab == nullptr || arss[j].inst->readyTime < ab->inst->readyTime || arss[j].inst->readyTime == ab->inst->readyTime && arss[j].inst->issueTime < ab->inst->issueTime))
                {
                    ab = arss + j;
                }
            if (ab == nullptr)
                break;
            adds[i].rs = (ReservationStation *)ab;
            adds[i].need_wait = true;
            adds[i].remain = 3;
        }
    }
    // from mrss to mults
    for (int i = 0; i < mults_length; ++i)
    {
        auto ab = mults[i].getRs();
        if (ab == nullptr)
        {
            for (int j = 0; j < mrss_length; ++j)
                if (mrss[j].busy && mrss[j].Qj == nullptr && mrss[j].Qk == nullptr &&
                    (ab == nullptr || mrss[j].inst->readyTime < ab->inst->readyTime || mrss[j].inst->readyTime == ab->inst->readyTime && mrss[j].inst->issueTime < ab->inst->issueTime))
                {
                    ab = mrss + j;
                }
            if (ab == nullptr)
                break;
            mults[i].rs = (ReservationStation *)ab;
            mults[i].need_wait = true;
            if (mrss[i].Vk == 0)
                mults[i].remain = 1;
            else
                mults[i].remain = 4;
        }
    }
    // from lbs to loads
    for (int i = 0; i < loads_length; ++i)
    {
        auto lb = loads[i].getRs();
        if (lb == nullptr)
        {
            for (int j = 0; j < lbs_length; ++j)
                if (lbs[j].busy &&
                    (lb == nullptr || lbs[j].inst->readyTime < lb->inst->readyTime || lbs[j].inst->readyTime == lb->inst->readyTime && lbs[j].inst->issueTime < lb->inst->issueTime))
                {
                    lb = lbs + j;
                }
            if (lb == nullptr)
                break;
            loads[i].rs = (ReservationStation *)lb;
            loads[i].need_wait = true;
            loads[i].remain = 3;
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
        printf("Usage: ./tomasulo <nel file>");
        return 0;
    }
    string filename(argv[1]);
    freopen(("TestCase/" + filename).c_str(), "r", stdin);

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
            insts.push_back(new ArgInst(vs[0], stoi(vs[1].substr(1)), stoi(vs[2].substr(1)), stoi(vs[3].substr(1))));
        }
    }

    freopen("output.md", "w", stdout);
    // run!
    for (;; ++t)
    {
        bool done = cur == insts.size();
        for (auto rs : rss)
            if (rs.busy)
                done = false;
        if (done)
            break;
        // run one cycle
        try_write();
        try_issue();
        try_execute();
        // TODO: print
    }

    freopen(("Log/" + filename).c_str(), "w", stdout);
    for (auto inst : insts)
        printf("%d %d %d\n", inst->issueTime, inst->finishTime, inst->writeTime);
}