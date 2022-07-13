//
// Created by pro on 2022/6/30.
//

#ifndef CPP_HFTA_SOLVER_H
#define CPP_HFTA_SOLVER_H

#include "solver.h"
#include "util.h"
class FTAEdge;

class FTANode {
public:
    NonTerminal* symbol;
    int depth;
    DataList oup_list;
    int tag;
    std::vector<FTAEdge*> edge_list;
    FTANode(NonTerminal* _symbol, int _depth, const DataList& oup): symbol(_symbol), depth(_depth), oup_list(oup) {}
    Data getOutput(int id, const DataList& info);
    ~FTANode();
};

class FTAEdge {
public:
    Semantics* semantics;
    std::vector<FTANode*> node_list;
    FTAEdge(Semantics* _semantics, const std::vector<FTANode*>& _node_list): semantics(_semantics), node_list(_node_list) {}
};

class MyFTA {
public:
    std::unordered_map<std::string, std::vector<FTANode*>> node_map;
    std::vector<FTANode*> final_list;
    Grammar* g;
    DataList inp_list;
    void clear() {
        for (auto &item: node_map) {
            for (auto *node: item.second) node->tag = false;
        }
    }
    void removeUseless();
    MyFTA(const std::unordered_map<std::string, std::vector<FTANode*>>& _node_map, Grammar* _g, const DataList& _inp):
            g(_g), node_map(_node_map), inp_list(_inp) {
        int start_id = g->start_symbol->id;
        for (auto& item: node_map) {
            for (auto* node: item.second) {
                if (node->symbol->id == start_id) final_list.push_back(node);
            }
        }
        clear();
        for (auto* node: final_list) node->tag = true;
        removeUseless();
    }
    void print() {
        for (int i = 0; i < 10; ++i) std::cout << std::endl;
        std::cout << "input: " << util::dataList2String(inp_list) << std::endl;
        for (auto* start: final_list) std::cout << util::dataList2String(start->oup_list) << std::endl;
        for (int i = 0; i < 10; ++i) std::cout << std::endl;
    }
    ~MyFTA() {
        for (auto& item: node_map) {
            for (auto* node: item.second) delete node;
        }
    }
};

class HFTA {
public:
    MyFTA* l, *r;
    HFTA(MyFTA* _l, MyFTA* _r);
    ~HFTA() {delete l; delete r;}
};

class HFTASolver: public Solver {
    int limit;
    HFTA* hfta;
    Grammar* grammar;
    std::vector<std::pair<Example*, Example*>> example_list;
    void addExample(const std::pair<Example*, Example*>& example);
    Program* synthesisFromExample();
public:
    HFTASolver(Task *_task): Solver(_task), limit(4), hfta(nullptr) {
    }
    virtual ~HFTASolver() = default;
    virtual std::vector<Program*> synthesis();
};

class HFTASolverBuilder: public SolverBuilder {
public:
    virtual Solver * build(Task *task) {
        return new HFTASolver(task);
    }
};

#endif //CPP_HFTA_SOLVER_H
