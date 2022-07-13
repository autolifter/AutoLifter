#include "enumerator.h"
#include "util.h"
#include "glog/logging.h"
#include <unordered_map>

namespace {

    void _splitSize(int pos, int rem, const std::vector<std::vector<int>>& pool, std::vector<int>& scheme, std::vector<std::vector<int>>& result) {
        if (pos == pool.size()) {
            if (rem == 0) {
                result.push_back(scheme);
            }
            return;
        }
        for (int value: pool[pos]) {
            if (value <= rem) {
                scheme.push_back(value);
                _splitSize(pos + 1, rem - value, pool, scheme, result);
                scheme.pop_back();
            }
        }
    }

    std::vector<std::vector<int>> splitSize(int tot, const std::vector<std::vector<int>>& pool) {
        std::vector<std::vector<int>> result;
        std::vector<int> scheme;
        _splitSize(0, tot, pool, scheme, result);
        return result;
    }

    void _mergeProgram(int pos, Rule* rule, std::vector<ProgramList>& candidate, ProgramList& sub_list, ProgramList& result) {
        if (pos == candidate.size()) {
            result.push_back(new Program(sub_list, rule->semantics));
            return;
        }
        for (auto* program: candidate[pos]) {
            sub_list.push_back(program);
            _mergeProgram(pos + 1, rule, candidate, sub_list, result);
            sub_list.pop_back();
        }
    }

    ProgramList mergeProgram(Rule* rule, std::vector<ProgramList>& candidate) {
        ProgramList result, sub_program;
        _mergeProgram(0, rule, candidate, sub_program, result);
        return result;
    }
}

// TODO: delete intermediate programs
std::vector<Program *> Enumerator::enumerateProgram(int size_limit) {
    std::vector<Program*> result;
    std::vector<ProgramStorage> storage_list;
    grammar->indexSymbols();
    for (auto* symbol: grammar->symbol_list) {
        storage_list.push_back({{}});
    }
    for (int size = 1; size <= size_upper_bound ; ++size) {
        // grammar->print();
        for (auto* symbol: grammar->symbol_list) {
            storage_list[symbol->id].emplace_back();
            for (auto* rule: symbol->rule_list) {
                std::vector<std::vector<int>> pool;
                for (auto* param_symbol: rule->param_list) {
                    int param_id = param_symbol->id;
                    std::vector<int> choice;
                    for (int i = 1; i < size; ++i) {
                        if (!storage_list[param_id][i].empty()) {
                            choice.push_back(i);
                        }
                    }
                    pool.push_back(choice);
                }
                auto split_list = splitSize(size - 1, pool);
                for (auto& split: split_list) {
                    std::vector<ProgramList> candidate;
                    for (int i = 0; i < split.size(); ++i) {
                        candidate.emplace_back(storage_list[rule->param_list[i]->id][split[i]]);
                    }
                    for (auto* program: mergeProgram(rule, candidate)) {
                        storage_list[symbol->id][size].push_back(program);
                        if (symbol->name == grammar->start_symbol->name && v->verify(program)) {
                            result.push_back(program);
                            if (result.size() >= size_limit) {
                                return result;
                            }
                        }
                    }
                }
            }
        }
    }
    return result;
}

bool OEEnumerator::isInsertProgram(Program *p, NonTerminal* nt) {
    if (p->semantics->oup_type == TSEMANTICS) return true;
    DataList feature;
    for (auto& inp: inp_list) {
        feature.emplace_back(p->run({inp}));
    }
    auto feature_str = nt->name + "@" + util::dataList2String(feature);
    if (feature_pool.find(feature_str) == feature_pool.end()) {
        feature_pool.insert(feature_str);
        return true;
    }
    return false;
}

Program * OEEnumerator::synthesisProgram() {
    std::vector<ProgramStorage> storage_list;
    grammar->indexSymbols();
    for (auto* symbol: grammar->symbol_list) {
        storage_list.push_back({{}});
    }
    for (int size = 1;; ++size) {
        for (auto* symbol: grammar->symbol_list) {
            storage_list[symbol->id].emplace_back();
            for (auto* rule: symbol->rule_list) {
                std::vector<std::vector<int>> pool;
                for (auto* param_symbol: rule->param_list) {
                    int param_id = param_symbol->id;
                    std::vector<int> choice;
                    for (int i = 1; i < size; ++i) {
                        if (!storage_list[param_id][i].empty()) {
                            choice.push_back(i);
                        }
                    }
                    pool.push_back(choice);
                }
                auto split_list = splitSize(size - 1, pool);
                for (auto& split: split_list) {
                    std::vector<ProgramList> candidate;
                    for (int i = 0; i < split.size(); ++i) {
                        candidate.emplace_back(storage_list[rule->param_list[i]->id][split[i]]);
                    }
                    for (auto* program: mergeProgram(rule, candidate)) {
                        if (isInsertProgram(program, symbol)) {
                            storage_list[symbol->id][size].push_back(program);
                            if (symbol->name == grammar->start_symbol->name && v->verify(program))
                                return program;
                        }
                    }
                }
            }
        }
    }
}