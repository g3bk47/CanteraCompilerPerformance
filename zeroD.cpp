#include <chrono>
#include <iostream>
#include <iomanip>
#include "cantera/base/Solution.h"
#include "cantera/thermo/IdealGasPhase.h"
#include "cantera/kinetics.h"
#include "cantera/zerodim.h"

int main ()
{
    auto sol = Cantera::newSolution("gri30.yaml", "gri30", "None");
    auto& gas = *(sol->thermo());
    auto& kin = *(sol->kinetics());

    double Tu = 1200.0;
    double pressure = 101325.0;
    
    // mass fractions in unburnt CH4/air mixture at an equivalence ratio of unity
    double Yu[] = {0, 0, 0, 0.22006045163353515903, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.05516641392519539, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.72477313444126933994, 0, 0, 0, 0, 0};
  
    gas.setState_TP(Tu, pressure);
    gas.setMassFractions_NoNorm(Yu);

    Cantera::IdealGasConstPressureReactor r;
    r.insert(sol);
    double tend = 0.325;
    Cantera::ReactorNet sim;
    sim.addReactor(r);
    sim.setTolerances(1e-16,1e-19);
  
    auto t1 = std::chrono::high_resolution_clock::now();
    sim.advance(tend);
    auto t2 = std::chrono::high_resolution_clock::now();
  
    std::cout<<std::setprecision(20)<<r.temperature()<<" "<<std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count() << " s\n";
}
