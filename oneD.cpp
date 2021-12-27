#include <chrono>
#include <iostream>
#include <iomanip>
#include "cantera/base/Solution.h"
#include "cantera/thermo/IdealGasPhase.h"
#include "cantera/kinetics.h"
#include "cantera/oneD/Sim1D.h"
#include "cantera/oneD/Boundary1D.h"
#include "cantera/oneD/StFlow.h"
#include "cantera/transport.h"

int main ()
{
    auto sol = Cantera::newSolution("h2o2.yaml");
    auto& gas = *(sol->thermo());
    auto& kin = *(sol->kinetics());

    auto nsp = gas.nSpecies();

    double uin = 2.0;
    double Tu = 300.0;
    double temp = Tu;
    double pressure = 101325.0;
    double Yu[] = {0.11190674437968359256, 0, 0, 0.88809325562031637968, 0, 0, 0, 0, 0, 0};

    double Yb[] = {0.020266412104572629344, 0.0052204337284118820448, 0.03559043926357815385, 0.10968750204444184526,
                   0.12100713261838066948, 0.70813188076631972123, 9.0306678609972874439e-05, 5.8927956848147251519e-06, 0, 0};
  
    double Tb = 3077.1439283700274245;
  
    gas.setState_TP(Tu, pressure);
    gas.setMassFractions_NoNorm(Yu);

    Cantera::vector_fp x(nsp);
    gas.getMoleFractions(x.data());

    double rho_in = gas.density();

    Cantera::vector_fp yin(nsp);
    gas.getMassFractions(&yin[0]);

    gas.setState_TP(Tb, pressure);
    gas.setMassFractions_NoNorm(Yb);

    Cantera::vector_fp yout(nsp);
    gas.getMassFractions(&yout[0]);
    double rho_out = gas.density();
    double Tad = gas.temperature();

    Cantera::StFlow flow(&gas);
    flow.setFreeFlow();

    // create an initial grid
    int nz = 6;
    double lz = 0.1;
    Cantera::vector_fp z(nz);
    double dz = lz/((double)(nz-1));
    for (int iz = 0; iz < nz; iz++)
    {
        z[iz] = ((double)iz)*dz;
    }

    flow.setupGrid(nz, &z[0]);

    // specify the objects to use to compute kinetic rates and transport properties

    std::unique_ptr<Cantera::Transport> trmix(newTransportMgr("Mix", sol->thermo().get()));

    flow.setTransport(*trmix);
    flow.setKinetics(*sol->kinetics());
    flow.setPressure(pressure);

    //------- step 2: create the inlet  -----------------------

    Cantera::Inlet1D inlet;

    inlet.setMoleFractions(x.data());
    double mdot=uin*rho_in;
    inlet.setMdot(mdot);
    inlet.setTemperature(temp);


    //------- step 3: create the outlet  ---------------------

    Cantera::Outlet1D outlet;

    //=================== create the container and insert the domains =====

    std::vector<Cantera::Domain1D*> domains { &inlet, &flow, &outlet };
    Cantera::Sim1D flame(domains);

    //----------- Supply initial guess----------------------

    Cantera::vector_fp locs{0.0, 0.3, 0.7, 1.0};

    double uout = inlet.mdot()/rho_out;
    Cantera::vector_fp value{uin, uin, uout, uout};
    flame.setInitialGuess("velocity",locs,value);
    value = {temp, temp, Tad, Tad};
    flame.setInitialGuess("T",locs,value);

    for (size_t i=0; i<nsp; i++)
    {
        value = {yin[i], yin[i], yout[i], yout[i]};
        flame.setInitialGuess(gas.speciesName(i),locs,value);
    }

    inlet.setMoleFractions(x.data());
    inlet.setMdot(mdot);
    inlet.setTemperature(temp);

    //flame.showSolution();

    int flowdomain = 1;
    double ratio = 2.0;
    double slope = 0.001; // for fine simulation: 0.0001
    double curve = 0.001; // for fine simulation: 0.0001

    flame.setRefineCriteria(flowdomain,ratio,slope,curve);
    flame.setMaxGridPoints(flowdomain, 1000000000);
    flame.setGridMin(flowdomain, 1e-15);

    // Solve freely propagating flame

    flame.setFixedTemperature(0.5 * (temp + Tad));
  
    auto t1 = std::chrono::high_resolution_clock::now();
    flow.solveEnergyEqn();
    flame.solve(0,true);
    auto t2 = std::chrono::high_resolution_clock::now();

    std::cout << std::setprecision(20)
      << flame.value(flowdomain,flow.componentIndex("velocity"),0)
      << " " << std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count() << " s\n";
}
