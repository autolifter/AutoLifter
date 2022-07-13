#include "config.h"

const std::string config::KSourcePath = KSOURCEPATH;
const std::string config::KGrammarFilePath = config::KSourcePath + "basic/grammar.json";
const int config::KDefaultValue = 1000000000;
const int config::KIntMin = -5;
const int config::KIntMax = 5;
bool config::is_lifting_only = false;
int config::KMaxBranchNum = 6;
int config::KMaxTermNum = 5;
int config::KTermIntMax = 2;
int config::KRandomC = 5;
int config::KExampleIntMax = 100000000;
int config::KExtraTurnNum = 300;
std::vector<int> global::int_consts;
bool config::KIsVerbose = true;