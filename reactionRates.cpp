#include <chrono>
#include <iostream>
#include <iomanip>
#include "cantera/base/Solution.h"
#include "cantera/thermo/IdealGasPhase.h"
#include "cantera/kinetics.h"

int main ()
{
    auto sol = Cantera::newSolution("gri30.yaml", "gri30", "None");
    auto& gas = *(sol->thermo());
    auto& kin = *(sol->kinetics());

    auto nSpecies = gas.nSpecies();
    Cantera::vector_fp Yu(nSpecies);
    Cantera::vector_fp Yb(nSpecies);
    Cantera::vector_fp Y(nSpecies);
    Cantera::vector_fp RR(nSpecies);

    double Tu = 300.0;
    double pressure = 101325.0;
    gas.setState_TP(Tu, pressure);
    gas.setEquivalenceRatio(1.0, "CH4", "O2:0.21,N2:0.79");
    gas.getMassFractions(Yu.data());

    gas.equilibrate("HP");
    gas.getMassFractions(Yb.data());
    double Tb = gas.temperature();

    auto t1 = std::chrono::high_resolution_clock::now();

    double sum = 0.;
    // evaluate reaction rates one million times at different points between unburnt and burnt state
    for (double i = 0.; i < 1.; i += 0.000001) 
    {
        // change starting conditions each time to bypass internal caching
        for (std::size_t k = 0; k != nSpecies; ++k)
            Y[k] = Yu[k]*i + (1.-i)*Yb[k];
        auto T = Tu*i + (1.-i)*Tb;
        pressure += 0.1;

        gas.setMassFractions_NoNorm(Y.data());
        gas.setState_TP(T,pressure);

        kin.getNetProductionRates(RR.data()); // this is the function that is effectively being timed

        for (std::size_t k = 0; k != nSpecies; ++k)
            sum += RR[k]; // sum up all reaction rates over all species and all one million conditions
    }

    auto t2 = std::chrono::high_resolution_clock::now();

    // evaluate accuracy and performance
    std::cout << std::setprecision(15) << sum << " "
              << std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count() << " s\n";
}
