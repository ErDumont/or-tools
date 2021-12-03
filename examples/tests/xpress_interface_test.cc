#include "examples/tests/linear_programming_unittests.h"
#include "ortools/linear_solver/xpress_interface.cc"

namespace operations_research {

  class XPRSGetter : public InterfaceGetter {
  public:
    using InterfaceGetter::InterfaceGetter;

    double getLb(int n) override {
      double lb;
      XPRSgetlb(prob(), &lb, n, n);
      return lb;
    }

    double getUb(int n) override {
      double ub;
      XPRSgetub(prob(), &ub, n, n);
      return ub;
    }

  private:
    XPRSprob prob() {
      return (XPRSprob)solver_->underlying_solver();
    }
  };

  void runAllTests() {
    MPSolver solver("XPRESS_MIP", MPSolver::XPRESS_MIXED_INTEGER_PROGRAMMING);
    XPRSGetter getter(&solver);
    LinearProgrammingTests tests(&solver, &getter);

    tests.testMakeVar(1, 10);
    tests.testMakeVar(0, 1);
    tests.testMakeVar(-10, 140);
  }

}  // namespace operations_research

int main(int argc, char** argv) {
  operations_research::runAllTests();
  return 0;
}