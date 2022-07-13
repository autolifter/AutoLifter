//
// Created by pro on 2022/6/30.
//

#include "hfta_solver.h"
#include "util.h"
#include <queue>
#include <set>
#include <basic/semantics_factory.h>
#include "glog/logging.h"

FTANode::~FTANode() {
    for (auto* edge: edge_list) delete edge;
}

void MyFTA::removeUseless() {
    std::queue<FTANode*> Q;
    for (auto* node: final_list) if (node->tag) Q.push(node);
    while (!Q.empty()) {
        auto* node = Q.front();
        for (auto* edge: node->edge_list) {
            for (auto* sub_node: edge->node_list) {
                if (sub_node->tag) continue;
                Q.push(sub_node); sub_node->tag = true;
            }
        }
        Q.pop();
    }
    int now = 0;
    for (auto* node: final_list) {
        if (node->tag) final_list[now++] = node;
    }
    final_list.resize(now);
    for (auto& item: node_map) {
        now = 0;
        for (auto* node: item.second) {
            if (node->tag) item.second[now++] = node;
            else delete node;
        }
        item.second.resize(now);
    }
}

namespace {
    int K = -1;
    bool _isValid(const DataList& l, const DataList& r) {
        assert(l.size() == r.size() && K > 0 && l.size() % K == 0);
        for (int i = 0; i < l.size(); i += K) {
            bool flag = false;
            for (int j = 0; j < K; ++j) {
                if (l[i + j] != r[i + j]) {
                    flag = true; break;
                }
            }
            if (!flag) return false;
        }
        return true;
    }

    bool _isValid(FTANode* l_node, FTANode* r_node) {
        return _isValid(l_node->oup_list, r_node->oup_list);
    }

    bool _isValid(const DataList& full) {
        int size = full.size() / 2;
        DataList l, r;
        for (int i = 0; i < size; ++i) l.push_back(full[i]);
        for (int i = size; i < full.size(); ++i) r.push_back(full[i]);
        return _isValid(l, r);
    }

    Program* buildProgram(FTANode* node) {
        assert(node->edge_list.size());
        ProgramList sub_list;
        for (auto* sub_node: node->edge_list[0]->node_list) {
            sub_list.push_back(buildProgram(sub_node));
        }
        return new Program(sub_list, node->edge_list[0]->semantics);
    }

    int _getSize(FTANode* node, std::unordered_set<FTANode*>& cache) {
        if (cache.find(node) != cache.end()) return node->tag;
        node->tag = 1e9;
        for (auto* e: node->edge_list) {
            int size = 1;
            for (auto* sub: e->node_list) size += _getSize(sub, cache);
            node->tag = std::min(node->tag, size);
        }
        return node->tag;
    }

    Program* _buildS(FTANode* node) {
        for (auto* e: node->edge_list) {
            int size = 1;
            for (auto* sub: e->node_list) size += sub->tag;
            if (size == node->tag) {
                ProgramList sub_prog;
                for (auto* sub: e->node_list) sub_prog.push_back(_buildS(sub));
                return new Program(sub_prog, e->semantics);
            }
        }
        assert(0);
    }
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
}

Data FTANode::getOutput(int id, const DataList& inp) {
    if (symbol->type != TSEMANTICS) return oup_list[id];
    return buildProgram(this)->run(inp);
}

HFTA::HFTA(MyFTA *_l, MyFTA *_r): l(_l), r(_r) {
    l->clear(); r->clear();
    for (auto* l_node: l->final_list) {
        for (auto* r_node: r->final_list) {
            if (_isValid(l_node, r_node)) {
                l_node->tag = true; break;
            }
        }
    }
    for (auto* r_node: r->final_list) {
        for (auto* l_node: r->final_list) {
            if (_isValid(r_node, l_node)) {
                r_node->tag = true; break;
            }
        }
    }
    l->clear(); r->clear();
}

namespace {
    DataList getInpList(Example* example) {
        DataList result;
        for (auto inp: *example) {
            if (inp.getType() == TLIST) result.push_back(inp);
        }
        if (K == -1) K = result.size(); else assert(K == result.size());
        return result;
    }

    std::string _getListFeature(int depth, int id) {
        return std::to_string(depth) + "@" + std::to_string(id);
    }

    void _collect(int k, std::vector<FTANode*>& tmp, const std::vector<std::vector<FTANode*>>& storage, std::vector<std::vector<FTANode*>>& res) {
        if (k == storage.size()) {
            res.push_back(tmp); return;
        }
        for (auto* node: storage[k]) {
            tmp[k] = node; _collect(k + 1, tmp, storage, res);
        }
    }

    std::vector<std::vector<FTANode*>> _flatten(const std::vector<std::vector<FTANode*>>& storage) {
        std::vector<FTANode*> tmp(storage.size());
        std::vector<std::vector<FTANode*>> res;
        _collect(0, tmp, storage, res);
        return res;
    }

    std::pair<std::string, std::string> _getSplitFeature(const DataList& oup_list, int l_size) {
        DataList l, r;
        for (int i = 0; i < l_size; ++i) l.push_back(oup_list[i]);
        for (int i = l_size; i < oup_list.size(); ++i) r.push_back(oup_list[i]);
        return {util::dataList2String(l), util::dataList2String(r)};
    }

    MyFTA* mergeFTA(MyFTA* l, MyFTA* r, int size_limit) {
        auto* grammar = l->g; std::vector<DataList> inp_list;
        for (auto& inp: l->inp_list) inp_list.push_back({inp});
        for (auto& inp: r->inp_list) inp_list.push_back({inp});
        int l_size = l->inp_list.size();
        std::vector<ParamInfo*> info_list(inp_list.size());
        std::unordered_map<std::string, std::vector<FTANode*>> node_map;
        for (int i = 0; i < inp_list.size(); ++i) info_list[i] = new ParamInfo(inp_list[i]);
        int dummy = 0;
        for (int size = 1; size <= size_limit; ++size) {
            for (auto* symbol: grammar->symbol_list) {
                auto list_feature = _getListFeature(size, symbol->id);
                auto& l_list = l->node_map[list_feature], &r_list = r->node_map[list_feature];
                if (l_list.empty() || r_list.empty()) continue;

                std::unordered_set<std::string> l_valid, r_valid;
                for (auto* node: l_list) l_valid.insert(util::dataList2String(node->oup_list));
                for (auto* node: r_list) r_valid.insert(util::dataList2String(node->oup_list));

                std::unordered_map<std::string, FTANode*> new_node_map;
                for (auto* rule: symbol->rule_list) {
                    std::vector<std::vector<int>> size_storage(rule->param_list.size());
                    for (int i = 0; i < rule->param_list.size(); ++i) {
                        for (int j = 0; j < size; ++j) {
                            auto feature = _getListFeature(j, rule->param_list[i]->id);
                            if (node_map.count(feature) && !node_map[feature].empty()) size_storage[i].push_back(j);
                        }
                    }
                    auto size_split = splitSize(size - 1, size_storage);
                    for (auto size_scheme: size_split) {
                        std::vector<std::vector<FTANode *>> sub_storage(rule->param_list.size());
                        for (int i = 0; i < rule->param_list.size(); ++i) {
                            sub_storage[i] = node_map[_getListFeature(size_scheme[i], rule->param_list[i]->id)];
                        }
                        for (auto &sub_list: sub_storage) if (sub_list.empty()) continue;
                        auto sub_scheme = _flatten(sub_storage);
                        for (auto &scheme: sub_scheme) {
                            FTANode *new_node;
                            if (symbol->type == TSEMANTICS) {
                                auto feature = std::to_string(++dummy);
                                new_node = new_node_map[feature] = new FTANode(symbol, size, {});
                            } else {
                                DataList oup_list(info_list.size());
                                for (int i = 0; i < info_list.size(); ++i) {
                                    DataList sub(scheme.size());
                                    for (int j = 0; j < scheme.size(); ++j)
                                        sub[j] = scheme[j]->getOutput(i, inp_list[i]);
                                    oup_list[i] = rule->semantics->run(std::move(sub), info_list[i]);
                                }

                                auto sub_feature = _getSplitFeature(oup_list, l_size);
                                if (l_valid.find(sub_feature.first) == l_valid.end() ||
                                    r_valid.find(sub_feature.second) == r_valid.end())
                                    continue;
                                auto oup_feature = util::dataList2String(oup_list);
                                if (!new_node_map.count(oup_feature))
                                    new_node = new_node_map[oup_feature] = new FTANode(symbol, size, oup_list);
                                else new_node = new_node_map[oup_feature];
                            }
                            auto *edge = new FTAEdge(rule->semantics, scheme);
                            new_node->edge_list.push_back(edge);
                        }
                    }
                }
                auto& new_node = node_map[_getListFeature(size, symbol->id)];
                for (auto& item: new_node_map) new_node.push_back(item.second);
            }
        }
        DataList tmp;
        for (auto& x: inp_list) tmp.push_back(x[0]);
        return new MyFTA(node_map, grammar, tmp);
    }


    MyFTA* constructFTA(Grammar* grammar, int size_limit, const Data& inp) {
        grammar->indexSymbols();
        std::unordered_map<std::string, std::vector<FTANode*>> node_map;
        DataList tmp_inp({inp});
        auto* info = new ParamInfo(tmp_inp);
        int dummy = 0;
        for (int size = 1; size <= size_limit; ++size) {
            for (auto* symbol: grammar->symbol_list) {
                std::unordered_map<std::string, FTANode*> new_node_map;
                for (auto* rule: symbol->rule_list) {
                    std::vector<std::vector<int>> size_storage(rule->param_list.size());
                    for (int i = 0; i < rule->param_list.size(); ++i) {
                        for (int j = 0; j < size; ++j) {
                            auto feature = _getListFeature(j, rule->param_list[i]->id);
                            if (node_map.count(feature) && !node_map[feature].empty()) size_storage[i].push_back(j);
                        }
                    }
                    auto size_split = splitSize(size - 1, size_storage);
                    for (auto size_scheme: size_split) {
                        std::vector<std::vector<FTANode *>> sub_storage(rule->param_list.size());
                        for (int i = 0; i < rule->param_list.size(); ++i) {
                            sub_storage[i] = node_map[_getListFeature(size_scheme[i], rule->param_list[i]->id)];
                        }
                        for (auto &sub_list: sub_storage) if (sub_list.empty()) continue;
                        auto sub_scheme = _flatten(sub_storage);
                        for (auto &scheme: sub_scheme) {
                            DataList sub(scheme.size());
                            for (int i = 0; i < scheme.size(); ++i) sub[i] = scheme[i]->getOutput(0, tmp_inp);
                            Data oup;
                            std::string oup_feature;
                            if (symbol->type == TSEMANTICS) {
                                oup = semantics::curry(rule->semantics, sub, false);
                                oup_feature = std::to_string(++dummy);
                            } else {
                                oup = rule->semantics->run(std::move(sub), info);
                                oup_feature = oup.toString();
                            }
                            FTANode *new_node;
                            if (!new_node_map.count(oup_feature))
                                new_node = new_node_map[oup_feature] = new FTANode(symbol, size, {oup});
                            else new_node = new_node_map[oup_feature];
                            auto *edge = new FTAEdge(rule->semantics, scheme);
                            new_node->edge_list.push_back(edge);
                        }
                    }
                }
                auto& new_node = node_map[_getListFeature(size, symbol->id)];
                for (auto& item: new_node_map) new_node.push_back(item.second);
            }
        }
        delete info;
        return new MyFTA(node_map, grammar, {inp});
    }

    MyFTA* constructFTA(Grammar* g, int depth_limit, const DataList& inp_list) {
        MyFTA* res = nullptr;
        for (auto& inp: inp_list) {
            auto* fta = constructFTA(g, depth_limit, inp);
            if (!res) res = fta;
            else {
                auto* next = mergeFTA(res, fta, depth_limit);
                delete res; delete fta; res = next;
            }
        }
        return res;
    }

    HFTA* constructHFTA(Grammar* g, int depth_limit, std::pair<Example*, Example*> example) {
        DataList l = getInpList(example.first), r = getInpList(example.second);
        return new HFTA(constructFTA(g, depth_limit, l), constructFTA(g, depth_limit, r));
    }

    void _decomposeProgram(Program* program, std::vector<Program*>& result) {
        std::string op = program->semantics->name;
        if (op == "[]" || op == "cons") {
            for (auto* sub_program: program->sub_list) {
                _decomposeProgram(sub_program, result);
            }
            return;
        }
        result.push_back(program);
    }

    std::vector<Program*> decomposeProgram(Program* program) {
        std::vector<Program*> result;
        _decomposeProgram(program, result);
        if (result.size() == 1 && result[0]->toString() == "-1") {
            return {};
        }
        return result;
    }

    Grammar* buildDSL(Task* task) {
        auto* grammar = grammar::getDeepCoderDSL();
        grammar->addParam({TLIST});
        for (auto& extra_semantics: task->extra_semantics) {
            extra_semantics.insertExtraSemantics(grammar);
        }
        auto* start = new NonTerminal("start", TLIST);
        grammar->symbol_list.push_back(start);
        grammar->start_symbol = start;
        NonTerminal* int_symbol = nullptr;
        for (auto* symbol: grammar->symbol_list) {
            if (symbol->name == "int_expr") int_symbol = symbol;
        }
        auto* single_rule = new Rule(semantics::getSemanticsFromName("[]"), {int_symbol});
        auto* cons_rule = new Rule(semantics::getSemanticsFromName("cons"), {int_symbol, start});
        start->rule_list.push_back(single_rule);
        start->rule_list.push_back(cons_rule);
        return grammar;
    }
}

Program * HFTASolver::synthesisFromExample() {
    while (1) {
        auto* merge = mergeFTA(hfta->l, hfta->r, limit);
        std::unordered_set<FTANode*> cache;
        FTANode* now = nullptr; int best = 1e9;
        for (auto* node: merge->final_list) {
            if (_isValid(node->oup_list)) {
                 int size = _getSize(node, cache);
                 if (size < best) {
                     best = size; now = node;
                 }
            }
        }
        if (now) return _buildS(now);
        ++limit; delete hfta; hfta = nullptr;
        auto backup = example_list;
        example_list.clear();
        for (const auto& example: backup) addExample(example);
    }
}

void HFTASolver::addExample(const std::pair<Example *, Example *> &example) {
    if (!hfta) hfta = constructHFTA(grammar, limit, example);
    else {
        auto* next = constructHFTA(grammar, limit, example);
        auto* new_l = mergeFTA(hfta->l, next->l, limit);
        auto* new_r = mergeFTA(hfta->r, next->r, limit);
        delete hfta; delete next;
        hfta = new HFTA(new_l, new_r);
    }
    example_list.push_back(example);
}

std::vector<Program *> HFTASolver::synthesis() {
    int turn_num = 0;
    grammar = buildDSL(task);
    auto example = verify({}, false);
    if (!example.first) return {};
    addExample(example);
    while (true) {
        turn_num += 1;
        auto candidate_result = decomposeProgram(synthesisFromExample());
        if (config::KIsVerbose) {
            LOG(INFO) << turn_num;
            for (auto *p: candidate_result) {
                LOG(INFO) << "  " << p->toString();
            }
        }
        for (auto& current_example: example_list) {
            assert(task->evaluate(candidate_result, current_example));
        }
        example = verify(candidate_result, false);
        if (example.first) {
            addExample(example);
        } else {
            return candidate_result;
        }
    }
}