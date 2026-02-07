#include "../include/Pricer.hpp"

#include <cmath>
#include <numeric>
#include <random>

PricerLookbackOption::PricerLookbackOption() {}

PricerLookbackOption::PricerLookbackOption(LookbackOption option,
                                           MarketParameters marketParameters,
                                           MonteCarloSimulator simulator)
    : option_(option), MarketParameters_(marketParameters),
      simulator_(simulator) {}

void PricerLookbackOption::setOption(LookbackOption option) {
  option_ = option;
}
void PricerLookbackOption::setOption(OptionType type, double maturity) {
  option_.setType(type);
  option_.setMaturity(maturity);
}

void PricerLookbackOption::setMarketParameters(
    MarketParameters marketParameters) {
  MarketParameters_ = marketParameters;
}

void PricerLookbackOption::setMarketParameters(double spot, double rate,
                                               double volatility) {
  MarketParameters_.setSpot(spot);
  MarketParameters_.setRiskFreeRate(rate);
  MarketParameters_.setVolatility(volatility);
}

void PricerLookbackOption::setMonteCarloSimulator(unsigned long numSimulations,
                                                  unsigned long numTimeSteps,
                                                  unsigned long seed) {
  simulator_.setNumSimulations(numSimulations);
  simulator_.setNumTimeSteps(numTimeSteps);
  simulator_.setRandomSeed(seed);
}

const LookbackOption &PricerLookbackOption::getOption() const {
  return option_;
}
const MarketParameters &PricerLookbackOption::getMarketParameters() const {
  return MarketParameters_;
}
const MonteCarloSimulator &PricerLookbackOption::getSimulator() const {
  return simulator_;
}

// Helper to compute standard error from a vector of per-simulation values
static double computeStdError(const std::vector<double> &values) {
  size_t N = values.size();
  double mean = std::accumulate(values.begin(), values.end(), 0.0) / N;
  double variance =
      std::accumulate(values.begin(), values.end(), 0.0,
                      [mean](double sum, double val) {
                        return sum + (val - mean) * (val - mean);
                      }) /
      (N - 1);
  return std::sqrt(variance / N);
}

// Stderror is passed by reference to return both price and its standard error
// from the same function.
// If storeValues is true, the individual discounted payoffs are stored in
// values, which allows computing standard errors for Greeks via finite
// differences on per-simulation values.
double PricerLookbackOption::computeRawPrice(double spot, double rate,
                                             double volatility, double maturity,
                                             double &stdError,
                                             unsigned int seed,
                                             std::vector<double> &values,
                                             bool storeValues) {
  simulator_.setRandomSeed(seed);
  // Simulate paths
  auto paths = simulator_.simulatePaths(spot, rate, volatility, maturity);
  // Compute payoffs
  std::vector<double> payoffs(paths.size());
  for (size_t i = 0; i < paths.size(); ++i) {
    if (option_.getType() == OptionType::Call) {
      payoffs[i] =
          option_.payoff(paths[i].minimumSpot, paths[i].spotAtMaturity);
    } else {
      payoffs[i] =
          option_.payoff(paths[i].maximumSpot, paths[i].spotAtMaturity);
    }
  }
  // Discount payoffs and compute price
  double discountFactor = std::exp(-rate * maturity);
  double price = discountFactor *
                 std::accumulate(payoffs.begin(), payoffs.end(), 0.0) /
                 payoffs.size();
  // Compute standard error
  double payoffMean =
      std::accumulate(payoffs.begin(), payoffs.end(), 0.0) / payoffs.size();
  double payoffVariance =
      std::accumulate(payoffs.begin(), payoffs.end(), 0.0,
                      [payoffMean](double sum, double payoff) {
                        return sum +
                               (payoff - payoffMean) * (payoff - payoffMean);
                      }) /
      (payoffs.size() - 1);
  stdError = discountFactor * std::sqrt(payoffVariance / payoffs.size());

  // Store individual discounted payoffs if requested
  if (storeValues) {
    values.resize(payoffs.size());
    for (size_t i = 0; i < payoffs.size(); ++i) {
      values[i] = discountFactor * payoffs[i];
    }
  }

  return price;
}

PricingResult PricerLookbackOption::compute() {
  PricingResult result;
  // needed to use the same randomness for all price computations to ensure that
  // the differences are due to parameter changes and not random noise
  unsigned int seed = simulator_.getRandomSeed();
  double stdError;

  // Vectors to store per-simulation discounted payoffs for Greek std errors
  std::vector<double> baseValues, upSpotValues, downSpotValues;
  std::vector<double> forwardTimeValues, backwardTimeValues;
  std::vector<double> upRateValues, downRateValues;
  std::vector<double> upVolValues, downVolValues;

  // Compute price and standard error for the base parameters
  result.price = computeRawPrice(
      MarketParameters_.getSpot(), MarketParameters_.gtetRiskFreeRate(),
      MarketParameters_.getVolatility(), option_.getMaturity(), stdError, seed,
      baseValues, true);
  result.priceStd = stdError;

  size_t N = baseValues.size();

  // Compute Greeks using finite differences

  // Delta = dV/dS ~ (V(S+ds) - V(S-ds)) / (2*ds)
  double spotBumpAbs = MarketParameters_.getSpot() * SPOT_BUMP;
  computeRawPrice(MarketParameters_.getSpot() * (1 + SPOT_BUMP),
                  MarketParameters_.gtetRiskFreeRate(),
                  MarketParameters_.getVolatility(), option_.getMaturity(),
                  stdError, seed, upSpotValues, true);

  computeRawPrice(MarketParameters_.getSpot() * (1 - SPOT_BUMP),
                  MarketParameters_.gtetRiskFreeRate(),
                  MarketParameters_.getVolatility(), option_.getMaturity(),
                  stdError, seed, downSpotValues, true);

  result.delta = (std::accumulate(upSpotValues.begin(), upSpotValues.end(), 0.0) -
                  std::accumulate(downSpotValues.begin(), downSpotValues.end(), 0.0)) /
                 N / (2 * spotBumpAbs);

  // Compute per-simulation delta for std error
  std::vector<double> greekPerSim(N);
  for (size_t i = 0; i < N; ++i) {
    greekPerSim[i] =
        (upSpotValues[i] - downSpotValues[i]) / (2 * spotBumpAbs);
  }
  result.deltaStd = computeStdError(greekPerSim);

  // Gamma = d2V/dS2 ~ (V(S+ds) - 2*V(S) + V(S-ds)) / (ds^2)
  double spotBumpAbs2 = spotBumpAbs * spotBumpAbs;
  result.gamma = (std::accumulate(upSpotValues.begin(), upSpotValues.end(), 0.0) -
                  2 * std::accumulate(baseValues.begin(), baseValues.end(), 0.0) +
                  std::accumulate(downSpotValues.begin(), downSpotValues.end(), 0.0)) /
                 N / spotBumpAbs2;

  for (size_t i = 0; i < N; ++i) {
    greekPerSim[i] =
        (upSpotValues[i] - 2 * baseValues[i] + downSpotValues[i]) / spotBumpAbs2;
  }
  result.gammaStd = computeStdError(greekPerSim);

  // Theta = dV/dT ~ (V(T+dt) - V(T-dt)) / (2*dt)
  computeRawPrice(MarketParameters_.getSpot(),
                  MarketParameters_.gtetRiskFreeRate(),
                  MarketParameters_.getVolatility(),
                  option_.getMaturity() + TIME_BUMP, stdError, seed,
                  forwardTimeValues, true);

  computeRawPrice(MarketParameters_.getSpot(),
                  MarketParameters_.gtetRiskFreeRate(),
                  MarketParameters_.getVolatility(),
                  option_.getMaturity() - TIME_BUMP, stdError, seed,
                  backwardTimeValues, true);

  result.theta = (std::accumulate(forwardTimeValues.begin(), forwardTimeValues.end(), 0.0) -
                  std::accumulate(backwardTimeValues.begin(), backwardTimeValues.end(), 0.0)) /
                 N / (2 * TIME_BUMP);

  for (size_t i = 0; i < N; ++i) {
    greekPerSim[i] =
        (forwardTimeValues[i] - backwardTimeValues[i]) / (2 * TIME_BUMP);
  }
  result.thetaStd = computeStdError(greekPerSim);

  // Rho = dV/dr ~ (V(r+dr) - V(r-dr)) / (2*dr)
  computeRawPrice(MarketParameters_.getSpot(),
                  MarketParameters_.gtetRiskFreeRate() + RATE_BUMP,
                  MarketParameters_.getVolatility(), option_.getMaturity(),
                  stdError, seed, upRateValues, true);

  computeRawPrice(MarketParameters_.getSpot(),
                  MarketParameters_.gtetRiskFreeRate() - RATE_BUMP,
                  MarketParameters_.getVolatility(), option_.getMaturity(),
                  stdError, seed, downRateValues, true);

  result.rho = (std::accumulate(upRateValues.begin(), upRateValues.end(), 0.0) -
                std::accumulate(downRateValues.begin(), downRateValues.end(), 0.0)) /
               N / (2 * RATE_BUMP);

  for (size_t i = 0; i < N; ++i) {
    greekPerSim[i] =
        (upRateValues[i] - downRateValues[i]) / (2 * RATE_BUMP);
  }
  result.rhoStd = computeStdError(greekPerSim);

  // Vega = dV/dsigma ~ (V(sigma+dv) - V(sigma-dv)) / (2*dv)
  double volBumpAbs = MarketParameters_.getVolatility() * VOL_BUMP;
  computeRawPrice(MarketParameters_.getSpot(),
                  MarketParameters_.gtetRiskFreeRate(),
                  MarketParameters_.getVolatility() * (1.0 + VOL_BUMP),
                  option_.getMaturity(), stdError, seed, upVolValues, true);

  computeRawPrice(MarketParameters_.getSpot(),
                  MarketParameters_.gtetRiskFreeRate(),
                  MarketParameters_.getVolatility() * (1.0 - VOL_BUMP),
                  option_.getMaturity(), stdError, seed, downVolValues, true);

  result.vega = (std::accumulate(upVolValues.begin(), upVolValues.end(), 0.0) -
                 std::accumulate(downVolValues.begin(), downVolValues.end(), 0.0)) /
                N / (2 * volBumpAbs);

  for (size_t i = 0; i < N; ++i) {
    greekPerSim[i] =
        (upVolValues[i] - downVolValues[i]) / (2 * volBumpAbs);
  }
  result.vegaStd = computeStdError(greekPerSim);

  return result;
}

PriceAndDelta PricerLookbackOption::priceAndDeltaForSpot(double spot) {
  unsigned int seed = simulator_.getRandomSeed();
  double stdError;
  std::vector<double> unused;
  double price = computeRawPrice(spot, MarketParameters_.gtetRiskFreeRate(),
                                 MarketParameters_.getVolatility(),
                                 option_.getMaturity(), stdError, seed, unused);
  double error = stdError;
  // Compute Delta using finite difference
  double priceUp = computeRawPrice(
      spot * (1 + SPOT_BUMP), MarketParameters_.gtetRiskFreeRate(),
      MarketParameters_.getVolatility(), option_.getMaturity(), stdError, seed,
      unused);
  double priceDown = computeRawPrice(
      spot * (1 - SPOT_BUMP), MarketParameters_.gtetRiskFreeRate(),
      MarketParameters_.getVolatility(), option_.getMaturity(), stdError, seed,
      unused);
  double delta = (priceUp - priceDown) / (2 * spot * SPOT_BUMP);
  return {price, error, delta};
}
