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
| compiler | default | fastmath | full |
| :--- | ---: | ---: | ---: |
| `g++8.5.0` |	19.23 |	20.44 |	20.43 |
| `g++9.4.0` |	22.11 |	21.94 |	20.59 |
| `g++10.3.0` |	21.43 |	23.56 |	23.39 |
| `g++11.1.0` |	22.99 |	20.30 |	21.78 |
| `clang++12.0.0` |	22.87 |	19.97 |	21.85 |
| `icpx2021.1` |	22.32 |	24.15 |	24.87 |
| `icpx2021.2.0` |	22.75 |	24.13 |	23.34 |
| `icpx2021.3.0` |	21.41 |	21.88 |	21.16 |
| `icpx2021.4.0` |	20.21 |	21.31 |	22.56 |
| `icpc18.0.5` |	30.66 |	32.22 |	86.17 |
| `icpc19.0.5` |	35.63 |	31.19 |	89.90 |
| `icpc19.1.3` |	33.74 |	33.11 |	88.85 |
| `icpc2021.1` |	38.20 |	37.03 |	103.73 |
| `icpc2021.2.0` |	38.70 |	38.85 |	104.53 |
| `icpc2021.3.0` |	42.37 |	40.93 |	109.13 |
| `icpc2021.4.0` |	36.08 |	38.16 |	101.70 |

## Accuracy

The sum of the net reaction rates over all species and over all one million calls to `getNetProductionRates` is `-2277551.38719024` (kmol/m3/s), when evaluated with `g++8.5` and `default` settings. The largest deviation to that value occurs for `icpc18` with `full` optimizations with a value of `-2277551.38720698`. Table 3 shows the relative accuracy of all cases compared to `g++8.5` with `default` settings.

**Table 3. Relative accuracy `(result(gcc8 default)-result)/result(gcc8 default)`.**
| compiler | default | fastmath | full |
| :--- | ---: | ---: | ---: |
| `g++8.5.0`	| 0.00E+00	| -7.11E-12	| -6.94E-12 |
| `g++9.4.0`	| 0.00E+00	| -7.29E-12	| -7.24E-12 |
| `g++10.3.0`	| 0.00E+00	| -7.17E-12	| -7.20E-12 |
| `g++11.1.0`	| 0.00E+00	| -7.17E-12	| -7.20E-12 |
| `clang++12.0.0`	| 0.00E+00	| -6.41E-12	| -4.44E-12 |
| `icpx2021.1`	| -3.07E-14	| -6.38E-12	| -4.35E-12 |
| `icpx2021.2.0`	| -3.07E-14	| -6.44E-12	| -4.35E-12 |
| `icpx2021.3.0`	| -3.07E-14	| -6.44E-12	| -4.35E-12 |
| `icpx2021.4.0`	| -3.07E-14	| -6.19E-12	| -4.35E-12 |
| `icpc18.0.5`	| -3.07E-14	| -2.33E-12	| -7.35E-12 |
| `icpc19.0.5`	| -3.07E-14	| -2.43E-12	| -6.85E-12 |
| `icpc19.1.3`	| -3.07E-14	| -2.43E-12	| -7.22E-12 |
| `icpc2021.1`	| -3.07E-14	| -2.43E-12	| -7.23E-12 |
| `icpc2021.2.0`	| -3.07E-14	| -2.43E-12	| -7.23E-12 |
| `icpc2021.3.0`	| -3.07E-14	| -2.43E-12	| -7.22E-12 |
| `icpc2021.4.0`	| -3.07E-14	| -2.43E-12	| -7.22E-12 |

While all `g++` and `clang++` versions yield the same results with the "safe" `default` settings and all Intel compilers yield the same results with `default`, they are slighly different (`-2277551.38719024` for `gcc` and `clang++` vs. `-2277551.38719031` for the Intel compilers). I don't know where this difference comes from exactly. At first I thought that some routines from a library compiled with a `C` compiler might be involed (as the disabling of `fast-math` only occurs for the `C++` Intel compiler in Cantera's build system if I understand it correctly), but I think my code snippet does not involve any external library calls (other than standard math functions, which should be all IEEE conforming in this case).

The relative accuracy differences between the `default` and `fastmath` runs are at O(10^-10 %).

## Performance

I measured the time it takes to call `getNetProductionRates` with `gri30` one million times (see https://github.com/g3bk47/CanteraCompilerPerformance/blob/main/reactionRates.cpp). I ran each program 100 times. Table 4 shows the mean time over the 100 runs. The standard deviation is between 0.04-0.2 s depending on the compiler and optimization settings.

**Table 4. Runtime of one million calls to `getNetProductionRates` with `gri30` in seconds.**
| compiler | default | fastmath | full |
| :--- | ---: | ---: | ---: |
| g++8.5.0	| 15.19| 	12.81 | 	12.44 |
| g++9.4.0	| 15.21| 	12.90	| 12.49 |
| g++10.3.0	| 15.04| 	12.81	| 12.42 |
| g++11.1.0	| 14.95| 	12.88	| 12.45 |
| clang++12.0.0	| 15.00| 	 13.63	| 13.29 |
| icpx2021.1	| 9.94| 	 9.56	| 9.44 |
| icpx2021.2.0	| 9.86| 	 9.47	| 9.23 |
| icpx2021.3.0	| 9.84| 	 9.36	| 9.20 |
| icpx2021.4.0	| 9.77| 	 9.26	| 9.07 |
| icpc18.0.5	| 10.09| 	 10.02	| 10.10 |
| icpc19.0.5	| 9.60| 	 9.53	| 10.17 |
| icpc19.1.3	| 9.63| 	 9.45	| 9.56 |
| icpc2021.1	| 9.60| 	 9.45	| 9.46 |
| icpc2021.2.0	| 9.61| 	 9.47	| 9.52 |
| icpc2021.3.0	| 9.57| 	 9.48	| 9.50 |
| icpc2021.4.0	| 9.58| 	 9.48	| 9.54 |

**In conclusion**: enabling `fastmath` increases the performance of `g++` by about 15 % and for the Intel compilers by less than 5 %. Results with the Intel compiler at `default` settings are about 35 % faster than the ones by g++, and at `fastmath` settings the intel compiler performs 25 % faster. The relative differences in the final results are about O(10^-10 %).
