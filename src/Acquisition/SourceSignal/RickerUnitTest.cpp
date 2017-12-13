#include <math.h>
#include <scai/lama.hpp>
#include <scai/lama/DenseVector.hpp>

#include "Ricker.hpp"
#include "gtest/gtest.h"

using namespace scai;
using namespace KITGPI;

TEST(RickerTest, TestConstructor)
{
    int NT = 4;
    double DT = 1;
    double FC = 1;
    double AMP = 1;
    double Tshift = 0;

    lama::DenseVector<double> sampleResult;
    sampleResult.allocate(NT);

    //calculate sample result
    lama::DenseVector<double> t;
    t.setRange(NT, double(0), DT);
    lama::DenseVector<double> help(t.size(), 1.5 / FC + Tshift);
    lama::DenseVector<double> tau(t - help);
    tau *= M_PI * FC;
    lama::DenseVector<double> one(sampleResult.size(), 1.0);
    help = tau * tau;
    tau = -1.0 * help;
    tau.exp();
    help = one - 2.0 * help;
    sampleResult = lama::Scalar(AMP) * help * tau;

    //Testing
    lama::DenseVector<double> testResult1;
    testResult1.allocate(NT);
    EXPECT_NO_THROW(Acquisition::SourceSignal::Ricker<double>(testResult1, NT, DT, FC, AMP, Tshift));

    lama::DenseVector<double> testResult2;
    testResult2.allocate(NT);
    Acquisition::SourceSignal::Ricker<double>(testResult2, NT, DT, FC, AMP, Tshift);

    EXPECT_EQ(sampleResult.getValue(3), testResult2.getValue(3));
}

TEST(RickerTest, TestAsserts)
{
    int NT = 4;
    double DT = 10;
    double FC = 1;
    double AMP = 1;
    double Tshift = 0;
    lama::DenseVector<double> testResult1;
    testResult1.allocate(NT);

    EXPECT_ANY_THROW(Acquisition::SourceSignal::Ricker<double>(testResult1, -NT, DT, FC, AMP, Tshift));
    EXPECT_ANY_THROW(Acquisition::SourceSignal::Ricker<double>(testResult1, NT, -DT, FC, AMP, Tshift));
    EXPECT_ANY_THROW(Acquisition::SourceSignal::Ricker<double>(testResult1, NT, DT, -FC, AMP, Tshift));
}
