#include <iostream>

#include "complete_solver.h"
#include "benchmark.h"
#include "glog/logging.h"
#include "gflags/gflags.h"
#include "semantics_factory.h"
#include "hfta_solver.h"
#include "enumerate_based_solver.h"
#include "brute_force_complete_solver.h"
#include "config.h"

#include "recorder.h"

DEFINE_string(type, "", "The type of the task (d/l/r)");
DEFINE_string(name, "", "The name of the task");
DEFINE_string(oup, "", "The path of the output file");
DEFINE_string(log, "", "The path of the log");
DEFINE_string(solver, "", "Lifting solver (AutoLifter/Enum/Relish)");
DEFINE_bool(verbose, false, "Output complete log");

int main(int argc, char **argv) {
    google::InitGoogleLogging(argv[0]);
    google::ParseCommandLineFlags(&argc, &argv, true);

    std::string domain, name, operator_name, log_file, oup_file;

    domain = FLAGS_type;
    name = FLAGS_name;
    log_file = FLAGS_log;
    oup_file = FLAGS_oup;
    config::KIsVerbose = FLAGS_verbose;

    if (!log_file.empty()) {
        google::SetLogDestination(google::GLOG_INFO, log_file.c_str());
    } else {
        FLAGS_logtostderr = true;
    }

    if (!oup_file.empty()) {
        freopen(oup_file.c_str(), "w", stdout);
    }

    std::vector<Task*> task_list = benchmark::getBenchmark(domain, name);
    recorder::recorder.setName(name);

    LiftingSolver* s = nullptr;
    if (FLAGS_solver == "Enum") {
        s = new BfLiftingSolver(task_list);
    } else {
        SolverBuilder *builder = nullptr;
        if (FLAGS_solver == "AutoLifter") {
            builder = new EnumerateBasedBuilder();
        } else if (FLAGS_solver == "Relish") {
            builder = new HFTASolverBuilder();
        } else assert(0);
        s = new CompleteSolver(task_list, builder, new ExternalSolverBuilder());
    }
    s->synthesis();
    std::cout << "Success" << std::endl;
    s->printSummary();
    return 0;
}
