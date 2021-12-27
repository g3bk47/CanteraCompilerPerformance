# CanteraCompilerPerformance

## Introduction

As discussed recently here https://github.com/Cantera/cantera/issues/1155, Cantera cannot be used when compiled with `fast-math` compiler optimizations due to explicit use of NaNs in its internal logic. I therefore set up a benchmark to conduct some accuracy and performance measurements of Cantera using different compilers and optimization settings.

## Setup

I am running my tests on `Red Hat Enterprise Linux 8.2` with `Cantera 2.6.0a3` (80c1e568f91bb0fd5be36f18052050d3d0f6fdc6 from Dec. 17, 2021).
The machine consists of a two-socket system with 2x Intel(R) Xeon(R) Platinum 8368 CPUs @ 2.40GHz.

In the benchmark, I included the following compilers:
- 4 versions of `g++` (8.5, 9.4, 10.3, 11.1)
- 1 version of `clang++` (12.0)
- 4 versions of `icpx` (21.1, 21.2, 21.3, 21.4)
- 7 versions of `icpc` (18.0, 19.0, 19.1, 21.1, 21.2, 21.3, 21.4)

I am using eight different optimization settings:
- `noOpt`: no optimiztations
- `O2`: Cantera's default settings, but using O2 instead of O3
- `strict`: for the Intel compilers, use `strict` instead of `precise` for `fp-model`
- `default`: Cantera's default optimization settings
- `fastmathsafe`: like default, but `fast-math` is enabled together with `-fno-finite-math-only`
- `fastmath`: like default, but `fast-math` is enabled
- `full`: even more aggressive optimization flags
- `fullsafe`: like full, but with `-fno-finite-math-only`

In detail, the optimization flags I used look like this (some flags might be redundant, I did not dig through all of the compiler manuals):

For the different `gcc/g++` and `clang/clang++` versions, I used:

**Table 2. Optimization flags for `gcc/g++` and `clang/clang++`.**
| setting | flags |
| :--- | --- |
| `noOpt` | `-O0` |
| `O2` | `-O2` |
| `strict` | only applies to Intel's compilers |
| `default` | `-O3` |
| `fastmathsafe` | `-O3 -ffast-math` |
| `fastmath` | `-O3 -ffast-math -fno-finite-math-only` |
| `full` | `-fstrict-aliasing -march=native -mtune=native -Ofast -ffast-math` |
| `fullsafe` | `-fstrict-aliasing -march=native -mtune=native -Ofast -ffast-math -fno-finite-math-only` |

For the different `icc/icpc` versions, I used:

**Table 3. Optimization flags for `icc/icpc`.**
| setting | flags |
| :--- | --- |
| `noOpt` | `-fp-model strict -O0` |
| `O2` | `-O2 -fp-model precise` |
| `strict` | `-O3 -fp-model strict` |
| `default` | `-O3 -fp-model precise` |
| `fastmathsafe` | `-O3 -fp-model fast=1 -ffast-math -fno-finite-math-only` |
| `fastmath` | `-O3 -fp-model fast=1 -ffast-math` |
| `full` | `-Ofast -ipo -ansi-alias -no-prec-div -no-prec-sqrt -fp-model fast=2 -fast-transcendentals -xHost` |
| `fullsafe` | `-Ofast -ipo -ansi-alias -no-prec-div -no-prec-sqrt -fp-model fast=2 -fast-transcendentals -xHost -fno-finite-math-only` |

For the different `icc/icpc` versions, I used:

**Table 4. Optimization flags for `icx/icpx`.**
| setting | flags |
| :--- | --- |
| `noOpt` | `-fp-model strict -O0` |
| `O2` | `-O2 -fp-model precise` |
| `strict` | `-O3 -fp-model strict` |
| `default` | `-O3 -fp-model precise` |
| `fastmathsafe` | `-O3 -fp-model fast -ffast-math -fno-finite-math-only` |
| `fastmath` | `-O3 -fp-model fast -ffast-math` |
| `full` | `-fp-model fast -Ofast -ffast-math -ansi-alias -xHost -mtune=native -march=native` |
| `fullsafe` | `-fp-model fast -Ofast -ffast-math -ansi-alias -xHost -mtune=native -march=native -fno-finite-math-only` |

I compiled Cantera with the following line (omitting some system specific options):

```
scons build -j 76 debug=no optimize=[noOpt=no,all others=yes] python_package=minimal system_eigen=n system_yamlcpp=n legacy_rate_constants=yes f90_interface=n optimize_flags="[see above]"no_optimize_flags="[see above]"
```

All instances of Cantera that are compiled with `fastmath` or `full` options have to be modified. I removed Cantera's internal handling of NaNs for those two cases with the following quick and dirty hack:
```
grep -rl NAN src/ | xargs sed -i 's/ NAN/ -1e300/g'
grep -rl NAN src/ | xargs sed -i 's/(NAN/(-1e300/g'
grep -rl NAN include/ | xargs sed -i 's/ NAN/ -1e300/g'
grep -rl NAN include/ | xargs sed -i 's/(NAN/(-1e300/g'
grep -rl 'std::isnan' src/ | xargs sed -i 's/std::isnan/nanfinitemath/g'
grep -rl 'isnan' src/ | xargs sed -i 's/isnan/nanfinitemath/g'
sed -i 's/include <cmath>/include <cmath>\ninline bool nanfinitemath(double x)\{return x<-1e299\;\}/g' include/cantera/base/ct_defs.h
grep -rl 'std::numeric_limits<double>::quiet_NaN()' src/ | xargs sed -i 's/std::numeric_limits<double>::quiet_NaN()/-1e300/g'
grep -rl 'std::numeric_limits<double>::quiet_NaN()' include/ | xargs sed -i 's/std::numeric_limits<double>::quiet_NaN()/-1e300/g'
```
Cantera's source code was left unchanged for the `noOpt`, `O2`, `strict`, `default` ,`fastmathsafe` and `fullsafe` installations. Note that it is sufficient to set `-fno-finite-math-only` for all compilers to use Cantera safely, including the Intel compilers independent of the `fp-model`. 

## Cantera Compilation Times

In the following, I recorded the compile times of the `scons build` line above. I only have a sample size of one for each value, so I don't know what the variance is. `icpc` tends to be a bit slower than the other compilers and `ipo` adds a lot of additonal compile time. All external libraries were installed using scons and thus with the same optimization flags as Cantera itself.

**Table 5. Cantera compilation times in seconds.**
| compiler | noOpt | O2 | strict | default | fastmathsafe | fastmath | full | fullsafe |
| :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `g++8.5.0` | 17.24  | 16.84  | n.a. | 17.89  | 17.83  | 17.49  | 18.63  | 18.78  |
| `g++9.4.0` | 16.66  | 19.79  | n.a. | 20.55  | 18.54  | 20.21  | 19.20  | 18.73  |
| `g++10.3.0` | 17.41  | 18.93  | n.a. | 20.95  | 20.43  | 18.81  | 22.07  | 21.30  |
| `g++11.1.0` | 18.30  | 20.73  | n.a. | 21.31  | 21.99  | 22.21  | 22.11  | 21.51  |
| `clang++12.0.0` | 15.47  | 20.35  | n.a. | 17.33  | 19.86  | 19.48  | 19.99  | 21.40  |
| `icpx2021.1` | 15.87  | 20.06  | 22.02  | 21.67  | 22.28  | 21.82  | 22.47  | 24.12  |
| `icpx2021.2.0` | 16.91  | 20.76  | 21.14  | 20.15  | 20.35  | 21.13  | 21.82  | 23.51  |
| `icpx2021.3.0` | 14.85  | 19.03  | 19.42  | 19.84  | 21.09  | 20.43  | 20.82  | 20.52  |
| `icpx2021.4.0` | 14.59  | 21.50  | 19.96  | 20.52  | 21.70  | 21.18  | 20.08  | 19.52  |
| `icpc18.0.5` | 19.35  | 26.95  | 32.64  | 29.80  | 28.96  | 32.23  | 86.80  | 87.30  |
| `icpc19.0.5` | 19.15  | 30.41  | 37.43  | 31.21  | 31.80  | 31.29  | 88.54  | 88.52  |
| `icpc19.1.3` | 21.72  | 27.35  | 35.97  | 32.40  | 33.57  | 32.57  | 89.13  | 88.59  |
| `icpc2021.1` | 33.47  | 33.65  | 39.24  | 40.37  | 39.12  | 35.09  | 103.56  | 103.23  |
| `icpc2021.2.0` | 32.98  | 37.25  | 41.19  | 37.02  | 40.97  | 39.76  | 104.12  | 105.34  |
| `icpc2021.3.0` | 38.50  | 41.67  | 44.74  | 39.22  | 37.90  | 40.70  | 109.01  | 107.93  |
| `icpc2021.4.0` | 28.44  | 33.94  | 41.33  | 32.38  | 32.65  | 37.39  | 100.15  | 98.81  |

# Benchmark Cases

I benchmarked three commin use-cases of Cantera, both for accuracy and computational performance:
- evaluation of chemical reaction rates
- time ingetration of zero-dimensional reactors
- solving one-dimensional freely propagating flames

# Evaluation of Chemical Reaction Rates

As a first benchmark, I measured the performance of `getNetProductionRates` together with the `gri30` reaction mechanism. I ran the function one million times, which takes about 10 s +- 0.1 s, and varied the temperature, pressure and mixture composition each time to bypass any internal caching. I then summed up the reaction rates for all species of all one million calls in order to compare the accuracy of this final result between the compilers. The test program can be found here: https://github.com/g3bk47/CanteraCompilerPerformance/blob/main/reactionRates.cpp.

## Performance

I measured the time it takes to call `getNetProductionRates` with `gri30` one million times (see https://github.com/g3bk47/CanteraCompilerPerformance/blob/main/reactionRates.cpp). I ran each program 100 times. Table 4 shows the mean time over the 100 runs.

Enabling `fastmath` increases the performance of `g++` by about 15 % compared to the default. Enabling `fastmath` together with `no-finite-math-only` still increases `g++`'s performance, but only by about 5 %. For the Intel compilers, the different settings yield almost equal runtimes. Only `fp-model strict` yields slighly slower code (1 % slower). In general, the Intel compilers are about 35 % faster at default optimizations flags compared with `g++` still 25 % faster at the most agressive optimization options. Using `fast math no-finite-math-only` instead of `fp-model precise` for the Intel compilers yields slightly faster code (1 %).

**Table 6. Runtime of one million calls to `getNetProductionRates` with `gri30` in seconds.**
| compiler | noOpt | O2 | strict | default | fastmathsafe | fastmath | full | fullsafe |
| :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `g++8.5.0` | 68.40  | 15.66  | n.a. | 15.15  | 14.26  | 12.85  | 12.39  | 14.06  |
| `g++9.4.0` | 68.37  | 15.64  | n.a. | 15.29  | 14.15  | 12.86  | 12.49  | 13.88  |
| `g++10.3.0` | 75.07  | 15.38  | n.a. | 15.13  | 14.29  | 12.73  | 12.39  | 13.86  |
| `g++11.1.0` | 74.82  | 15.29  | n.a. | 14.90  | 14.11  | 12.85  | 12.41  | 13.67  |
| `clang++12.0.0` | 66.03  | 14.99  | n.a. | 14.98  | 14.87  | 13.65  | 13.29  | 14.47  |
| `icpx2021.1` | 61.26  | 9.94  | 10.35  | 9.94  | 9.63  | 9.59  | 9.39  | 9.36  |
| `icpx2021.2.0` | 61.81  | 9.83  | 10.22  | 9.85  | 9.23  | 9.49  | 9.25  | 9.24  |
| `icpx2021.3.0` | 61.06  | 9.85  | 10.23  | 9.83  | 9.37  | 9.39  | 9.17  | 9.17  |
| `icpx2021.4.0` | 60.13  | 9.77  | 10.14  | 9.74  | 9.27  | 9.29  | 9.01  | 8.96  |
| `icpc18.0.5` | 89.04  | 10.19  | 10.19  | 10.11  | 10.02  | 10.03  | 10.11  | 10.18  |
| `icpc19.0.5` | 86.07  | 9.72  | 9.80  | 9.61  | 9.49  | 9.50  | 9.90  | 9.75  |
| `icpc19.1.3` | 86.21  | 9.65  | 9.66  | 9.57  | 9.44  | 9.47  | 9.58  | 9.55  |
| `icpc2021.1` | 86.30  | 9.59  | 9.65  | 9.58  | 9.49  | 9.50  | 9.33  | 9.79  |
| `icpc2021.2.0` | 86.20  | 9.58  | 9.67  | 9.57  | 9.43  | 9.50  | 9.34  | 9.67  |
| `icpc2021.3.0` | 85.85  | 9.56  | 9.67  | 9.54  | 9.45  | 9.41  | 9.52  | 9.49  |
| `icpc2021.4.0` | 86.05  | 9.64  | 9.66  | 9.54  | 9.48  | 9.48  | 9.47  | 9.48  |

## Accuracy

The sum of the net reaction rates over all species and over all one million calls to `getNetProductionRates` is evaluated. The reference value is taken to be the one of `g++11` with `noOpt` settings. All compilers/version yield the exact (bitwise) same result when compiled with Cantera's default optimization options. Enabling `fastmath` results in differences of O(10^-10 %).

The reference value for the sum of the reaction rates in kmol/m3/s and the largest deviation from it are:
- `-2277551.336645 (g++11 noOpt)`
- `-2277551.336662 (largest deviation)`

**Table 7. Relative accuracy of reaction rate evaluation `(|result(g++11 noOpt)-result)/result(g++11 noOpt)|`.**
| compiler | noOpt | O2 | strict | default | fastmathsafe | fastmath | full | fullsafe |
| :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `g++8.5.0` | 0  | 0  | n.a. | 0  | 7.17e-12  | 7.17e-12  | 7.17e-12  | 7.17e-12  |
| `g++9.4.0` | 0  | 0  | n.a. | 0  | 7.17e-12  | 7.17e-12  | 7.17e-12  | 7.17e-12  |
| `g++10.3.0` | 0  | 0  | n.a. | 0  | 7.17e-12  | 7.17e-12  | 7.17e-12  | 7.17e-12  |
| `g++11.1.0` | 0  | 0  | n.a. | 0  | 7.17e-12  | 7.17e-12  | 7.17e-12  | 7.17e-12  |
| `clang++12.0.0` | 0  | 0  | n.a. | 0  | 6.44e-12  | 6.45e-12  | 4.43e-12  | 4.43e-12  |
| `icpx2021.1` | 0  | 0  | 0  | 0  | 6.44e-12  | 6.44e-12  | 4.43e-12  | 4.43e-12  |
| `icpx2021.2.0` | 0  | 0  | 0  | 0  | 6.44e-12  | 6.44e-12  | 4.43e-12  | 4.43e-12  |
| `icpx2021.3.0` | 0  | 0  | 0  | 0  | 6.44e-12  | 6.44e-12  | 4.43e-12  | 4.43e-12  |
| `icpx2021.4.0` | 0  | 0  | 0  | 0  | 6.44e-12  | 6.44e-12  | 4.43e-12  | 4.43e-12  |
| `icpc18.0.5` | 0  | 0  | 0  | 0  | 2.50e-12  | 2.50e-12  | 7.17e-12  | 7.21e-12  |
| `icpc19.0.5` | 0  | 0  | 0  | 0  | 2.50e-12  | 2.50e-12  | 7.21e-12  | 7.21e-12  |
| `icpc19.1.3` | 0  | 0  | 0  | 0  | 2.50e-12  | 2.50e-12  | 7.17e-12  | 7.21e-12  |
| `icpc2021.1` | 0  | 0  | 0  | 0  | 2.50e-12  | 2.50e-12  | 7.17e-12  | 7.21e-12  |
| `icpc2021.2.0` | 0  | 0  | 0  | 0  | 2.50e-12  | 2.50e-12  | 7.17e-12  | 7.21e-12  |
| `icpc2021.3.0` | 0  | 0  | 0  | 0  | 2.50e-12  | 2.50e-12  | 7.17e-12  | 7.21e-12  |
| `icpc2021.4.0` | 0  | 0  | 0  | 0  | 2.50e-12  | 2.50e-12  | 7.17e-12  | 7.21e-12  |


# Zero-dimensional Reactor

The second benchmark case measures the computing time for a one-dimensional auto-ignition case. The reaction mechanism is again `gri30`. A methane/air mixture at T=1200 K undergoes auto-ignition. The time integration of that problem is performed for t=0.325 s. The temperature at that time point is compared to assess the accuracy. For the performance evaluation, eahc program is ran one thousand times. The code for the benchmark case can be found here: https://github.com/g3bk47/CanteraCompilerPerformance/blob/main/zeroD.cpp

## Performance

Below is the time required to integrate the reactor equation over t=0.325 s. Similar to the first benchmark, enabling `fastmath` speeds up the `g++` code by 11 %. Enabling `fastmath` but with `no-finite-math-only` only increases performance by about 5 %. Again, the different options almost do not affect the timings of the (newer) Intel compilers, which are about 25 % faster than `g++/clang++` at default settings and 15 % faster at full optimization settings. In this case, compiling with `O2` with `g++` is about 8 % slower than compiling with `O3`. Using `fast math no-finite-math-only` instead of `fp-model precise` for the Intel compilers yields slightly faster code (<1 %).

**Table 8. Runtime for 0D reactor integration in seconds.**
| compiler | noOpt | O2 | strict | default | fastmathsafe | fastmath | full | fullsafe |
| :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `g++8.5.0` | 1.844  | 0.378  | n.a. | 0.344  | 0.318  | 0.292  | 0.288  | 0.319  |
| `g++9.4.0` | 1.844  | 0.375  | n.a. | 0.342  | 0.326  | 0.303  | 0.280  | 0.313  |
| `g++10.3.0` | 1.964  | 0.374  | n.a. | 0.343  | 0.328  | 0.299  | 0.276  | 0.307  |
| `g++11.1.0` | 1.972  | 0.372  | n.a. | 0.341  | 0.323  | 0.303  | 0.276  | 0.299  |
| `clang++12.0.0` | 1.730  | 0.346  | n.a. | 0.345  | 0.340  | 0.318  | 0.306  | 0.326  |
| `icpx2021.1` | 1.697  | 0.262  | 0.280  | 0.263  | 0.248  | 0.256  | 0.231  | 0.242  |
| `icpx2021.2.0` | 1.695  | 0.262  | 0.278  | 0.263  | 0.321  | 0.242  | 0.226  | 0.246  |
| `icpx2021.3.0` | 1.693  | 0.262  | 0.277  | 0.259  | 0.240  | 0.240  | 0.224  | 0.238  |
| `icpx2021.4.0` | 1.668  | 0.260  | 0.278  | 0.261  | 0.252  | 0.240  | 0.228  | 0.229  |
| `icpc18.0.5` | 2.519  | 0.264  | 0.262  | 0.263  | 0.260  | 0.260  | 0.257  | 0.260  |
| `icpc19.0.5` | 2.372  | 0.255  | 0.258  | 0.255  | 0.252  | 0.252  | 0.247  | 0.244  |
| `icpc19.1.3` | 2.375  | 0.252  | 0.256  | 0.253  | 0.249  | 0.251  | 0.231  | 0.239  |
| `icpc2021.1` | 2.375  | 0.253  | 0.256  | 0.254  | 0.249  | 0.249  | 0.223  | 0.238  |
| `icpc2021.2.0` | 2.376  | 0.253  | 0.256  | 0.253  | 0.249  | 0.249  | 0.222  | 0.240  |
| `icpc2021.3.0` | 2.374  | 0.253  | 0.256  | 0.253  | 0.248  | 0.250  | 0.231  | 0.237  |
| `icpc2021.4.0` | 2.373  | 0.252  | 0.256  | 0.254  | 0.248  | 0.247  | 0.230  | 0.237  |

## Accuracy

The temperature at t=0.325 s is evaluated. The reference temperature in K from `g++11 noOpt` and the temperature that deviates most from that value among all compilers are:
- `2150.5660919129 (g++11 noOpt)`
- `2150.5660919159 (largest deviation)`

In this case, the results are not the same bitwise between `g++/clang++` and the Intel compilers. Again, the deviations are within O(10^-10 %).

**Table 9. Relative accuracy of 0D reactor integration `(|result(g++11 noOpt)-result)/result(g++11 noOpt)|`.**
| compiler | noOpt | O2 | strict | default | fastmathsafe | fastmath | full | fullsafe |
| :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `g++8.5.0` | 0  | 0  | n.a. | 0  | 1.01e-13  | 1.01e-13  | 1.87e-13  | 1.87e-13  |
| `g++9.4.0` | 0  | 0  | n.a. | 0  | 1.32e-12  | 1.32e-12  | 8.22e-13  | 8.22e-13  |
| `g++10.3.0` | 0  | 0  | n.a. | 0  | 1.32e-12  | 1.32e-12  | 1.42e-12  | 1.42e-12  |
| `g++11.1.0` | 0  | 0  | n.a. | 0  | 1.32e-12  | 1.32e-12  | 7.82e-13  | 7.82e-13  |
| `clang++12.0.0` | 0  | 0  | n.a. | 0  | 5.43e-13  | 8.71e-13  | 1.54e-12  | 8.07e-13  |
| `icpx2021.1` | 8.90e-13  | 8.90e-13  | 8.90e-13  | 8.90e-13  | 4.38e-13  | 6.69e-13  | 3.14e-13  | 9.32e-13  |
| `icpx2021.2.0` | 8.90e-13  | 8.90e-13  | 8.90e-13  | 8.90e-13  | 4.54e-13  | 9.72e-13  | 3.31e-13  | 1.01e-12  |
| `icpx2021.3.0` | 8.90e-13  | 8.90e-13  | 8.90e-13  | 8.90e-13  | 4.54e-13  | 9.72e-13  | 3.31e-13  | 1.01e-12  |
| `icpx2021.4.0` | 8.90e-13  | 8.90e-13  | 8.90e-13  | 8.90e-13  | 7.81e-13  | 9.41e-13  | 8.50e-13  | 5.94e-13  |
| `icpc18.0.5` | 8.90e-13  | 8.90e-13  | 8.90e-13  | 8.90e-13  | 7.49e-13  | 7.49e-13  | 2.18e-13  | 9.45e-13  |
| `icpc19.0.5` | 8.90e-13  | 8.90e-13  | 8.90e-13  | 8.90e-13  | 7.49e-13  | 7.49e-13  | 2.18e-13  | 2.18e-13  |
| `icpc19.1.3` | 8.90e-13  | 8.90e-13  | 8.90e-13  | 8.90e-13  | 7.49e-13  | 7.49e-13  | 8.17e-13  | 2.18e-13  |
| `icpc2021.1` | 8.90e-13  | 8.90e-13  | 8.90e-13  | 8.90e-13  | 7.49e-13  | 7.49e-13  | 9.00e-13  | 9.45e-13  |
| `icpc2021.2.0` | 8.90e-13  | 8.90e-13  | 8.90e-13  | 8.90e-13  | 7.49e-13  | 7.49e-13  | 9.00e-13  | 9.45e-13  |
| `icpc2021.3.0` | 8.90e-13  | 8.90e-13  | 8.90e-13  | 8.90e-13  | 7.49e-13  | 7.49e-13  | 8.17e-13  | 2.18e-13  |
| `icpc2021.4.0` | 8.90e-13  | 8.90e-13  | 8.90e-13  | 8.90e-13  | 7.49e-13  | 7.49e-13  | 8.17e-13  | 2.18e-13  |

# One-dimensional freely propagating flame (coarse)

The third benchmark is a freely propagating H2/O2 flame at atmospheric conditions. This case is denoted as "coarse" (`slope` and `curve` =  0.001). Times are measured and averaged over ten runs. To compare the accuracy, the flame speed (taken as the velocity at the first domain point) is evaluated. The code of this case can be found here: https://github.com/g3bk47/CanteraCompilerPerformance/blob/main/oneD.cpp

## Performance

Below is the time required to solve the 1D problem. Enabling `fastmath` speeds up the `g++` code by only 3 %. Enabling `fastmath` but with `no-finite-math-only` increases performance by about 0.5 %. Again, the different options almost do not affect the timings of the (newer) Intel compilers, which are about 8 % faster than `g++/clang++` at default settings and 7 % faster at full optimization settings. Using `fast math no-finite-math-only` instead of `fp-model precise` for the Intel compilers yields slightly faster code (<1 %).

**Table 11. Runtime for solving a (coarse) 1D flame in seconds.**
| compiler | noOpt | O2 | strict | default | fastmathsafe | fastmath | full | fullsafe |
| :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `g++8.5.0` | 119.24  | 16.57  | n.a. | 9.39  | 9.24  | 9.03  | 8.92  | 9.36  |
| `g++9.4.0` | 120.17  | 16.57  | n.a. | 9.31  | 9.25  | 9.09  | 8.86  | 9.20  |
| `g++10.3.0` | 121.91  | 9.48  | n.a. | 9.24  | 9.19  | 9.00  | 8.87  | 9.10  |
| `g++11.1.0` | 122.12  | 9.51  | n.a. | 9.26  | 9.12  | 8.93  | 8.92  | 9.24  |
| `clang++12.0.0` | 109.14  | 9.33  | n.a. | 9.38  | 9.35  | 9.12  | 8.98  | 9.24  |
| `icpx2021.1` | 108.15  | 8.43  | 8.61  | 8.43  | 8.37  | 8.42  | 8.43  | 8.35  |
| `icpx2021.2.0` | 107.67  | 8.48  | 8.61  | 8.43  | 8.40  | 8.43  | 8.42  | 8.33  |
| `icpx2021.3.0` | 108.29  | 8.49  | 8.64  | 8.49  | 8.40  | 8.43  | 8.35  | 8.32  |
| `icpx2021.4.0` | 107.15  | 8.47  | 8.64  | 8.47  | 8.39  | 8.40  | 8.33  | 8.32  |
| `icpc18.0.5` | 135.82  | 8.52  | 8.79  | 8.56  | 8.52  | 8.53  | 8.31  | 8.33  |
| `icpc19.0.5` | 132.71  | 8.42  | 8.71  | 8.56  | 8.49  | 8.42  | 8.25  | 8.27  |
| `icpc19.1.3` | 128.56  | 8.53  | 8.67  | 8.53  | 8.54  | 8.45  | 8.24  | 8.18  |
| `icpc2021.1` | 123.33  | 8.64  | 8.70  | 8.49  | 8.45  | 8.48  | 8.34  | 8.16  |
| `icpc2021.2.0` | 124.04  | 8.49  | 8.75  | 8.48  | 8.47  | 8.46  | 8.29  | 8.16  |
| `icpc2021.3.0` | 116.15  | 8.51  | 8.71  | 8.48  | 8.56  | 8.51  | 8.15  | 8.19  |
| `icpc2021.4.0` | 129.17  | 8.46  | 8.67  | 8.56  | 8.46  | 8.46  | 8.15  | 8.21  |

## Accuracy

The flame speed is evaluated. The reference flame speed in m/s from `g++11 noOpt` and the flame speed that deviates most from that value among all compilers are:
- `10.14616550 (g++11 noOpt)`
- `10.14616549 (largest deviation)`

Again, the results are not the same bitwise between `g++/clang++` and the Intel compilers. The deviations are within O(10^-7 %). Note that the only compiler that yields different results at default settings than all other compilers is `icpc18.0.5`.

**Table 12. Relative accuracy for solving a (coarse) 1D flame `(|result(g++11 noOpt)-result)/result(g++11 noOpt)|`.**
| compiler | noOpt | O2 | strict | default | fastmathsafe | fastmath | full | fullsafe |
| :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `g++8.5.0` | 0  | 0  | n.a. | 0  | 3.10e-09  | 3.10e-09  | 2.71e-09  | 2.71e-09  |
| `g++9.4.0` | 0  | 0  | n.a. | 0 | 3.10e-09  | 3.10e-09  | 3.16e-09  | 3.16e-09  |
| `g++10.3.0` | 0  | 0  | n.a. | 0  | 3.10e-09  | 3.10e-09  | 2.57e-09  | 2.57e-09  |
| `g++11.1.0` | 0  | 0  | n.a. | 0  | 3.30e-09  | 3.30e-09  | 3.09e-09  | 3.09e-09  |
| `clang++12.0.0` | 0 | 0  | n.a. | 0  | 2.90e-09  | 3.25e-09  | 3.36e-09  | 2.96e-09  |
| `icpx2021.1` | 6.86e-10  | 6.86e-10  | 6.86e-10  | 6.86e-10  | 2.81e-09  | 2.93e-09  | 3.75e-09  | 2.94e-09  |
| `icpx2021.2.0` | 6.86e-10  | 6.86e-10  | 6.86e-10  | 6.86e-10  | 3.67e-09  | 3.36e-09  | 3.48e-09  | 2.54e-09  |
| `icpx2021.3.0` | 6.86e-10  | 6.86e-10  | 6.86e-10  | 6.86e-10  | 3.67e-09  | 3.36e-09  | 3.48e-09  | 2.54e-09  |
| `icpx2021.4.0` | 6.86e-10  | 6.86e-10  | 6.86e-10  | 6.86e-10  | 3.23e-09  | 2.59e-09  | 2.60e-09  | 4.04e-09  |
| `icpc18.0.5` | 6.86e-10  | 1.22e-09  | 3.86e-10  | 1.22e-09  | 9.18e-10  | 9.18e-10  | 3.14e-09  | 3.14e-09  |
| `icpc19.0.5` | 6.86e-10  | 6.86e-10  | 6.86e-10  | 6.86e-10  | 9.18e-10  | 9.18e-10  | 3.09e-09  | 3.09e-09  |
| `icpc19.1.3` | 6.86e-10  | 6.86e-10  | 6.86e-10  | 6.86e-10  | 9.18e-10  | 9.18e-10  | 3.09e-09  | 3.09e-09  |
| `icpc2021.1` | 6.86e-10  | 6.86e-10  | 6.86e-10  | 6.86e-10  | 9.18e-10  | 9.18e-10  | 3.09e-09  | 3.09e-09  |
| `icpc2021.2.0` | 6.86e-10  | 6.86e-10  | 6.86e-10  | 6.86e-10  | 9.18e-10  | 9.18e-10  | 3.09e-09  | 3.09e-09  |
| `icpc2021.3.0` | 6.86e-10  | 6.86e-10  | 6.86e-10  | 6.86e-10  | 9.18e-10  | 9.18e-10  | 3.09e-09  | 3.09e-09  |
| `icpc2021.4.0` | 6.86e-10  | 6.86e-10  | 6.86e-10  | 6.86e-10  | 9.18e-10  | 9.18e-10  | 3.09e-09  | 3.09e-09  |

# One-dimensional freely propagating flame (fine)

The last benchmark case is the same 1D flame from before, but with ten times lower refinement settings (`slope` and `curve` =  0.0001, https://github.com/g3bk47/CanteraCompilerPerformance/blob/main/oneD.cpp). Each simulation is ran once.

## Performance

This case converges in 10 to 20 minutes at default optimization flags. Enabling `fastmath`, however, can increase simulation times tenfold. This behavior is very much dependent on the compiler/version. There are even cases where simulations with `fastmath` find a solution, but with `fastmath no-finite-math-only` I had to cancel the simulation after 36 hours. Therefore, neither `fastmath` nor `fastmath no-finite-math-only` should be used in the defaults.

**Table 13. Runtime for solving a (fine) 1D flame in hours.**
| compiler | noOpt | O2 | strict | default | fastmathsafe | fastmath | full | fullsafe |
| :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `g++8.5.0` | 1.64  | 0.22  | n.a. | 0.17  | 0.44  | 0.46  | 0.77  | 0.76  |
| `g++9.4.0` | 1.65  | 0.22  | n.a. | 0.19  | 0.47  | 0.45  | 0.77  | 0.77  |
| `g++10.3.0` | 1.65  | 0.17  | n.a. | 0.18  | 0.50  | 0.46  | 0.16  | 0.18  |
| `g++11.1.0` | 1.66  | 0.17  | n.a. | 0.19  | 0.75  | 0.77  | 0.39  | 0.39  |
| `clang++12.0.0` | 1.54  | 0.17  | n.a. | 0.18  | 0.83  | 0.77  | 27.89  | 3.11  |
| `icpx2021.1` | 2.81  | 0.31  | 0.32  | 0.31  | 0.31  | 0.32  | 1.07  | 0.51  |
| `icpx2021.2.0` | 2.82  | 0.33  | 0.31  | 0.34  | >36  | 0.37  | 0.29  | 7.94  |
| `icpx2021.3.0` | 2.84  | 0.33  | 0.32  | 0.32  | >36  | 0.37  | 0.29  | 7.83  |
| `icpx2021.4.0` | 2.84  | 0.33  | 0.33  | 0.32  | 0.48  | 0.62  | 0.49  | 0.54  |
| `icpc18.0.5` | 3.09  | 0.83  | 3.22  | 0.90  | 16.50  | 14.91  | 0.23  | 0.23  |
| `icpc19.0.5` | 3.02  | 0.36  | 0.33  | 0.33  | 13.50  | 13.70  | 0.82  | 0.89  |
| `icpc19.1.3` | 2.59  | 0.34  | 0.36  | 0.32  | 12.71  | 14.16  | 0.93  | 0.88  |
| `icpc2021.1` | 2.20  | 0.35  | 0.33  | 0.34  | 13.56  | 13.38  | 0.81  | 0.84  |
| `icpc2021.2.0` | 2.30  | 0.32  | 0.36  | 0.31  | 13.60  | 13.64  | 0.81  | 0.94  |
| `icpc2021.3.0` | 2.43  | 0.33  | 0.33  | 0.31  | 13.18  | 14.89  | 0.93  | 0.94  |
| `icpc2021.4.0` | 3.04  | 0.36  | 0.34  | 0.34  | 14.71  | 13.37  | 0.83  | 0.81  |

## Accuracy

Although the flame speed should converge more closely beteween the different compilers/versions due to the finer mesh, results differ by up to 0.001 %, much more than in the previous case. Again, `icpc18.0.5` produces different results compared with all other Intel compilers at default optimization settings.

**Table 14. Relative accuracy for solving a (fine) 1D flame `(|result(g++11 noOpt)-result)/result(g++11 noOpt)|`.**
| compiler | noOpt | O2 | strict | default | fastmathsafe | fastmath | full | fullsafe |
| :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `g++8.5.0` | 0  | 0  | n.a. | 0  | 3.24e-05  | 3.24e-05  | 4.43e-06  | 4.43e-06  |
| `g++9.4.0` | 0  | 0  | n.a. | 0  | 3.24e-05  | 3.24e-05  | 2.66e-05  | 2.66e-05  |
| `g++10.3.0` | 0  | 0  | n.a. | 0  | 3.24e-05  | 3.24e-05  | 5.79e-06  | 5.79e-06  |
| `g++11.1.0` | 0  | 0  | n.a. | 0  | 2.04e-05  | 2.04e-05  | 6.47e-06  | 6.47e-06  |
| `clang++12.0.0` | 0  | 0  | n.a. | 0  | 3.82e-06  | 2.78e-05  | 2.57e-04  | 1.02e-04  |
| `icpx2021.1` | 2.07e-05  | 2.07e-05  | 2.07e-05  | 2.07e-05  | 4.20e-06  | 1.77e-05  | 3.17e-06  | 1.15e-05  |
| `icpx2021.2.0` | 2.07e-05  | 2.07e-05  | 2.07e-05  | 2.07e-05  | ??  | 3.89e-05  | 3.17e-05  | 2.53e-05  |
| `icpx2021.3.0` | 2.07e-05  | 2.07e-05  | 2.07e-05  | 2.07e-05  | ??  | 3.89e-05  | 3.17e-05  | 2.53e-05  |
| `icpx2021.4.0` | 2.07e-05  | 2.07e-05  | 2.07e-05  | 2.07e-05  | 2.57e-05  | 1.09e-05  | 3.61e-05  | 3.71e-05  |
| `icpc18.0.5` | 2.07e-05  | 6.34e-06  | 6.55e-05  | 6.34e-06  | 8.32e-05  | 8.32e-05  | 2.82e-05  | 2.82e-05  |
| `icpc19.0.5` | 2.07e-05  | 2.07e-05  | 2.07e-05  | 2.07e-05  | 3.29e-05  | 3.29e-05  | 1.49e-05  | 1.49e-05  |
| `icpc19.1.3` | 2.07e-05  | 2.07e-05  | 2.07e-05  | 2.07e-05  | 3.29e-05  | 3.29e-05  | 1.49e-05  | 1.49e-05  |
| `icpc2021.1` | 2.07e-05  | 2.07e-05  | 2.07e-05  | 2.07e-05  | 3.29e-05  | 3.29e-05  | 1.49e-05  | 1.49e-05  |
| `icpc2021.2.0` | 2.07e-05  | 2.07e-05  | 2.07e-05  | 2.07e-05  | 3.29e-05  | 3.29e-05  | 1.49e-05  | 1.49e-05  |
| `icpc2021.3.0` | 2.07e-05  | 2.07e-05  | 2.07e-05  | 2.07e-05  | 3.29e-05  | 3.29e-05  | 1.49e-05  | 1.49e-05  |
| `icpc2021.4.0` | 2.07e-05  | 2.07e-05  | 2.07e-05  | 2.07e-05  | 3.29e-05  | 3.29e-05  | 1.49e-05  | 1.49e-05  |

# Conclusions

I ran different sample programs (evaluation of reaction rates, 0D auto-ignition and 1D flame) with 16 different compilers/versions and 8 different optimization settings. The findings are summarized as follows:

- Even at the strictest settings, Intel compilers and `g++/clang++` do not yield the same results (bitwise) in general.
- For simpler cases, differences in results are within O(10^-7 %).
- For `g++`, `O2` generates slightly slower code compared to `O3` but without affecting the results.
- In many cases, using `fastmath` increases performance by 10 % to 15 % for `g++`. Using ` fastmath` together with `no-finite-math-only` increases performance by only 5 %. However, both options can drastically deteriorate convergence behavior.
- For the Intel compiler, `fp-model strict` is slighly slower than `fp-model precise` but the accuracy was the same in all test cases. `fastmath` together with `no-finite-math-only` produces slighly faster code and can be used together with Cantera, however, convergence might deteriorate drastically. In general, the difference between compiler settings have much less effect for the Intel compilers than for `g++/clang++`.

From my tests above, the current defaults of Cantera seem to be the optimal compromise between performance and safety:
- `O3` for `g++/clang++`
- `O3 -fp-model precise` for the Intel compilers

Since `fastmath` **without** `no-finite-math-only` can significantly improve the performance of `g++` for simple cases like the evaluation of reaction rates, it would be nice for Cantera to be compatible with this option, e.g. when users couple Cantera to other CFD codes, which means removing the internal use of NaNs.
