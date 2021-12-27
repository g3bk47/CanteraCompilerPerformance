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
    Cantera::vector_fp Y(nSpecies);
    Cantera::vector_fp RR(nSpecies);

    double Tu = 300.0;
    double pressure = 101325.0;
    // mass fraction in unburnt CH4/air mixture at an equivalence ratio of unity
    double Yu[] =   {0, 0, 0, 0.22006045163353515903, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.05516641392519539, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.72477313444126933994, 0, 0, 0, 0, 0};

    double Yb[] = {0.0002644563470936303032, 1.4301131656740240539e-05, 0.00012536884270838356717, 0.0053822430200884145354, 0.0017790028056872840469,
                   0.12046148971584411114, 5.9867684962653181121e-07, 5.6582275795662597661e-08, 9.6250653934340703101e-18, 1.5817696982706584423e-18,
                   4.9879188080892780134e-18, 3.0221117075573467071e-19, 3.390737794845052857e-17, 1.7653423368814362471e-17, 0.0091608543147444349009,
                   0.13693853052214766119, 8.3747521057828367258e-10, 1.4391368644685359644e-11, 4.6294437929035276963e-17, 7.3965072678126367941e-19,
                   4.0619141228008616495e-18, 3.3150902875230312239e-24, 8.6124814774626015782e-22, 6.7594028667499293319e-27, 7.334703182550482444e-27,
                   8.2307441742874032332e-32, 6.1921551001392274235e-33, 7.9660258076252971934e-20, 1.0820889417626471324e-19, 1.0598056539242757278e-22,
                   7.2419078414844568392e-09, 1.2863137373687248127e-09, 5.4773153149081699019e-10, 1.6646232778792049841e-09, 7.9699029977325361003e-10,
                   0.0020620653674327579091, 5.7957347118027331975e-07, 1.6050132250718053082e-07, 3.795856491281827324e-08, 6.3860504913104165441e-14,
                   1.8944117444364307015e-11, 5.564135619964596127e-18, 1.9553746176320010663e-21, 1.5915240747821875467e-16, 1.7678214486001022938e-12,
                   6.0456245975441310291e-10, 2.3549399468901742103e-11, 0.72381024160179130433, 0, 5.4886626188291490727e-47, 3.8562355501877615531e-48,
                   4.5973324930587030561e-25, 9.3375169281721645109e-26};
    
    double Tb = 2225.1320774643604636;
    
    gas.setState_TPY(Tu, pressure,Yu);

    auto t1 = std::chrono::high_resolution_clock::now();

    double sum = 0.;
    for (double i = 0.; i < 1.; i += 0.000001) // evaluate reaction rates one million times at different points between unburnt and burnt state
    {
        // change starting conditions to bypass internal caching
        for (std::size_t k = 0; k != nSpecies; ++k)
            Y[k] = Yu[k]*i + (1.-i)*Yb[k];
        auto T = Tu*i + (1.-i)*Tb;
        pressure += 0.1;

        gas.setState_TP(T,pressure);
        gas.setMassFractions_NoNorm(Y.data());

        kin.getNetProductionRates(RR.data()); // this is the function that is effectively being times

        for (std::size_t k = 0; k != nSpecies; ++k)
            sum += RR[k]; // sum up all reaction rates over all species and all one million conditions
    }

    auto t2 = std::chrono::high_resolution_clock::now();

    std::cout << std::setprecision(20) << sum << " " << std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count() << " s\n";
}
