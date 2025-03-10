#include "ortools/linear_solver/xpress_interface.cc"

#include <fstream>
#include <locale>

#include "gtest/gtest.h"
#define XPRS_NAMELENGTH 1028

namespace operations_research {

#define EXPECT_STATUS(s)                              \
  do {                                                \
    int const status_ = s;                            \
    EXPECT_EQ(0, status_) << "Nonzero return status"; \
  } while (0)

  class XPRSGetter {
  public:
    XPRSGetter(MPSolver* solver) : solver_(solver) {}

    int getNumVariables() {
      int cols;
      EXPECT_STATUS(XPRSgetintattrib(prob(), XPRS_COLS, &cols));
      return cols;
    }

    int getNumConstraints() {
      int cols;
      EXPECT_STATUS(XPRSgetintattrib(prob(), XPRS_ROWS, &cols));
      return cols;
    }

    std::string getRowName(int n) {
      EXPECT_LT(n, getNumConstraints());
      return getName(n, XPRS_NAMES_ROW);
    }

    double getLb(int n) {
      EXPECT_LT(n, getNumVariables());
      double lb;
      EXPECT_STATUS(XPRSgetlb(prob(), &lb, n, n));
      return lb;
    }

    double getUb(int n) {
      EXPECT_LT(n, getNumVariables());
      double ub;
      EXPECT_STATUS(XPRSgetub(prob(), &ub, n, n));
      return ub;
    }

    std::string getColName(int n) {
      EXPECT_LT(n, getNumVariables());
      return getName(n, XPRS_NAMES_COLUMN);
    }

    char getVariableType(int n) {
      EXPECT_LT(n, getNumVariables());
      char type;
      EXPECT_STATUS(XPRSgetcoltype(prob(), &type, n, n));
      return type;
    }

    char getConstraintType(int n) {
      EXPECT_LT(n, getNumConstraints());
      char type;
      EXPECT_STATUS(XPRSgetrowtype(prob(), &type, n, n));
      return type;
    }

    double getConstraintRhs(int n) {
      EXPECT_LT(n, getNumConstraints());
      double rhs;
      EXPECT_STATUS(XPRSgetrhs(prob(), &rhs, n, n));
      return rhs;
    }

    double getConstraintRange(int n) {
      EXPECT_LT(n, getNumConstraints());
      double range;
      EXPECT_STATUS(XPRSgetrhsrange(prob(), &range, n, n));
      return range;
    }

    double getConstraintCoef(int row, int col) {
      EXPECT_LT(col, getNumVariables());
      EXPECT_LT(row, getNumConstraints());
      double coef;
      EXPECT_STATUS(XPRSgetcoef(prob(), row, col, &coef));
      return coef;
    }

    double getObjectiveCoef(int n) {
      EXPECT_LT(n, getNumVariables());
      double objCoef;
      EXPECT_STATUS(XPRSgetobj(prob(), &objCoef, n, n));
      return objCoef;
    }

    double getObjectiveOffset() {
      double offset;
      EXPECT_STATUS(XPRSgetdblattrib(prob(), XPRS_OBJRHS, &offset));
      return offset;
    }

    double getObjectiveSense() {
      double sense;
      EXPECT_STATUS(XPRSgetdblattrib(prob(), XPRS_OBJSENSE, &sense));
      return sense;
    }

    std::string getStringControl(int control) {
      std::string value(280, '\0');
      int valueSize;
      EXPECT_STATUS(XPRSgetstringcontrol(prob(), control, &value[0], value.size(), &valueSize));
      value.resize(valueSize - 1);
      return value;
    }

    double getDoubleControl(int control) {
      double value;
      EXPECT_STATUS(XPRSgetdblcontrol(prob(), control, &value));
      return value;
    }

    int getIntegerControl(int control) {
      int value;
      EXPECT_STATUS(XPRSgetintcontrol(prob(), control, &value));
      return value;
    }

    int getInteger64Control(int control) {
      XPRSint64 value;
      EXPECT_STATUS(XPRSgetintcontrol64(prob(), control, &value));
      return value;
    }

  private:
    MPSolver* solver_;

    XPRSprob prob() {
      return (XPRSprob)solver_->underlying_solver();
    }

    std::string getName(int n, int type) {
      int namelength;
      EXPECT_STATUS(XPRSgetintattrib(prob(), XPRS_NAMELENGTH, &namelength));

      std::string name;
      name.resize(8 * namelength + 1);
      EXPECT_STATUS(XPRSgetnames(prob(), type, name.data(), n, n));

      name.erase(std::find_if(name.rbegin(), name.rend(),
                              [](unsigned char ch) {
                                return !std::isspace(ch) && ch != '\0';
                              })
                     .base(),
                 name.end());

      return name;
    }
  };

#define UNITTEST_INIT_MIP() \
  MPSolver solver("XPRESS_MIP", MPSolver::XPRESS_MIXED_INTEGER_PROGRAMMING);\
  XPRSGetter getter(&solver)
#define UNITTEST_INIT_LP() \
  MPSolver solver("XPRESS_LP", MPSolver::XPRESS_LINEAR_PROGRAMMING);\
  XPRSGetter getter(&solver)

  void _unittest_verify_var(XPRSGetter* getter, MPVariable* x, char type, double lb, double ub) {
    EXPECT_EQ(getter->getVariableType(x->index()), type);
    EXPECT_EQ(getter->getLb(x->index()), lb);
    EXPECT_EQ(getter->getUb(x->index()), ub);
  }

  void _unittest_verify_constraint(XPRSGetter* getter, MPConstraint* c, char type, double lb, double ub) {
    int idx = c->index();
    EXPECT_EQ(getter->getConstraintType(idx), type);
    switch (type) {
      case 'L':
        EXPECT_EQ(getter->getConstraintRhs(idx), ub);
        break;
      case 'U':
        EXPECT_EQ(getter->getConstraintRhs(idx), lb);
        break;
      case 'E':
        EXPECT_EQ(getter->getConstraintRhs(idx), ub);
        EXPECT_EQ(getter->getConstraintRhs(idx), lb);
        break;
      case 'R':
        EXPECT_EQ(getter->getConstraintRhs(idx), ub);
        EXPECT_EQ(getter->getConstraintRange(idx), ub - lb);
        break;
    }
  }

  TEST(XpressInterface, isMIP) {
    UNITTEST_INIT_MIP();
    EXPECT_EQ(solver.IsMIP(), true);
  }

  TEST(XpressInterface, isLP) {
    UNITTEST_INIT_LP();
    EXPECT_EQ(solver.IsMIP(), false);
  }

  TEST(XpressInterface, NumVariables) {
    UNITTEST_INIT_MIP();
    MPVariable* x1 = solver.MakeNumVar(-1., 5.1, "x1");
    MPVariable* x2 = solver.MakeNumVar(3.14, 5.1, "x2");
    std::vector<MPVariable*> xs;
    solver.MakeBoolVarArray(500, "xs", &xs);
    solver.Solve();
    EXPECT_EQ(getter.getNumVariables(), 502);
  }
  
  TEST(XpressInterface, VariablesName) {
    UNITTEST_INIT_MIP();
    solver.MakeRowConstraint(-solver.infinity(), 0);

    std::string pi("Pi");
    std::string secondVar("Name");
    MPVariable* x1 = solver.MakeNumVar(-1., 5.1, pi);
    MPVariable* x2 = solver.MakeNumVar(3.14, 5.1, secondVar);
    solver.Solve();
    EXPECT_EQ(getter.getColName(0), pi);
    EXPECT_EQ(getter.getColName(1), secondVar);
  }

  TEST(XpressInterface, NumConstraints) {
    UNITTEST_INIT_MIP();
    solver.MakeRowConstraint(12., 100.0);
    solver.MakeRowConstraint(13., 13.1);
    solver.MakeRowConstraint(12.1, 1000.0);
    solver.Solve();
    EXPECT_EQ(getter.getNumConstraints(), 3);
  }

  TEST(XpressInterface, ConstraintName) {
    UNITTEST_INIT_MIP();

    std::string phi("Phi");
    std::string otherCnt("constraintName");
    solver.MakeRowConstraint(100.0, 100.0, phi);
    solver.MakeRowConstraint(-solver.infinity(), 13.1, otherCnt);
    solver.Solve();
    EXPECT_EQ(getter.getRowName(0), phi);
    EXPECT_EQ(getter.getRowName(1), otherCnt);
  }

  TEST(XpressInterface, Reset) {
    UNITTEST_INIT_MIP();
    solver.MakeBoolVar("x1");
    solver.MakeBoolVar("x2");
    solver.MakeRowConstraint(12., 100.0);
    solver.Solve();
    EXPECT_EQ(getter.getNumConstraints(), 1);
    EXPECT_EQ(getter.getNumVariables(), 2);
    solver.Reset();
    EXPECT_EQ(getter.getNumConstraints(), 0);
    EXPECT_EQ(getter.getNumVariables(), 0);
  }

  TEST(XpressInterface, MakeIntVar) {
    UNITTEST_INIT_MIP();
    int lb = 0, ub = 10;
    MPVariable* x = solver.MakeIntVar(lb, ub, "x");
    solver.Solve();
    _unittest_verify_var(&getter, x, 'I', lb, ub);
  }

  TEST(XpressInterface, MakeNumVar) {
    UNITTEST_INIT_MIP();
    double lb = 1.5, ub = 158.2;
    MPVariable* x = solver.MakeNumVar(lb, ub, "x");
    solver.Solve();
    _unittest_verify_var(&getter, x, 'C', lb, ub);
  }

  TEST(XpressInterface, MakeBoolVar) {
    UNITTEST_INIT_MIP();
    MPVariable* x = solver.MakeBoolVar("x");
    solver.Solve();
    _unittest_verify_var(&getter, x, 'B', 0, 1);
  }

  TEST(XpressInterface, MakeIntVarArray) {
    UNITTEST_INIT_MIP();
    int n1 = 25, lb1 = -7, ub1 = 18;
    std::vector<MPVariable*> xs1;
    solver.MakeIntVarArray(n1, lb1, ub1, "xs1", &xs1);
    int n2 = 37, lb2 = 19, ub2 = 189;
    std::vector<MPVariable*> xs2;
    solver.MakeIntVarArray(n2, lb2, ub2, "xs2", &xs2);
    solver.Solve();
    for (int i = 0; i < n1; ++i) {
      _unittest_verify_var(&getter, xs1[i], 'I', lb1, ub1);
    }
    for (int i = 0; i < n2; ++i) {
      _unittest_verify_var(&getter, xs2[i], 'I', lb2, ub2);
    }
  }

  TEST(XpressInterface, MakeNumVarArray) {
    UNITTEST_INIT_MIP();
    int n1 = 1;
    double lb1 = 5.1, ub1 = 8.1;
    std::vector<MPVariable*> xs1;
    solver.MakeNumVarArray(n1, lb1, ub1, "xs1", &xs1);
    int n2 = 13;
    double lb2 = -11.5, ub2 = 189.9;
    std::vector<MPVariable*> xs2;
    solver.MakeNumVarArray(n2, lb2, ub2, "xs2", &xs2);
    solver.Solve();
    for (int i = 0; i < n1; ++i) {
      _unittest_verify_var(&getter, xs1[i], 'C', lb1, ub1);
    }
    for (int i = 0; i < n2; ++i) {
      _unittest_verify_var(&getter, xs2[i], 'C', lb2, ub2);
    }
  }

  TEST(XpressInterface, MakeBoolVarArray) {
    UNITTEST_INIT_MIP();
    double n = 43;
    std::vector<MPVariable*> xs;
    solver.MakeBoolVarArray(n, "xs", &xs);
    solver.Solve();
    for (int i = 0; i < n; ++i) {
      _unittest_verify_var(&getter, xs[i], 'B', 0, 1);
    }
  }

  TEST(XpressInterface, SetVariableBounds) {
    UNITTEST_INIT_MIP();
    int lb1 = 3, ub1 = 4;
    MPVariable* x1 = solver.MakeIntVar(lb1, ub1, "x1");
    double lb2 = 3.7, ub2 = 4;
    MPVariable* x2 = solver.MakeNumVar(lb2, ub2, "x2");
    solver.Solve();
    _unittest_verify_var(&getter, x1, 'I', lb1, ub1);
    _unittest_verify_var(&getter, x2, 'C', lb2, ub2);
    lb1 = 12, ub1 = 15;
    x1->SetBounds(lb1, ub1);
    lb2 = -1.1, ub2 = 0;
    x2->SetBounds(lb2, ub2);
    solver.Solve();
    _unittest_verify_var(&getter, x1, 'I', lb1, ub1);
    _unittest_verify_var(&getter, x2, 'C', lb2, ub2);
  }

  TEST(XpressInterface, SetVariableInteger) {
    UNITTEST_INIT_MIP();
    int lb = -1, ub = 7;
    MPVariable* x = solver.MakeIntVar(lb, ub, "x");
    solver.Solve();
    _unittest_verify_var(&getter, x, 'I', lb, ub);
    x->SetInteger(false);
    solver.Solve();
    _unittest_verify_var(&getter, x, 'C', lb, ub);
  }

  TEST(XpressInterface, ConstraintL) {
    UNITTEST_INIT_MIP();
    double lb = -solver.infinity(), ub = 10.;
    MPConstraint* c = solver.MakeRowConstraint(lb, ub);
    solver.Solve();
    _unittest_verify_constraint(&getter, c, 'L', lb, ub);
  }

  TEST(XpressInterface, ConstraintR) {
    UNITTEST_INIT_MIP();
    double lb = -2, ub = -1;
    MPConstraint* c = solver.MakeRowConstraint(lb, ub);
    solver.Solve();
    _unittest_verify_constraint(&getter, c, 'R', lb, ub);
  }

  TEST(XpressInterface, ConstraintG) {
    UNITTEST_INIT_MIP();
    double lb = 8.1, ub = solver.infinity();
    MPConstraint* c = solver.MakeRowConstraint(lb, ub);
    solver.Solve();
    _unittest_verify_constraint(&getter, c, 'G', lb, ub);
  }

  TEST(XpressInterface, ConstraintE) {
    UNITTEST_INIT_MIP();
    double lb = 18947.3, ub = lb;
    MPConstraint* c = solver.MakeRowConstraint(lb, ub);
    solver.Solve();
    _unittest_verify_constraint(&getter, c, 'E', lb, ub);
  }

  TEST(XpressInterface, SetConstraintBoundsL) {
    UNITTEST_INIT_MIP();
    double lb = 18947.3, ub = lb;
    MPConstraint* c = solver.MakeRowConstraint(lb, ub);
    solver.Solve();
    _unittest_verify_constraint(&getter, c, 'E', lb, ub);
    lb = -solver.infinity(), ub = 16.6;
    c->SetBounds(lb, ub);
    solver.Solve();
    _unittest_verify_constraint(&getter, c, 'L', lb, ub);
  }

  TEST(XpressInterface, SetConstraintBoundsR) {
    UNITTEST_INIT_MIP();
    double lb = -solver.infinity(), ub = 15;
    MPConstraint* c = solver.MakeRowConstraint(lb, ub);
    solver.Solve();
    _unittest_verify_constraint(&getter, c, 'L', lb, ub);
    lb = 0, ub = 0.1;
    c->SetBounds(lb, ub);
    solver.Solve();
    _unittest_verify_constraint(&getter, c, 'R', lb, ub);
  }

  TEST(XpressInterface, SetConstraintBoundsG) {
    UNITTEST_INIT_MIP();
    double lb = 1, ub = 2;
    MPConstraint* c = solver.MakeRowConstraint(lb, ub);
    solver.Solve();
    _unittest_verify_constraint(&getter, c, 'R', lb, ub);
    lb = 5, ub = solver.infinity();
    c->SetBounds(lb, ub);
    solver.Solve();
    _unittest_verify_constraint(&getter, c, 'G', lb, ub);
  }

  TEST(XpressInterface, SetConstraintBoundsE) {
    UNITTEST_INIT_MIP();
    double lb = -1, ub = solver.infinity();
    MPConstraint* c = solver.MakeRowConstraint(lb, ub);
    solver.Solve();
    _unittest_verify_constraint(&getter, c, 'G', lb, ub);
    lb = 128, ub = lb;
    c->SetBounds(lb, ub);
    solver.Solve();
    _unittest_verify_constraint(&getter, c, 'E', lb, ub);
  }

  TEST(XpressInterface, ConstraintCoef) {
    UNITTEST_INIT_MIP();
    MPVariable* x1 = solver.MakeBoolVar("x1");
    MPVariable* x2 = solver.MakeBoolVar("x2");
    MPConstraint* c1 = solver.MakeRowConstraint(4.1, solver.infinity());
    MPConstraint* c2 = solver.MakeRowConstraint(-solver.infinity(), 0.1);
    double c11 = -15.6, c12 = 0.4, c21 = -11, c22 = 4.5;
    c1->SetCoefficient(x1, c11);
    c1->SetCoefficient(x2, c12);
    c2->SetCoefficient(x1, c21);
    c2->SetCoefficient(x2, c22);
    solver.Solve();
    EXPECT_EQ(getter.getConstraintCoef(c1->index(), x1->index()), c11);
    EXPECT_EQ(getter.getConstraintCoef(c1->index(), x2->index()), c12);
    EXPECT_EQ(getter.getConstraintCoef(c2->index(), x1->index()), c21);
    EXPECT_EQ(getter.getConstraintCoef(c2->index(), x2->index()), c22);
    c11 = 0.11, c12 = 0.12, c21 = 0.21, c22 = 0.22;
    c1->SetCoefficient(x1, c11);
    c1->SetCoefficient(x2, c12);
    c2->SetCoefficient(x1, c21);
    c2->SetCoefficient(x2, c22);
    solver.Solve();
    EXPECT_EQ(getter.getConstraintCoef(c1->index(), x1->index()), c11);
    EXPECT_EQ(getter.getConstraintCoef(c1->index(), x2->index()), c12);
    EXPECT_EQ(getter.getConstraintCoef(c2->index(), x1->index()), c21);
    EXPECT_EQ(getter.getConstraintCoef(c2->index(), x2->index()), c22);
  }

  TEST(XpressInterface, ClearConstraint) {
    UNITTEST_INIT_MIP();
    MPVariable* x1 = solver.MakeBoolVar("x1");
    MPVariable* x2 = solver.MakeBoolVar("x2");
    MPConstraint* c1 = solver.MakeRowConstraint(4.1, solver.infinity());
    MPConstraint* c2 = solver.MakeRowConstraint(-solver.infinity(), 0.1);
    double c11 = -1533.6, c12 = 3.4, c21 = -11000, c22 = 0.0001;
    c1->SetCoefficient(x1, c11);
    c1->SetCoefficient(x2, c12);
    c2->SetCoefficient(x1, c21);
    c2->SetCoefficient(x2, c22);
    solver.Solve();
    EXPECT_EQ(getter.getConstraintCoef(c1->index(), x1->index()), c11);
    EXPECT_EQ(getter.getConstraintCoef(c1->index(), x2->index()), c12);
    EXPECT_EQ(getter.getConstraintCoef(c2->index(), x1->index()), c21);
    EXPECT_EQ(getter.getConstraintCoef(c2->index(), x2->index()), c22);
    c1->Clear();
    c2->Clear();
    solver.Solve();
    EXPECT_EQ(getter.getConstraintCoef(c1->index(), x1->index()), 0);
    EXPECT_EQ(getter.getConstraintCoef(c1->index(), x2->index()), 0);
    EXPECT_EQ(getter.getConstraintCoef(c2->index(), x1->index()), 0);
    EXPECT_EQ(getter.getConstraintCoef(c2->index(), x2->index()), 0);
  }

  TEST(XpressInterface, ObjectiveCoef) {
    UNITTEST_INIT_MIP();
    MPVariable* x = solver.MakeBoolVar("x");
    MPObjective* obj = solver.MutableObjective();
    double coef = 3112.4;
    obj->SetCoefficient(x, coef);
    solver.Solve();
    EXPECT_EQ(getter.getObjectiveCoef(x->index()), coef);
    coef = 0.2;
    obj->SetCoefficient(x, coef);
    solver.Solve();
    EXPECT_EQ(getter.getObjectiveCoef(x->index()), coef);
  }

  TEST(XpressInterface, ObjectiveOffset) {
    UNITTEST_INIT_MIP();
    MPVariable* x = solver.MakeBoolVar("x");
    MPObjective* obj = solver.MutableObjective();
    double offset = 4.3;
    obj->SetOffset(offset);
    solver.Solve();
    EXPECT_EQ(getter.getObjectiveOffset(), offset);
    offset = 3.6;
    obj->SetOffset(offset);
    solver.Solve();
    EXPECT_EQ(getter.getObjectiveOffset(), offset);
  }

  TEST(XpressInterface, ClearObjective) {
    UNITTEST_INIT_MIP();
    MPVariable* x = solver.MakeBoolVar("x");
    MPObjective* obj = solver.MutableObjective();
    double coef = -15.6;
    obj->SetCoefficient(x, coef);
    solver.Solve();
    EXPECT_EQ(getter.getObjectiveCoef(x->index()), coef);
    obj->Clear();
    solver.Solve();
    EXPECT_EQ(getter.getObjectiveCoef(x->index()), 0);
  }

  TEST(XpressInterface, ObjectiveSense) {
    UNITTEST_INIT_MIP();
    MPObjective* const objective = solver.MutableObjective();
    objective->SetMinimization();
    EXPECT_EQ(getter.getObjectiveSense(), XPRS_OBJ_MINIMIZE);
    objective->SetMaximization();
    EXPECT_EQ(getter.getObjectiveSense(), XPRS_OBJ_MAXIMIZE);
  }

  TEST(XpressInterface, interations) {
    UNITTEST_INIT_LP();
    int nc = 100, nv = 100;
    std::vector<MPConstraint*> cs(nc);
    for (int ci = 0; ci < nc; ++ci) {
      cs[ci] = solver.MakeRowConstraint(ci, ci + 1);
    }
    MPObjective* const objective = solver.MutableObjective();
    for (int vi = 0; vi < nv; ++vi) {
      MPVariable* v = solver.MakeNumVar(0, nv, "x" + std::to_string(vi));
      for (int ci = 0; ci < nc; ++ci) {
        cs[ci]->SetCoefficient(v, vi + ci);
      }
      objective->SetCoefficient(v, 1);
    }
    solver.Solve();
    EXPECT_GT(solver.iterations(), 0);
  }

  TEST(XpressInterface, nodes) {
    UNITTEST_INIT_MIP();
    int nc = 100, nv = 100;
    std::vector<MPConstraint*> cs(nc);
    for (int ci = 0; ci < nc; ++ci) {
      cs[ci] = solver.MakeRowConstraint(ci, ci + 1);
    }
    MPObjective* const objective = solver.MutableObjective();
    for (int vi = 0; vi < nv; ++vi) {
      MPVariable* v = solver.MakeIntVar(0, nv, "x" + std::to_string(vi));
      for (int ci = 0; ci < nc; ++ci) {
        cs[ci]->SetCoefficient(v, vi + ci);
      }
      objective->SetCoefficient(v, 1);
    }
    solver.Solve();
    EXPECT_GT(solver.nodes(), 0);
  }

  TEST(XpressInterface, SolverVersion) {
    UNITTEST_INIT_MIP();
    EXPECT_GE(solver.SolverVersion().size(), 30);
  }

  TEST(XpressInterface, Write) {
    UNITTEST_INIT_MIP();
    MPVariable* x1 = solver.MakeIntVar(-1.2, 9.3, "C1");
    MPVariable* x2 = solver.MakeNumVar(-1, 5.147593849384714, "C2");
    MPConstraint* c1 = solver.MakeRowConstraint(-solver.infinity(), 1, "R1");
    c1->SetCoefficient(x1, 3);
    c1->SetCoefficient(x2, 1.5);
    MPConstraint* c2 = solver.MakeRowConstraint(3, 5, "R2");
    c2->SetCoefficient(x2, -1.1122334455667788);
    MPObjective* obj = solver.MutableObjective();
    obj->SetMaximization();
    obj->SetCoefficient(x1, 1);
    obj->SetCoefficient(x2, 2);

    std::string tmpName = std::string(std::tmpnam(nullptr)) + ".mps";
    solver.Write(tmpName);

    std::ifstream tmpFile(tmpName);
    std::stringstream tmpBuffer;
    tmpBuffer << tmpFile.rdbuf();
    tmpFile.close();
    std::remove(tmpName.c_str());

    EXPECT_EQ(tmpBuffer.str(), R"(NAME          newProb
OBJSENSE  MAXIMIZE
ROWS
 N  __OBJ___
 L  R1      
 L  R2      
COLUMNS
    C1        __OBJ___  1
    C1        R1        3
    C2        __OBJ___  2
    C2        R1        1.5
    C2        R2        -1.1122334455667788
RHS
    RHS00001  R1        1
    RHS00001  R2        5
RANGES
    RNG00001  R2        2
BOUNDS
 UI BND00001  C1        9
 LO BND00001  C1        -1
 UP BND00001  C2        5.147593849384714
 LO BND00001  C2        -1
ENDATA
)");
  }

  TEST(XpressInterface, SetPrimalTolerance) {
    UNITTEST_INIT_LP();
    MPSolverParameters params;
    double tol = 1e-4;
    params.SetDoubleParam(MPSolverParameters::PRIMAL_TOLERANCE, tol);
    solver.Solve(params);
    EXPECT_EQ(getter.getDoubleControl(XPRS_FEASTOL), tol);
  }

  TEST(XpressInterface, SetDualTolerance) {
    UNITTEST_INIT_LP();
    MPSolverParameters params;
    double tol = 1e-2;
    params.SetDoubleParam(MPSolverParameters::DUAL_TOLERANCE, tol);
    solver.Solve(params);
    EXPECT_EQ(getter.getDoubleControl(XPRS_OPTIMALITYTOL), tol);
  }

  TEST(XpressInterface, SetPresolveMode) {
    UNITTEST_INIT_MIP();
    MPSolverParameters params;
    params.SetIntegerParam(MPSolverParameters::PRESOLVE, MPSolverParameters::PRESOLVE_OFF);
    solver.Solve(params);
    EXPECT_EQ(getter.getIntegerControl(XPRS_PRESOLVE), 0);
    params.SetIntegerParam(MPSolverParameters::PRESOLVE, MPSolverParameters::PRESOLVE_ON);
    solver.Solve(params);
    EXPECT_EQ(getter.getIntegerControl(XPRS_PRESOLVE), 1);
  }

  TEST(XpressInterface, SetLpAlgorithm) {
    UNITTEST_INIT_LP();
    MPSolverParameters params;
    params.SetIntegerParam(MPSolverParameters::LP_ALGORITHM, MPSolverParameters::DUAL);
    solver.Solve(params);
    EXPECT_EQ(getter.getIntegerControl(XPRS_DEFAULTALG), 2);
    params.SetIntegerParam(MPSolverParameters::LP_ALGORITHM, MPSolverParameters::PRIMAL);
    solver.Solve(params);
    EXPECT_EQ(getter.getIntegerControl(XPRS_DEFAULTALG), 3);
    params.SetIntegerParam(MPSolverParameters::LP_ALGORITHM, MPSolverParameters::BARRIER);
    solver.Solve(params);
    EXPECT_EQ(getter.getIntegerControl(XPRS_DEFAULTALG), 4);
  }

  TEST(XpressInterface, SetScaling) {
    UNITTEST_INIT_MIP();
    MPSolverParameters params;
    params.SetIntegerParam(MPSolverParameters::SCALING, MPSolverParameters::SCALING_OFF);
    solver.Solve(params);
    EXPECT_EQ(getter.getIntegerControl(XPRS_SCALING), 0);
    params.SetIntegerParam(MPSolverParameters::SCALING, MPSolverParameters::SCALING_ON);
    solver.Solve(params);
    EXPECT_EQ(getter.getIntegerControl(XPRS_SCALING), 163);
  }

  TEST(XpressInterface, SetRelativeMipGap) {
    UNITTEST_INIT_MIP();
    MPSolverParameters params;
    double relativeMipGap = 1e-3;
    params.SetDoubleParam(MPSolverParameters::RELATIVE_MIP_GAP, relativeMipGap);
    solver.Solve(params);
    EXPECT_EQ(getter.getDoubleControl(XPRS_MIPRELSTOP), relativeMipGap);
  }

  TEST(XpressInterface, setStringControls) {
    std::vector<std::tuple<std::string, int, std::string>> params = {
      {"MPSRHSNAME", XPRS_MPSRHSNAME, "default_value"},
      {"MPSOBJNAME", XPRS_MPSOBJNAME, "default_value"},
      {"MPSRANGENAME", XPRS_MPSRANGENAME, "default_value"},
      {"MPSBOUNDNAME", XPRS_MPSBOUNDNAME, "default_value"},
      {"OUTPUTMASK", XPRS_OUTPUTMASK, "default_value"},
      {"TUNERMETHODFILE", XPRS_TUNERMETHODFILE, "default_value"},
      {"TUNEROUTPUTPATH", XPRS_TUNEROUTPUTPATH, "default_value"},
      {"TUNERSESSIONNAME", XPRS_TUNERSESSIONNAME, "default_value"},
      {"COMPUTEEXECSERVICE", XPRS_COMPUTEEXECSERVICE, "default_value"},
    };
    for (const auto& [paramString, control, paramValue] : params) {
      UNITTEST_INIT_MIP();
      std::string xpressParamString = paramString + " " + paramValue;
      solver.SetSolverSpecificParametersAsString(xpressParamString);
      EXPECT_EQ(paramValue, getter.getStringControl(control));
    }
  }

  TEST(XpressInterface, setDoubleControls) {
    std::vector<std::tuple<std::string, int, double>> params = {
      {"MAXCUTTIME", XPRS_MAXCUTTIME, 1.},
      {"MAXSTALLTIME", XPRS_MAXSTALLTIME, 1.},
      {"TUNERMAXTIME", XPRS_TUNERMAXTIME, 1.},
      {"MATRIXTOL", XPRS_MATRIXTOL, 1.},
      {"PIVOTTOL", XPRS_PIVOTTOL, 1.},
      {"FEASTOL", XPRS_FEASTOL, 1.},
      {"OUTPUTTOL", XPRS_OUTPUTTOL, 1.},
      {"SOSREFTOL", XPRS_SOSREFTOL, 1.},
      {"OPTIMALITYTOL", XPRS_OPTIMALITYTOL, 1.},
      {"ETATOL", XPRS_ETATOL, 1.},
      {"RELPIVOTTOL", XPRS_RELPIVOTTOL, 1.},
      {"MIPTOL", XPRS_MIPTOL, 1.},
      {"MIPTOLTARGET", XPRS_MIPTOLTARGET, 1.},
      {"BARPERTURB", XPRS_BARPERTURB, 1.},
      {"MIPADDCUTOFF", XPRS_MIPADDCUTOFF, 1.},
      {"MIPABSCUTOFF", XPRS_MIPABSCUTOFF, 1.},
      {"MIPRELCUTOFF", XPRS_MIPRELCUTOFF, 1.},
      {"PSEUDOCOST", XPRS_PSEUDOCOST, 1.},
      {"PENALTY", XPRS_PENALTY, 1.},
      {"BIGM", XPRS_BIGM, 1.},
      {"MIPABSSTOP", XPRS_MIPABSSTOP, 1.},
      {"MIPRELSTOP", XPRS_MIPRELSTOP, 1.},
      {"CROSSOVERACCURACYTOL", XPRS_CROSSOVERACCURACYTOL, 1.},
      {"PRIMALPERTURB", XPRS_PRIMALPERTURB, 1.},
      {"DUALPERTURB", XPRS_DUALPERTURB, 1.},
      {"BAROBJSCALE", XPRS_BAROBJSCALE, 1.},
      {"BARRHSSCALE", XPRS_BARRHSSCALE, 1.},
      {"CHOLESKYTOL", XPRS_CHOLESKYTOL, 1.},
      {"BARGAPSTOP", XPRS_BARGAPSTOP, 1.},
      {"BARDUALSTOP", XPRS_BARDUALSTOP, 1.},
      {"BARPRIMALSTOP", XPRS_BARPRIMALSTOP, 1.},
      {"BARSTEPSTOP", XPRS_BARSTEPSTOP, 1.},
      {"ELIMTOL", XPRS_ELIMTOL, 1.},
      {"MARKOWITZTOL", XPRS_MARKOWITZTOL, 1.},
      {"MIPABSGAPNOTIFY", XPRS_MIPABSGAPNOTIFY, 1.},
      {"MIPRELGAPNOTIFY", XPRS_MIPRELGAPNOTIFY, 1.},
      {"BARLARGEBOUND", XPRS_BARLARGEBOUND, 1.},
      {"PPFACTOR", XPRS_PPFACTOR, 1.},
      {"REPAIRINDEFINITEQMAX", XPRS_REPAIRINDEFINITEQMAX, 1.},
      {"BARGAPTARGET", XPRS_BARGAPTARGET, 1.},
      {"DUMMYCONTROL", XPRS_DUMMYCONTROL, 1.},
      {"BARSTARTWEIGHT", XPRS_BARSTARTWEIGHT, 1.},
      {"BARFREESCALE", XPRS_BARFREESCALE, 1.},
      {"SBEFFORT", XPRS_SBEFFORT, 1.},
      {"HEURDIVERANDOMIZE", XPRS_HEURDIVERANDOMIZE, 1.},
      {"HEURSEARCHEFFORT", XPRS_HEURSEARCHEFFORT, 1.},
      {"CUTFACTOR", XPRS_CUTFACTOR, 1.},
      {"EIGENVALUETOL", XPRS_EIGENVALUETOL, 1.},
      {"INDLINBIGM", XPRS_INDLINBIGM, 1.},
      {"TREEMEMORYSAVINGTARGET", XPRS_TREEMEMORYSAVINGTARGET, 1.},
      {"INDPRELINBIGM", XPRS_INDPRELINBIGM, 1.},
      {"RELAXTREEMEMORYLIMIT", XPRS_RELAXTREEMEMORYLIMIT, 1.},
      {"MIPABSGAPNOTIFYOBJ", XPRS_MIPABSGAPNOTIFYOBJ, 1.},
      {"MIPABSGAPNOTIFYBOUND", XPRS_MIPABSGAPNOTIFYBOUND, 1.},
      {"PRESOLVEMAXGROW", XPRS_PRESOLVEMAXGROW, 1.},
      {"HEURSEARCHTARGETSIZE", XPRS_HEURSEARCHTARGETSIZE, 1.},
      {"CROSSOVERRELPIVOTTOL", XPRS_CROSSOVERRELPIVOTTOL, 1.},
      {"CROSSOVERRELPIVOTTOLSAFE", XPRS_CROSSOVERRELPIVOTTOLSAFE, 1.},
      {"DETLOGFREQ", XPRS_DETLOGFREQ, 1.},
      {"MAXIMPLIEDBOUND", XPRS_MAXIMPLIEDBOUND, 1.},
      {"FEASTOLTARGET", XPRS_FEASTOLTARGET, 1.},
      {"OPTIMALITYTOLTARGET", XPRS_OPTIMALITYTOLTARGET, 1.},
      {"PRECOMPONENTSEFFORT", XPRS_PRECOMPONENTSEFFORT, 1.},
      {"LPLOGDELAY", XPRS_LPLOGDELAY, 1.},
      {"HEURDIVEITERLIMIT", XPRS_HEURDIVEITERLIMIT, 1.},
      {"BARKERNEL", XPRS_BARKERNEL, 1.},
      {"FEASTOLPERTURB", XPRS_FEASTOLPERTURB, 1.},
      {"CROSSOVERFEASWEIGHT", XPRS_CROSSOVERFEASWEIGHT, 1.},
      {"LUPIVOTTOL", XPRS_LUPIVOTTOL, 1.},
      {"MIPRESTARTGAPTHRESHOLD", XPRS_MIPRESTARTGAPTHRESHOLD, 1.},
      {"NODEPROBINGEFFORT", XPRS_NODEPROBINGEFFORT, 1.},
      {"INPUTTOL", XPRS_INPUTTOL, 1.},
      {"MIPRESTARTFACTOR", XPRS_MIPRESTARTFACTOR, 1.},
      {"BAROBJPERTURB", XPRS_BAROBJPERTURB, 1.},
      {"CPIALPHA", XPRS_CPIALPHA, 1.},
      {"GLOBALBOUNDINGBOX", XPRS_GLOBALBOUNDINGBOX, 1.},
      {"TIMELIMIT", XPRS_TIMELIMIT, 1.},
      {"SOLTIMELIMIT", XPRS_SOLTIMELIMIT, 1.},
      {"REPAIRINFEASTIMELIMIT", XPRS_REPAIRINFEASTIMELIMIT, 1.},
    };
    for (const auto& [paramString, control, paramValue] : params) {
      UNITTEST_INIT_MIP();
      std::string xpressParamString = paramString + " " + std::to_string(paramValue);
      solver.SetSolverSpecificParametersAsString(xpressParamString);
      EXPECT_EQ(paramValue, getter.getDoubleControl(control));
    }
  }

  TEST(XpressInterface, setIntControl) {
    std::vector<std::tuple<std::string, int, int>> params = {
      {"EXTRAROWS", XPRS_EXTRAROWS, 1},
      {"EXTRACOLS", XPRS_EXTRACOLS, 1},
      {"LPITERLIMIT", XPRS_LPITERLIMIT, 1},
      {"LPLOG", XPRS_LPLOG, 1},
      {"SCALING", XPRS_SCALING, 1},
      {"PRESOLVE", XPRS_PRESOLVE, 1},
      {"CRASH", XPRS_CRASH, 1},
      {"PRICINGALG", XPRS_PRICINGALG, 1},
      {"INVERTFREQ", XPRS_INVERTFREQ, 1},
      {"INVERTMIN", XPRS_INVERTMIN, 1},
      {"MAXNODE", XPRS_MAXNODE, 1},
      {"MAXTIME", XPRS_MAXTIME, 1},
      {"MAXMIPSOL", XPRS_MAXMIPSOL, 1},
      {"SIFTPASSES", XPRS_SIFTPASSES, 1},
      {"DEFAULTALG", XPRS_DEFAULTALG, 1},
      {"VARSELECTION", XPRS_VARSELECTION, 1},
      {"NODESELECTION", XPRS_NODESELECTION, 1},
      {"BACKTRACK", XPRS_BACKTRACK, 1},
      {"MIPLOG", XPRS_MIPLOG, 1},
      {"KEEPNROWS", XPRS_KEEPNROWS, 1},
      {"MPSECHO", XPRS_MPSECHO, 1},
      {"MAXPAGELINES", XPRS_MAXPAGELINES, 1},
      {"OUTPUTLOG", XPRS_OUTPUTLOG, 1},
      {"BARSOLUTION", XPRS_BARSOLUTION, 1},
      {"CACHESIZE", XPRS_CACHESIZE, 1},
      {"CROSSOVER", XPRS_CROSSOVER, 1},
      {"BARITERLIMIT", XPRS_BARITERLIMIT, 1},
      {"CHOLESKYALG", XPRS_CHOLESKYALG, 1},
      {"BAROUTPUT", XPRS_BAROUTPUT, 1},
      {"EXTRAMIPENTS", XPRS_EXTRAMIPENTS, 1},
      {"REFACTOR", XPRS_REFACTOR, 1},
      {"BARTHREADS", XPRS_BARTHREADS, 1},
      {"KEEPBASIS", XPRS_KEEPBASIS, 1},
      {"CROSSOVEROPS", XPRS_CROSSOVEROPS, 1},
      {"VERSION", XPRS_VERSION, 1},
      {"CROSSOVERTHREADS", XPRS_CROSSOVERTHREADS, 1},
      {"BIGMMETHOD", XPRS_BIGMMETHOD, 1},
      {"MPSNAMELENGTH", XPRS_MPSNAMELENGTH, 1},
      {"ELIMFILLIN", XPRS_ELIMFILLIN, 1},
      {"PRESOLVEOPS", XPRS_PRESOLVEOPS, 1},
      {"MIPPRESOLVE", XPRS_MIPPRESOLVE, 1},
      {"MIPTHREADS", XPRS_MIPTHREADS, 1},
      {"BARORDER", XPRS_BARORDER, 1},
      {"BREADTHFIRST", XPRS_BREADTHFIRST, 1},
      {"AUTOPERTURB", XPRS_AUTOPERTURB, 1},
      {"DENSECOLLIMIT", XPRS_DENSECOLLIMIT, 1},
      {"CALLBACKFROMMASTERTHREAD", XPRS_CALLBACKFROMMASTERTHREAD, 1},
      {"MAXMCOEFFBUFFERELEMS", XPRS_MAXMCOEFFBUFFERELEMS, 1},
      {"REFINEOPS", XPRS_REFINEOPS, 1},
      {"LPREFINEITERLIMIT", XPRS_LPREFINEITERLIMIT, 1},
      {"MIPREFINEITERLIMIT", XPRS_MIPREFINEITERLIMIT, 1},
      {"DUALIZEOPS", XPRS_DUALIZEOPS, 1},
      {"CROSSOVERITERLIMIT", XPRS_CROSSOVERITERLIMIT, 1},
      {"PREBASISRED", XPRS_PREBASISRED, 1},
      {"PRESORT", XPRS_PRESORT, 1},
      {"PREPERMUTE", XPRS_PREPERMUTE, 1},
      {"PREPERMUTESEED", XPRS_PREPERMUTESEED, 1},
      {"MAXMEMORYSOFT", XPRS_MAXMEMORYSOFT, 1},
      {"CUTFREQ", XPRS_CUTFREQ, 1},
      {"SYMSELECT", XPRS_SYMSELECT, 1},
      {"SYMMETRY", XPRS_SYMMETRY, 1},
      {"MAXMEMORYHARD", XPRS_MAXMEMORYHARD, 1},
      {"MIQCPALG", XPRS_MIQCPALG, 1},
      {"QCCUTS", XPRS_QCCUTS, 1},
      {"QCROOTALG", XPRS_QCROOTALG, 1},
      {"PRECONVERTSEPARABLE", XPRS_PRECONVERTSEPARABLE, 1},
      {"ALGAFTERNETWORK", XPRS_ALGAFTERNETWORK, 1},
      {"TRACE", XPRS_TRACE, 1},
      {"MAXIIS", XPRS_MAXIIS, 1},
      {"CPUTIME", XPRS_CPUTIME, 1},
      {"COVERCUTS", XPRS_COVERCUTS, 1},
      {"GOMCUTS", XPRS_GOMCUTS, 1},
      {"LPFOLDING", XPRS_LPFOLDING, 1},
      {"MPSFORMAT", XPRS_MPSFORMAT, 1},
      {"CUTSTRATEGY", XPRS_CUTSTRATEGY, 1},
      {"CUTDEPTH", XPRS_CUTDEPTH, 1},
      {"TREECOVERCUTS", XPRS_TREECOVERCUTS, 1},
      {"TREEGOMCUTS", XPRS_TREEGOMCUTS, 1},
      {"CUTSELECT", XPRS_CUTSELECT, 1},
      {"TREECUTSELECT", XPRS_TREECUTSELECT, 1},
      {"DUALIZE", XPRS_DUALIZE, 1},
      {"DUALGRADIENT", XPRS_DUALGRADIENT, 1},
      {"SBITERLIMIT", XPRS_SBITERLIMIT, 1},
      {"SBBEST", XPRS_SBBEST, 1},
      {"BARINDEFLIMIT", XPRS_BARINDEFLIMIT, 1},
      {"HEURFREQ", XPRS_HEURFREQ, 1},
      {"HEURDEPTH", XPRS_HEURDEPTH, 1},
      {"HEURMAXSOL", XPRS_HEURMAXSOL, 1},
      {"HEURNODES", XPRS_HEURNODES, 1},
      {"LNPBEST", XPRS_LNPBEST, 1},
      {"LNPITERLIMIT", XPRS_LNPITERLIMIT, 1},
      {"BRANCHCHOICE", XPRS_BRANCHCHOICE, 1},
      {"BARREGULARIZE", XPRS_BARREGULARIZE, 1},
      {"SBSELECT", XPRS_SBSELECT, 1},
      {"LOCALCHOICE", XPRS_LOCALCHOICE, 1},
      {"LOCALBACKTRACK", XPRS_LOCALBACKTRACK, 1},
      {"DUALSTRATEGY", XPRS_DUALSTRATEGY, 1},
      {"L1CACHE", XPRS_L1CACHE, 1},
      {"HEURDIVESTRATEGY", XPRS_HEURDIVESTRATEGY, 1},
      {"HEURSELECT", XPRS_HEURSELECT, 1},
      {"BARSTART", XPRS_BARSTART, 1},
      {"PRESOLVEPASSES", XPRS_PRESOLVEPASSES, 1},
      {"BARNUMSTABILITY", XPRS_BARNUMSTABILITY, 1},
      {"BARORDERTHREADS", XPRS_BARORDERTHREADS, 1},
      {"EXTRASETS", XPRS_EXTRASETS, 1},
      {"FEASIBILITYPUMP", XPRS_FEASIBILITYPUMP, 1},
      {"PRECOEFELIM", XPRS_PRECOEFELIM, 1},
      {"PREDOMCOL", XPRS_PREDOMCOL, 1},
      {"HEURSEARCHFREQ", XPRS_HEURSEARCHFREQ, 1},
      {"HEURDIVESPEEDUP", XPRS_HEURDIVESPEEDUP, 1},
      {"SBESTIMATE", XPRS_SBESTIMATE, 1},
      {"BARCORES", XPRS_BARCORES, 1},
      {"MAXCHECKSONMAXTIME", XPRS_MAXCHECKSONMAXTIME, 1},
      {"MAXCHECKSONMAXCUTTIME", XPRS_MAXCHECKSONMAXCUTTIME, 1},
      {"HISTORYCOSTS", XPRS_HISTORYCOSTS, 1},
      {"ALGAFTERCROSSOVER", XPRS_ALGAFTERCROSSOVER, 1},
      {"MUTEXCALLBACKS", XPRS_MUTEXCALLBACKS, 1},
      {"BARCRASH", XPRS_BARCRASH, 1},
      {"HEURDIVESOFTROUNDING", XPRS_HEURDIVESOFTROUNDING, 1},
      {"HEURSEARCHROOTSELECT", XPRS_HEURSEARCHROOTSELECT, 1},
      {"HEURSEARCHTREESELECT", XPRS_HEURSEARCHTREESELECT, 1},
      {"MPS18COMPATIBLE", XPRS_MPS18COMPATIBLE, 1},
      {"ROOTPRESOLVE", XPRS_ROOTPRESOLVE, 1},
      {"CROSSOVERDRP", XPRS_CROSSOVERDRP, 1},
      {"FORCEOUTPUT", XPRS_FORCEOUTPUT, 1},
      {"PRIMALOPS", XPRS_PRIMALOPS, 1},
      {"DETERMINISTIC", XPRS_DETERMINISTIC, 1},
      {"PREPROBING", XPRS_PREPROBING, 1},
      {"TREEMEMORYLIMIT", XPRS_TREEMEMORYLIMIT, 1},
      {"TREECOMPRESSION", XPRS_TREECOMPRESSION, 1},
      {"TREEDIAGNOSTICS", XPRS_TREEDIAGNOSTICS, 1},
      {"MAXTREEFILESIZE", XPRS_MAXTREEFILESIZE, 1},
      {"PRECLIQUESTRATEGY", XPRS_PRECLIQUESTRATEGY, 1},
      {"REPAIRINFEASMAXTIME", XPRS_REPAIRINFEASMAXTIME, 1},
      {"IFCHECKCONVEXITY", XPRS_IFCHECKCONVEXITY, 1},
      {"PRIMALUNSHIFT", XPRS_PRIMALUNSHIFT, 1},
      {"REPAIRINDEFINITEQ", XPRS_REPAIRINDEFINITEQ, 1},
      {"MIPRAMPUP", XPRS_MIPRAMPUP, 1},
      {"MAXLOCALBACKTRACK", XPRS_MAXLOCALBACKTRACK, 1},
      {"USERSOLHEURISTIC", XPRS_USERSOLHEURISTIC, 1},
      {"FORCEPARALLELDUAL", XPRS_FORCEPARALLELDUAL, 1},
      {"BACKTRACKTIE", XPRS_BACKTRACKTIE, 1},
      {"BRANCHDISJ", XPRS_BRANCHDISJ, 1},
      {"MIPFRACREDUCE", XPRS_MIPFRACREDUCE, 1},
      {"CONCURRENTTHREADS", XPRS_CONCURRENTTHREADS, 1},
      {"MAXSCALEFACTOR", XPRS_MAXSCALEFACTOR, 1},
      {"HEURTHREADS", XPRS_HEURTHREADS, 1},
      {"THREADS", XPRS_THREADS, 1},
      {"HEURBEFORELP", XPRS_HEURBEFORELP, 1},
      {"PREDOMROW", XPRS_PREDOMROW, 1},
      {"BRANCHSTRUCTURAL", XPRS_BRANCHSTRUCTURAL, 1},
      {"QUADRATICUNSHIFT", XPRS_QUADRATICUNSHIFT, 1},
      {"BARPRESOLVEOPS", XPRS_BARPRESOLVEOPS, 1},
      {"QSIMPLEXOPS", XPRS_QSIMPLEXOPS, 1},
      {"MIPRESTART", XPRS_MIPRESTART, 1},
      {"CONFLICTCUTS", XPRS_CONFLICTCUTS, 1},
      {"PREPROTECTDUAL", XPRS_PREPROTECTDUAL, 1},
      {"CORESPERCPU", XPRS_CORESPERCPU, 1},
      {"RESOURCESTRATEGY", XPRS_RESOURCESTRATEGY, 1},
      {"CLAMPING", XPRS_CLAMPING, 1},
      {"SLEEPONTHREADWAIT", XPRS_SLEEPONTHREADWAIT, 1},
      {"PREDUPROW", XPRS_PREDUPROW, 1},
      {"CPUPLATFORM", XPRS_CPUPLATFORM, 1},
      {"BARALG", XPRS_BARALG, 1},
      {"SIFTING", XPRS_SIFTING, 1},
      {"LPLOGSTYLE", XPRS_LPLOGSTYLE, 1},
      {"RANDOMSEED", XPRS_RANDOMSEED, 1},
      {"TREEQCCUTS", XPRS_TREEQCCUTS, 1},
      {"PRELINDEP", XPRS_PRELINDEP, 1},
      {"DUALTHREADS", XPRS_DUALTHREADS, 1},
      {"PREOBJCUTDETECT", XPRS_PREOBJCUTDETECT, 1},
      {"PREBNDREDQUAD", XPRS_PREBNDREDQUAD, 1},
      {"PREBNDREDCONE", XPRS_PREBNDREDCONE, 1},
      {"PRECOMPONENTS", XPRS_PRECOMPONENTS, 1},
      {"MAXMIPTASKS", XPRS_MAXMIPTASKS, 1},
      {"MIPTERMINATIONMETHOD", XPRS_MIPTERMINATIONMETHOD, 1},
      {"PRECONEDECOMP", XPRS_PRECONEDECOMP, 1},
      {"HEURFORCESPECIALOBJ", XPRS_HEURFORCESPECIALOBJ, 1},
      {"HEURSEARCHROOTCUTFREQ", XPRS_HEURSEARCHROOTCUTFREQ, 1},
      {"PREELIMQUAD", XPRS_PREELIMQUAD, 1},
      {"PREIMPLICATIONS", XPRS_PREIMPLICATIONS, 1},
      {"TUNERMODE", XPRS_TUNERMODE, 1},
      {"TUNERMETHOD", XPRS_TUNERMETHOD, 1},
      {"TUNERTARGET", XPRS_TUNERTARGET, 1},
      {"TUNERTHREADS", XPRS_TUNERTHREADS, 1},
      {"TUNERHISTORY", XPRS_TUNERHISTORY, 1},
      {"TUNERPERMUTE", XPRS_TUNERPERMUTE, 1},
      {"TUNERVERBOSE", XPRS_TUNERVERBOSE, 1},
      {"TUNEROUTPUT", XPRS_TUNEROUTPUT, 1},
      {"PREANALYTICCENTER", XPRS_PREANALYTICCENTER, 1},
      {"NETCUTS", XPRS_NETCUTS, 1},
      {"LPFLAGS", XPRS_LPFLAGS, 1},
      {"MIPKAPPAFREQ", XPRS_MIPKAPPAFREQ, 1},
      {"OBJSCALEFACTOR", XPRS_OBJSCALEFACTOR, 1},
      {"TREEFILELOGINTERVAL", XPRS_TREEFILELOGINTERVAL, 1},
      {"IGNORECONTAINERCPULIMIT", XPRS_IGNORECONTAINERCPULIMIT, 1},
      {"IGNORECONTAINERMEMORYLIMIT", XPRS_IGNORECONTAINERMEMORYLIMIT, 1},
      {"MIPDUALREDUCTIONS", XPRS_MIPDUALREDUCTIONS, 1},
      {"GENCONSDUALREDUCTIONS", XPRS_GENCONSDUALREDUCTIONS, 1},
      {"PWLDUALREDUCTIONS", XPRS_PWLDUALREDUCTIONS, 1},
      {"BARFAILITERLIMIT", XPRS_BARFAILITERLIMIT, 1},
      {"AUTOSCALING", XPRS_AUTOSCALING, 1},
      {"GENCONSABSTRANSFORMATION", XPRS_GENCONSABSTRANSFORMATION, 1},
      {"COMPUTEJOBPRIORITY", XPRS_COMPUTEJOBPRIORITY, 1},
      {"PREFOLDING", XPRS_PREFOLDING, 1},
      {"NETSTALLLIMIT", XPRS_NETSTALLLIMIT, 1},
      {"SERIALIZEPREINTSOL", XPRS_SERIALIZEPREINTSOL, 1},
      {"NUMERICALEMPHASIS", XPRS_NUMERICALEMPHASIS, 1},
      {"PWLNONCONVEXTRANSFORMATION", XPRS_PWLNONCONVEXTRANSFORMATION, 1},
      {"MIPCOMPONENTS", XPRS_MIPCOMPONENTS, 1},
      {"MIPCONCURRENTNODES", XPRS_MIPCONCURRENTNODES, 1},
      {"MIPCONCURRENTSOLVES", XPRS_MIPCONCURRENTSOLVES, 1},
      {"OUTPUTCONTROLS", XPRS_OUTPUTCONTROLS, 1},
      {"SIFTSWITCH", XPRS_SIFTSWITCH, 1},
      {"HEUREMPHASIS", XPRS_HEUREMPHASIS, 1},
      {"COMPUTEMATX", XPRS_COMPUTEMATX, 1},
      {"COMPUTEMATX_IIS", XPRS_COMPUTEMATX_IIS, 1},
      {"COMPUTEMATX_IISMAXTIME", XPRS_COMPUTEMATX_IISMAXTIME, 1},
      {"BARREFITER", XPRS_BARREFITER, 1},
      {"COMPUTELOG", XPRS_COMPUTELOG, 1},
      {"SIFTPRESOLVEOPS", XPRS_SIFTPRESOLVEOPS, 1},
      {"CHECKINPUTDATA", XPRS_CHECKINPUTDATA, 1},
      {"ESCAPENAMES", XPRS_ESCAPENAMES, 1},
      {"IOTIMEOUT", XPRS_IOTIMEOUT, 1},
      {"AUTOCUTTING", XPRS_AUTOCUTTING, 1},
      {"CALLBACKCHECKTIMEDELAY", XPRS_CALLBACKCHECKTIMEDELAY, 1},
      {"MULTIOBJOPS", XPRS_MULTIOBJOPS, 1},
      {"MULTIOBJLOG", XPRS_MULTIOBJLOG, 1},
      {"GLOBALSPATIALBRANCHIFPREFERORIG", XPRS_GLOBALSPATIALBRANCHIFPREFERORIG, 1},
      {"PRECONFIGURATION", XPRS_PRECONFIGURATION, 1},
      {"FEASIBILITYJUMP", XPRS_FEASIBILITYJUMP, 1},
    };
    for (const auto& [paramString, control, paramValue] : params) {
      UNITTEST_INIT_MIP();
      std::string xpressParamString = paramString + " " + std::to_string(paramValue);
      solver.SetSolverSpecificParametersAsString(xpressParamString);
      EXPECT_EQ(paramValue, getter.getIntegerControl(control));
    }
  }

  TEST(XpressInterface, setInt64Control) {
    std::vector<std::tuple<std::string, int, int>> params = {
      {"EXTRAELEMS", XPRS_EXTRAELEMS, 1},
      {"EXTRASETELEMS", XPRS_EXTRASETELEMS, 1},
    };
    for (const auto& [paramString, control, paramValue] : params) {
      UNITTEST_INIT_MIP();
      std::string xpressParamString = paramString + " " + std::to_string(paramValue);
      solver.SetSolverSpecificParametersAsString(xpressParamString);
      EXPECT_EQ(paramValue, getter.getInteger64Control(control));
    }
  }

  TEST(XpressInterface, SolveMIP) {
    UNITTEST_INIT_MIP();

    // max   x + 2y
    // st.  -x +  y <= 1
    //      2x + 3y <= 12
    //      3x + 2y <= 12
    //       x ,  y >= 0
    //       x ,  y \in Z

    double inf = solver.infinity();
    MPVariable* x = solver.MakeIntVar(0, inf, "x");
    MPVariable* y = solver.MakeIntVar(0, inf, "y");
    MPObjective* obj = solver.MutableObjective();
    obj->SetCoefficient(x, 1);
    obj->SetCoefficient(y, 2);
    obj->SetMaximization();
    MPConstraint* c1 = solver.MakeRowConstraint(-inf, 1);
    c1->SetCoefficient(x, -1);
    c1->SetCoefficient(y, 1);
    MPConstraint* c2 = solver.MakeRowConstraint(-inf, 12);
    c2->SetCoefficient(x, 3);
    c2->SetCoefficient(y, 2);
    MPConstraint* c3 = solver.MakeRowConstraint(-inf, 12);
    c3->SetCoefficient(x, 2);
    c3->SetCoefficient(y, 3);
    solver.Solve();

    EXPECT_EQ(obj->Value(), 6);
    EXPECT_EQ(obj->BestBound(), 6);
    EXPECT_EQ(x->solution_value(), 2);
    EXPECT_EQ(y->solution_value(), 2);
  }

  TEST(XpressInterface, SolveLP) {
    UNITTEST_INIT_LP();

    // max   x + 2y
    // st.  -x +  y <= 1
    //      2x + 3y <= 12
    //      3x + 2y <= 12
    //       x ,  y \in R+

    double inf = solver.infinity();
    MPVariable* x = solver.MakeNumVar(0, inf, "x");
    MPVariable* y = solver.MakeNumVar(0, inf, "y");
    MPObjective* obj = solver.MutableObjective();
    obj->SetCoefficient(x, 1);
    obj->SetCoefficient(y, 2);
    obj->SetMaximization();
    MPConstraint* c1 = solver.MakeRowConstraint(-inf, 1);
    c1->SetCoefficient(x, -1);
    c1->SetCoefficient(y, 1);
    MPConstraint* c2 = solver.MakeRowConstraint(-inf, 12);
    c2->SetCoefficient(x, 3);
    c2->SetCoefficient(y, 2);
    MPConstraint* c3 = solver.MakeRowConstraint(-inf, 12);
    c3->SetCoefficient(x, 2);
    c3->SetCoefficient(y, 3);
    solver.Solve();

    EXPECT_NEAR(obj->Value(), 7.4, 1e-8);
    EXPECT_NEAR(x->solution_value(), 1.8, 1e-8);
    EXPECT_NEAR(y->solution_value(), 2.8, 1e-8);
    EXPECT_NEAR(x->reduced_cost(), 0, 1e-8);
    EXPECT_NEAR(y->reduced_cost(), 0, 1e-8);
    EXPECT_NEAR(c1->dual_value(), 0.2, 1e-8);
    EXPECT_NEAR(c2->dual_value(), 0, 1e-8);
    EXPECT_NEAR(c3->dual_value(), 0.6, 1e-8);
  }

  TEST(XpressInterface, SetHint) {
    // Once a solution is added to XPRESS, it is actually impossible to get it
    // back using the API
    // This test is a simple one that adds a hint and makes sure everything is OK
    // (it is a copy of the previous one but with integer variables)
    // The logs should contain line "User solution (USER_HINT) stored."
    UNITTEST_INIT_MIP();

    double inf = solver.infinity();
    MPVariable* x = solver.MakeIntVar(0, inf, "x");
    MPVariable* y = solver.MakeIntVar(0, inf, "y");
    MPObjective* obj = solver.MutableObjective();
    obj->SetCoefficient(x, 1);
    obj->SetCoefficient(y, 2);
    obj->SetMaximization();
    MPConstraint* c1 = solver.MakeRowConstraint(-inf, 1);
    c1->SetCoefficient(x, -1);
    c1->SetCoefficient(y, 1);
    MPConstraint* c2 = solver.MakeRowConstraint(-inf, 12);
    c2->SetCoefficient(x, 3);
    c2->SetCoefficient(y, 2);
    MPConstraint* c3 = solver.MakeRowConstraint(-inf, 12);
    c3->SetCoefficient(x, 2);
    c3->SetCoefficient(y, 3);

    std::vector<std::pair<const MPVariable*, double>> hint{{{x, 1.}, {y, 1.}}};
    solver.SetHint(hint);

    solver.EnableOutput();
    solver.Solve();

    EXPECT_NEAR(obj->Value(), 6.0, 1e-8);
    EXPECT_NEAR(x->solution_value(), 2.0, 1e-8);
    EXPECT_NEAR(y->solution_value(), 2.0, 1e-8);
  }

}  // namespace operations_research

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  absl::SetFlag(&FLAGS_logtostderr, 1);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
