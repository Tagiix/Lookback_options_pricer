# Lookback Option Pricer

C++ pricer for **floating-strike lookback options** combining Monte Carlo simulation and the closed-form Goldman-Sosin-Gatto analytic formula.

## Features

- Monte Carlo engine with common random numbers for Greek estimation (Delta, Gamma, Vega, Theta, Rho)
- Analytic solution (Goldman-Sosin-Gatto) for continuous monitoring
- Broadie-Glasserman-Kou (`--bgk`) continuity correction to reduce discrete-monitoring bias
- Spot-sweep tool (`graph`) generating Price(S) and Delta(S) PNG plots via gnuplot
- Excel/LibreOffice interface (`excel/`) calling the pricer through VBA

## Build

Requires a C++17 compiler and gnuplot (optional, for plots).

```bash
cd src

# Single evaluation tool
g++ -std=c++17 -O2 -o pricer \
    cli.cpp Pricer.cpp MonteCarloSimulator.cpp \
    OptionParameters.cpp MarketParameters.cpp AnalyticLookback.cpp

# Spot-sweep / graph tool
g++ -std=c++17 -O2 -o graph \
    graph.cpp Pricer.cpp MonteCarloSimulator.cpp \
    OptionParameters.cpp MarketParameters.cpp AnalyticLookback.cpp
```

## Usage

```bash
# Price a call expiring 2027-01-15
./pricer --type call -s 100 -t 2027-01-15 -r 0.05 -v 0.20 -n 100000

# Same with BGK continuity correction
./pricer --type call -s 100 -T 1.0 -r 0.05 -v 0.20 -n 100000 --bgk

# Spot sweep and generate PNG graphs
./graph --type call -T 1.0 -r 0.05 -v 0.20
```

Key options for `pricer`:

| Flag | Description | Default |
|---|---|---|
| `--type` | `call` or `put` | `call` |
| `-s` | Spot price | `100` |
| `-t DATE` | Maturity date (`YYYY-MM-DD`) | — |
| `-T` | Time to maturity (years) | `1.0` |
| `-r` | Risk-free rate | `0.05` |
| `-v` | Volatility | `1.0` |
| `-n` | Number of MC paths | `100000` |
| `-m` | Time steps per path | `100` |
| `--bgk` | BGK continuity correction | off |
| `--std_error` | Print standard errors on Greeks | off |

## Sample Output

```
Price (MC):       12.48   ± 0.04
Price (Analytic): 12.31
Delta (MC):        0.821
Gamma (MC):        0.014
Vega  (MC):       24.6
```

## Repository Structure

```
src/          C++ sources and headers
excel/        LookbackPricer.xlsm + VBA module (pricer_vba.bas)
docs/         Doxygen HTML documentation
lookback-options.pdf   Reference — analytic formula derivation
mfBGK.pdf              Reference — Broadie-Glasserman-Kou correction
```

## References

- Goldman, Sosin, Gatto (1979) — *Path Dependent Options: Buy at the Low, Sell at the High*
- Broadie, Glasserman, Kou (1999) — *Connecting Discrete and Continuous Path-Dependent Options*
