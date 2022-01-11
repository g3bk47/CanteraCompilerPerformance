# CanteraCompilerPerformance on two additional systems

## 1) Performance of reaction rate calculation

Mean runtime for over 20 runs for reaction rate calculation from [??]. For a description of the compile flags, see [??]

### System 1: Intel Xeon E5-2660 v4 (Broadwell)

**Table 1. Runtime in seconds for calculatin reaction rates on Intel Xeon E5-2660 v4.**
| compiler | noOpt | O2 | strict | default | fastmathsafe | fastmath | full | fullsafe |
| :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `g++8` | 85.91  | 20.79  | n.a. | 20.33  | 19.71  | 17.23  | 16.80  | 19.21  |
| `g++9` | 86.88  | 20.74  | n.a. | 20.62  | 19.67  | 17.46  | 17.07  | 19.44  |
| `g++10.2` | 93.46  | 20.61  | n.a. | 20.32  | 19.41  | 17.16  | 16.77  | 19.14  |
| `g++10.3` | 92.34  | 20.39  | n.a. | 20.25  | 19.59  | 17.14  | 16.92  | 19.35  |
| `g++11.1` | 92.92  | 20.24  | n.a. | 20.23  | 19.40  | 17.22  | 16.77  | 18.83  |
| `g++11.2` | 92.33  | 20.44  | n.a. | 19.96  | 19.28  | 17.21  | 16.76  | 18.87  |
| `clang++9.0` | 90.57  | 20.48  | n.a. | 20.43  | 20.62  | 17.87  | 17.85  | 20.48  |
| `clang++10.0` | 90.51  | 20.55  | n.a. | 20.74  | 20.49  | 20.28  | 19.97  | 20.31  |
| `clang++11.0` | 90.29  | 20.60  | n.a. | 20.60  | 20.64  | 20.53  | 20.08  | 20.53  |
| `clang++12.0` | 90.57  | 20.34  | n.a. | 20.54  | 20.38  | 18.04  | 18.10  | 20.12  |
| `clang++13.0` | 90.66  | 20.42  | n.a. | 20.68  | 20.34  | 18.00  | 17.85  | 20.07  |
| `icpx2021.4` | 81.84  | 13.43  | 13.91  | 13.43  | 13.06  | 13.01  | 12.87  | 12.83  |
| `icpc19.0` | 109.35  | 13.05  | 13.32  | 12.96  | 13.02  | 12.91  | 12.57  | 12.67  |
| `icpc19.1` | 110.40  | 12.97  | 13.15  | 12.94  | 12.95  | 12.96  | 12.99  | 12.59  |
| `icpc21.4` | 109.60  | 12.84  | 13.19  | 13.02  | 12.90  | 12.99  | 12.74  | 12.64  |

### System 2: Intel Xeon Gold 6230 (Cascade Lake)

**Table 2. Runtime in seconds for calculatin reaction rates on Intel Xeon Gold 6230.**
| compiler | noOpt | O2 | strict | default | fastmathsafe | fastmath | full | fullsafe |
| :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `g++8` | 67.29  | 15.73  | n.a. | 15.74  | 15.03  | 13.06  | 12.58  | 14.49  |
| `g++9` | 67.73  | 15.69  | n.a. | 15.82  | 14.93  | 13.04  | 12.73  | 14.50  |
| `g++10.2` | 70.50  | 15.93  | n.a. | 15.59  | 15.03  | 12.94  | 12.66  | 14.54  |
| `g++10.3` | 70.87  | 15.85  | n.a. | 15.64  | 15.08  | 12.93  | 12.72  | 14.56  |
| `g++11.1` | 71.17  | 15.55  | n.a. | 15.57  | 15.00  | 13.08  | 12.72  | 14.37  |
| `g++11.2` | 70.89  | 15.69  | n.a. | 15.51  | 14.93  | 13.01  | 12.71  | 14.33  |
| `clang++9.0` | 69.27  | 15.85  | n.a. | 15.76  | 15.68  | 13.47  | 13.51  | 15.80  |
| `clang++10.0` | 69.15  | 15.84  | n.a. | 15.75  | 15.72  | 15.49  | 15.06  | 15.49  |
| `clang++11.0` | 69.32  | 15.73  | n.a. | 15.84  | 15.61  | 15.43  | 15.04  | 15.34  |
| `clang++12.0` | 68.76  | 15.67  | n.a. | 15.57  | 15.41  | 13.58  | 13.39  | 15.04  |
| `clang++13.0` | 68.27  | 15.64  | n.a. | 15.70  | 15.44  | 13.59  | 13.19  | 15.14  |
| `icpx2021.4` | 64.16  | 10.21  | 10.21  | 9.84  | 9.56  | 9.40  | 9.21  | 9.19  |
| `icpc19.0` | 82.49  | 9.52  | 9.60  | 9.51  | 9.46  | 9.40  | 9.47  | 9.38  |
| `icpc19.1` | 82.53  | 9.51  | 9.53  | 9.48  | 9.57  | 9.63  | 9.26  | 9.38  |
| `icpc21.4` | 83.01  | 9.48  | 9.54  | 9.34  | 9.48  | 9.62  | 9.40  | 9.34  |

## 2) Profiling

All results shown in this section have been obtained on the Intel Xeon E5-2660 v4 system, measurements are done with `perf`

### gcc with fastmath vs. fasthmath + nofinitemath

The image below shows profiling of the reaction rate calculation with `gcc 10.3` and `O3+fastmath`. About 39% of the total runtime is spent on calling `__ieee754_exp_fma`. Additionally, special versions of other functions (`log10_finite` and `exp_finite`) appear as well.

**Figure 1: `gcc 10.3` and `O3+fastmath`**
![pic](https://github.com/g3bk47/CanteraCompilerPerformance/blob/main/gcc_fastmath.png?raw=true)

The next image shows the profiling with `gcc 10.3` and `O3+fastmath+nofinitemath`. In addition to the calls to `__ieee754_exp_fma`, 12% of the runtime are spent on calling `__GI___exp`, which seems to be a version of `exp` with additional error handling.

**Figure 2: `gcc 10.3` and `O3+fastmath+nofinitemath`**
![pic](https://github.com/g3bk47/CanteraCompilerPerformance/blob/main/gcc_nofinitemath.png?raw=true)

Interestingly, both programs yield the exact same bitwise results, but a performance gain of >10% can be observed when using `fastmath` without `nofinitemath`.

### Intel vs. gcc

The Intel compiler finds more opportunities to optimize. For example, the picture below is the generated assembly for `updateTemp` from https://github.com/Cantera/cantera/blob/ad213c45a39eb0ba39b2f4e418518371d822cc11/src/kinetics/Falloff.cpp#L184-L188. 

**Figure 3: Optizations done by the Intel compiler**
![pic](https://github.com/g3bk47/CanteraCompilerPerformance/blob/main/4_annotated.png?raw=true)

The two calls to the exponential function at the beginning are merged into a single call of a vectorized version of that function. Interestingly, pretty much the same assembly is generated for `O3`, `O3+fp-model fast+fastmath` and `O3+fp-model fast+fastmath+nofinitemath`. Gcc and clang, on the other hand, always generate machine code with three exponential function calls. See also here for a direct comparison: https://godbolt.org/z/zMhaEPdYM
