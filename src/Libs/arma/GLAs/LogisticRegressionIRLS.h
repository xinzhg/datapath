#include <armadillo>
#include <math.h>
#include <boost/math/distributions.hpp>

#define ADD(x) x += other.x;

/*
 *  This is an implementation of Logistic Regression based heavily on the
 *  MADlib implementation.
 *
 *  This implementation uses the iteratively-reweighted least-squares method.
 */

/** Information for Meta-GLAs
 *  GLA_DESC
 *
 *  NAME(</LogisticRegressionIRLS/>)
 *  INPUTS(</(x, VECTOR), (y, DOUBLE)/>)
 *  OUTPUTS(</(count, BIGINT)/>)
 *  CONSTRUCTOR(</(width, BIGINT)/>)
 *  RESULT_TYPE(</multi/>)
 *
 *  OPT_ITERABLE
 *
 *  LIBS(armadillo)
 *
 *  END_DESC
 */

using namespace arma;

typedef Col<DOUBLE> VECTOR;
typedef Mat<DOUBLE> MATRIX;

// Declaration
class LogisticRegressionIRLS;

class LogisticRegressionIRLS_ConstState {
    // Inter-iteration components
    VECTOR coef;
    uint64_t iteration;

    // Constructor arguments
    BIGINT width;

    friend class LogisticRegressionIRLS;

public:
    LogisticRegressionIRLS_ConstState( const BIGINT& width ) :
        width(width),
        // Inter-iteration components
        coef            (width),
        iteration       (0)
    {
        coef.zeros();
    }

};

class LogisticRegressionIRLS {
    // Inter-iteration components in const state
    const LogisticRegressionIRLS_ConstState & constState;

    // Intra-iteration components
    uint64_t numRows;           // number of rows processed this iteration
    VECTOR X_transp_Az;         // X^T A z
    MATRIX X_transp_AX;         // X^T A X
    DOUBLE loglikelihood;       // ln(l(c))

    // Keep track of internal iteration for retrieving results
    uint64_t tuplesProduced;

    // Additional members needed when calculating results.
    VECTOR diag_of_inv_X_transp_AX;     // Diagonal of the inverse of X_transp_AX

    // Helper function
    double sigma( double x ) {
        return 1.0 / (1.0 + std::exp(-x));
    }

public:

    LogisticRegressionIRLS( const LogisticRegressionIRLS_ConstState & state ) :
        constState(state),
        // Intra-iteration components
        numRows         (0),
        X_transp_Az     (state.width),
        X_transp_AX     (state.width, state.width),
        loglikelihood   (0.0),
        // Additional members
        diag_of_inv_X_transp_AX     (state.width)
    {
        X_transp_Az.zeros();
        X_transp_AX.zeros();
    }

    void AddItem( const VECTOR & x, const DOUBLE & y ) {
        const VECTOR & coef = constState.coef;

        ++numRows;

        // xc = x^T_i c
        double xc = dot(x, coef);

        // a_i = sigma(x_i c) sigma(-x_i c)
        double a = sigma(xc) * sigma(-xc);

        // Note: sigma(-x) = 1 - sigma(x).
        //
        //             sigma(-y_i x_i c) y_i
        // z = x_i c + ---------------------
        //                     a_i
        //
        // To avoid overflows if a_i is close to 0, we do not compute z directly,
        // but instead compute a * z.
        double az = xc * a + sigma(-y * xc) * y;

        X_transp_Az += x * az;
        X_transp_AX += x * trans(x) * a;

        //          n
        //         --
        // l(c) = -\  ln(1 + exp(-y_i * c^T x_i))
        //         /_
        //         i=1
        loglikelihood -= std::log( 1. + std::exp(-y * xc) );
    }

    void AddState( const LogisticRegressionIRLS & other ) {
        ADD(numRows);
        ADD(X_transp_Az);
        ADD(X_transp_AX);
        ADD(loglikelihood);
    }

    void Finalize() {
        // Set internal iterator
        tuplesProduced = 0;
    }

    bool ShouldIterate( LogisticRegressionIRLS_ConstState& modibleState ) {
        // References to the modifyable state's members so that we don't have
        // to specifically access the members all the time.
        VECTOR & coef = modibleState.coef;
        uint64_t & iteration = modibleState.iteration;

        MATRIX inverse_of_X_transp_AX = inv(X_transp_AX);

        coef = inverse_of_X_transp_AX * X_transp_Az;

        diag_of_inv_X_transp_AX = diagvec(inverse_of_X_transp_AX);

        // FIXME: Put in real iteration condition
        ++iteration;

        return (iteration < 5);
    }

    // FIXME: The output should technically be a variety of things, including
    // vectors and other complex types.
    // May need to just produce itself as a state.
    bool GetNextResult( BIGINT& count ) {
        const VECTOR & coef = constState.coef;
        const uint64_t & iteration = constState.iteration;

        if( tuplesProduced < 1 ) { // fast track
            ++tuplesProduced;
            count = numRows;

            // Set up vectors to hold diagnostics
            VECTOR stdErr(coef.n_rows);
            VECTOR waldZStats(coef.n_rows);
            VECTOR waldPValues(coef.n_rows);
            VECTOR oddsRatios(coef.n_rows);

            for( size_t i = 0; i < coef.n_rows; ++i ) {
                stdErr(i) = std::sqrt(diag_of_inv_X_transp_AX(i));
                waldZStats(i) = coef(i) / stdErr(i);
                // Note: may need to add wrapper to the boost cdf function to modify
                // the domain, as cdf may throw a domain_error if the input value is
                // infinite instead of returning the correct mathematical result.
                waldPValues(i) = 2.0 * boost::math::cdf( boost::math::normal_distribution<>(),
                        - std::abs(waldZStats(i)));
                oddsRatios(i) = std::exp( coef(i) );
            }

            cout << endl;
            cout << "[LogisticRegressionIRLS] Output for iteration " << iteration << ":" << endl;
            cout << "Coefficients:" << endl;
            cout << coef << endl;
            cout << "Standard Error:" << endl;
            cout << stdErr << endl;
            cout << "Z Statistics:" << endl;
            cout << waldZStats << endl;
            cout << "P Values:" << endl;
            cout << waldPValues << endl;
            cout << "Odds Ratios:" << endl;
            cout << oddsRatios << endl;

            return true;
        }

        return false;
    }
};
