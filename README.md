# CanteraCompilerPerformance

## Introduction

As discussed recently here https://github.com/Cantera/cantera/issues/1155, Cantera cannot be used when compiled with `fast-math` compiler optimizations.
I therefore set up a benchmark to conduct some accuracy and performance measurements of Cantera using different compilers and optimization settings.

## Setup

I am running my tests on `Red Hat Enterprise Linux 8.2` with `Cantera 2.6.0a3` (80c1e568f91bb0fd5be36f18052050d3d0f6fdc6) from Dec. 17, 2021.
The machine consists of a two-socket system with 2x Intel(R) Xeon(R) Platinum 8368 CPUs @ 2.40GHz.

In the benchmark, I included the following compilers:
- 4 versions of `g++` (8.5, 9.4, 10.3, 11.1)
- 1 version of `clang++` (12.0)
- 4 versions of `icpx` (21.1, 21.2, 21.3, 21.4)
- 7 versions of `icpc` (18.0, 19.0, 19.1, 21.1, 21.2, 21.3, 21.4)

I am using three different optimization settings:
- `default`: Cantera's default optimization settings
- `fastmath`: like default, but `fast-math` is enabled
- `full`: even more aggressive optimization flags

In detail, the optimization flags I used look like this (some flags might be redundant, I did not dig through all of the compiler manuals):

**Table 1. Optimization flags.**
| compiler | default | fastmath | full |
| :--- | --- | --- | --- |
| `g++` and `clang++` |	`-O3` |	`-O3 -ffast-math` |	`-fstrict-aliasing -march=native -mtune=native -Ofast -ffast-math` |
| `icpc` |	`-O3 -fp-model precise ` |	`-O3 -fp-model fast=1` |	`-Ofast -ipo -ansi-alias -no-prec-div -no-prec-sqrt -fp-model fast=2 -fast-transcendentals -xHost` |
| `icpx` |	`-O3 -fp-model precise` |	`-O3 -fp-model fast -ffast-math` |	`-fp-model fast -Ofast -ffast-math -ansi-alias -xHost -mtune=native -march=native` |

I compiled Cantera with the following line (omitting some system specific options):

```
scons build -j 76 debug=no optimize=yes python_package=minimal system_yamlcpp=n legacy_rate_constants=yes f90_interface=n optimize_flags="[see above]"
```

All instances of Cantera that are not compiled with `default` options have to be modified. I removed Cantera's handling of NaNs  for all installations with `fastmath` and `full` with a quick and dirty hack:
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
The code was left unchanged for the `default` installations.

## Benchmark Case

As a first benchmark, I measured the performance of `getNetProductionRates` together with the `gri30` reaction mechanism. I ran the function one million times, which takes about 10 s +- 0.1 s, and varied the temperature, pressure and mixture composition each time to bypass any internal caching. I then summed up the reaction rates for all species of all one million calls in order to compare the accuracy of this final result between the compilers. The test program can be found here: https://github.com/g3bk47/CanteraCompilerPerformance/blob/main/reactionRates.cpp.

## Cantera Compilation Times

In the following, I recorded the compile times of the `scons build` line above. I only have a sample size of one for each value, so I don't know what the variance is. `icpc` tends to be a bit slower than the other compilers and `ipo` adds a lot of additonal compile time. In my case, the only external library available on the system was `Eigen`, everything else was compiled as part of `ext/`.

**Table 2. Compilation times in seconds.**
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


## Accuracy

The sum of the net reaction rates over all species and over all one million calls to `getNetProductionRates` is `-2277551.38719024` (kmol/m3/s), when evaluated with `g++8.5` and `default` settings. The largest deviation to that value occurs for `icpc18` with `full` optimizations with a value of `-2277551.38720698`. Table 3 shows the relative accuracy of all cases compared to `g++8.5` with `default` settings.

## Performance

I measured the time it takes to call `getNetProductionRates` with `gri30` one million times (see https://github.com/g3bk47/CanteraCompilerPerformance/blob/main/reactionRates.cpp). I ran each program 100 times. Table 4 shows the mean time over the 100 runs. The standard deviation is between 0.04-0.2 s depending on the compiler and optimization settings.

**Table X. Runtime of one million calls to `getNetProductionRates` with `gri30` in seconds.**
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


**Table X. Relative accuracy `(result(gcc8 default)-result)/result(gcc8 default)`.**
| compiler | noOpt | O2 | strict | default | fastmathsafe | fastmath | full | fullsafe |
| :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `g++8.5.0` | 0.00e+00  | 0.00e+00  | n.a. | 0.00e+00  | 7.17e-12  | 7.17e-12  | 7.17e-12  | 7.17e-12  |
| `g++9.4.0` | 0.00e+00  | 0.00e+00  | n.a. | 0.00e+00  | 7.17e-12  | 7.17e-12  | 7.17e-12  | 7.17e-12  |
| `g++10.3.0` | 0.00e+00  | 0.00e+00  | n.a. | 0.00e+00  | 7.17e-12  | 7.17e-12  | 7.17e-12  | 7.17e-12  |
| `g++11.1.0` | 0.00e+00  | 0.00e+00  | n.a. | 0.00e+00  | 7.17e-12  | 7.17e-12  | 7.17e-12  | 7.17e-12  |
| `clang++12.0.0` | 0.00e+00  | 0.00e+00  | n.a. | 0.00e+00  | 6.44e-12  | 6.45e-12  | 4.43e-12  | 4.43e-12  |
| `icpx2021.1` | 0.00e+00  | 0.00e+00  | 0.00e+00  | 0.00e+00  | 6.44e-12  | 6.44e-12  | 4.43e-12  | 4.43e-12  |
| `icpx2021.2.0` | 0.00e+00  | 0.00e+00  | 0.00e+00  | 0.00e+00  | 6.44e-12  | 6.44e-12  | 4.43e-12  | 4.43e-12  |
| `icpx2021.3.0` | 0.00e+00  | 0.00e+00  | 0.00e+00  | 0.00e+00  | 6.44e-12  | 6.44e-12  | 4.43e-12  | 4.43e-12  |
| `icpx2021.4.0` | 0.00e+00  | 0.00e+00  | 0.00e+00  | 0.00e+00  | 6.44e-12  | 6.44e-12  | 4.43e-12  | 4.43e-12  |
| `icpc18.0.5` | 0.00e+00  | 0.00e+00  | 0.00e+00  | 0.00e+00  | 2.50e-12  | 2.50e-12  | 7.17e-12  | 7.21e-12  |
| `icpc19.0.5` | 0.00e+00  | 0.00e+00  | 0.00e+00  | 0.00e+00  | 2.50e-12  | 2.50e-12  | 7.21e-12  | 7.21e-12  |
| `icpc19.1.3` | 0.00e+00  | 0.00e+00  | 0.00e+00  | 0.00e+00  | 2.50e-12  | 2.50e-12  | 7.17e-12  | 7.21e-12  |
| `icpc2021.1` | 0.00e+00  | 0.00e+00  | 0.00e+00  | 0.00e+00  | 2.50e-12  | 2.50e-12  | 7.17e-12  | 7.21e-12  |
| `icpc2021.2.0` | 0.00e+00  | 0.00e+00  | 0.00e+00  | 0.00e+00  | 2.50e-12  | 2.50e-12  | 7.17e-12  | 7.21e-12  |
| `icpc2021.3.0` | 0.00e+00  | 0.00e+00  | 0.00e+00  | 0.00e+00  | 2.50e-12  | 2.50e-12  | 7.17e-12  | 7.21e-12  |
| `icpc2021.4.0` | 0.00e+00  | 0.00e+00  | 0.00e+00  | 0.00e+00  | 2.50e-12  | 2.50e-12  | 7.17e-12  | 7.21e-12  |


While all `g++` and `clang++` versions yield the same results with the "safe" `default` settings and all Intel compilers yield the same results with `default`, they are slighly different (`-2277551.38719024` for `gcc` and `clang++` vs. `-2277551.38719031` for the Intel compilers). I don't know where this difference comes from exactly. At first I thought that some routines from a library compiled with a `C` compiler might be involed (as the disabling of `fast-math` only occurs for the `C++` Intel compiler in Cantera's build system if I understand it correctly), but I think my code snippet does not involve any external library calls (other than standard math functions, which should be all IEEE conforming in this case).

The relative accuracy differences between the `default` and `fastmath` runs are at O(10^-10 %).



**In conclusion**: enabling `fastmath` increases the performance of `g++` by about 15 % and for the Intel compilers by less than 5 %. Results with the Intel compiler at `default` settings are about 35 % faster than the ones by g++, and at `fastmath` settings the intel compiler performs 25 % faster. The relative differences in the final results are about O(10^-10 %).

# Zero-dimensional Reactor

**Table X. Runtime of one million calls to `getNetProductionRates` with `gri30` in seconds.**
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


**Table X. Relative accuracy `(result(gcc8 default)-result)/result(gcc8 default)`.**
| compiler | noOpt | O2 | strict | default | fastmathsafe | fastmath | full | fullsafe |
| :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `g++8.5.0` | 0.00e+00  | 0.00e+00  | n.a. | 0.00e+00  | 1.01e-13  | 1.01e-13  | 1.87e-13  | 1.87e-13  |
| `g++9.4.0` | 0.00e+00  | 0.00e+00  | n.a. | 0.00e+00  | 1.32e-12  | 1.32e-12  | 8.22e-13  | 8.22e-13  |
| `g++10.3.0` | 0.00e+00  | 0.00e+00  | n.a. | 0.00e+00  | 1.32e-12  | 1.32e-12  | 1.42e-12  | 1.42e-12  |
| `g++11.1.0` | 0.00e+00  | 0.00e+00  | n.a. | 0.00e+00  | 1.32e-12  | 1.32e-12  | 7.82e-13  | 7.82e-13  |
| `clang++12.0.0` | 0.00e+00  | 0.00e+00  | n.a. | 0.00e+00  | 5.43e-13  | 8.71e-13  | 1.54e-12  | 8.07e-13  |
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

**Table X. Runtime of one million calls to `getNetProductionRates` with `gri30` in seconds.**
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


**Table X. Relative accuracy `(result(gcc8 default)-result)/result(gcc8 default)`.**
| compiler | noOpt | O2 | strict | default | fastmathsafe | fastmath | full | fullsafe |
| :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `g++8.5.0` | 0.00e+00  | 0.00e+00  | n.a. | 0.00e+00  | 3.10e-09  | 3.10e-09  | 2.71e-09  | 2.71e-09  |
| `g++9.4.0` | 0.00e+00  | 0.00e+00  | n.a. | 0.00e+00  | 3.10e-09  | 3.10e-09  | 3.16e-09  | 3.16e-09  |
| `g++10.3.0` | 0.00e+00  | 0.00e+00  | n.a. | 0.00e+00  | 3.10e-09  | 3.10e-09  | 2.57e-09  | 2.57e-09  |
| `g++11.1.0` | 0.00e+00  | 0.00e+00  | n.a. | 0.00e+00  | 3.30e-09  | 3.30e-09  | 3.09e-09  | 3.09e-09  |
| `clang++12.0.0` | 0.00e+00  | 0.00e+00  | n.a. | 0.00e+00  | 2.90e-09  | 3.25e-09  | 3.36e-09  | 2.96e-09  |
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

**Table X. Runtime of one million calls to `getNetProductionRates` with `gri30` in seconds.**
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



**Table X. Relative accuracy `(result(gcc8 default)-result)/result(gcc8 default)`.**
| compiler | noOpt | O2 | strict | default | fastmathsafe | fastmath | full | fullsafe |
| :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `g++8.5.0` | 0.00e+00  | 0.00e+00  | n.a. | 0.00e+00  | 3.24e-05  | 3.24e-05  | 4.43e-06  | 4.43e-06  |
| `g++9.4.0` | 0.00e+00  | 0.00e+00  | n.a. | 0.00e+00  | 3.24e-05  | 3.24e-05  | 2.66e-05  | 2.66e-05  |
| `g++10.3.0` | 0.00e+00  | 0.00e+00  | n.a. | 0.00e+00  | 3.24e-05  | 3.24e-05  | 5.79e-06  | 5.79e-06  |
| `g++11.1.0` | 0.00e+00  | 0.00e+00  | n.a. | 0.00e+00  | 2.04e-05  | 2.04e-05  | 6.47e-06  | 6.47e-06  |
| `clang++12.0.0` | 0.00e+00  | 0.00e+00  | n.a. | 0.00e+00  | 3.82e-06  | 2.78e-05  | 2.57e-04  | 1.02e-04  |
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
