#include <scai/lama.hpp>
#include <scai/lama/DenseVector.hpp>

#include "ABS2D.hpp"
#include "gtest/gtest.h"

using namespace scai;
using namespace KITGPI;

TEST(ABS2DTest, TestApplyThrows)
{
    int N = 10;
    double testValue = 123.0;
    lama::DenseVector<double> testVector;
    testVector.allocate(N);
    testVector.assign(testValue);
    
    ForwardSolver::BoundaryCondition::ABS2D<double> test;
    
    EXPECT_ANY_THROW(test.apply(testVector, testVector, testVector));
    EXPECT_ANY_THROW(test.apply(testVector, testVector, testVector, testVector, testVector));
    
}
