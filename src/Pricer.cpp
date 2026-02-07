#include "../include/Pricer.hpp"

#include <cmath>
#include <numeric>

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

// Stderror is passed by reference to return both price and its standard error
// from the same function
double PricerLookbackOption::computeRawPrice(double spot, double rate,
                                             double volatility, double maturity,
                                             double &stdError) {
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
  return price;
}

PricingResult PricerLookbackOption::compute() {
  PricingResult result;
  double stdError;
  // Compute price and standard error for the base parameters
  result.price = computeRawPrice(
      MarketParameters_.getSpot(), MarketParameters_.gtetRiskFreeRate(),
      MarketParameters_.getVolatility(), option_.getMaturity(), stdError);
  result.priceStd = stdError;

  // Compute Greeks using finite differences

  // Delta = dV/dS ~ (V(S+ds) - V(S-ds)) / (2*ds)
  double priceUp = computeRawPrice(
      MarketParameters_.getSpot() * (1 + SPOT_BUMP),
      MarketParameters_.gtetRiskFreeRate(), MarketParameters_.getVolatility(),
      option_.getMaturity(), stdError);
  double priceDown = computeRawPrice(
      MarketParameters_.getSpot() * (1 - SPOT_BUMP),
      MarketParameters_.gtetRiskFreeRate(), MarketParameters_.getVolatility(),
      option_.getMaturity(), stdError);
  result.delta =
      (priceUp - priceDown) / (2 * MarketParameters_.getSpot() * SPOT_BUMP);

  // Gamma = d2V/dS2 ~ (V(S+ds) - 2*V(S) + V(S-ds)) / (ds^2)
  result.gamma = (priceUp - 2 * result.price + priceDown) /
                 (MarketParameters_.getSpot() * MarketParameters_.getSpot() *
                  SPOT_BUMP * SPOT_BUMP);

  // Theta = -dV/dT ~ (V(T+dt) - V(T-dt)) / (2*dt)
  double priceForward = computeRawPrice(
      MarketParameters_.getSpot(), MarketParameters_.gtetRiskFreeRate(),
      MarketParameters_.getVolatility(), option_.getMaturity() + TIME_BUMP,
      stdError);
  double priceBackward = computeRawPrice(
      MarketParameters_.getSpot(), MarketParameters_.gtetRiskFreeRate(),
      MarketParameters_.getVolatility(), option_.getMaturity() - TIME_BUMP,
      stdError);
  result.theta = (priceBackward - priceForward) / (2 * TIME_BUMP);

  // Rho = dV/dr ~ (V(r+dr) - V(r-dr)) / (2*dr)
  double priceUpRate = computeRawPrice(
      MarketParameters_.getSpot(),
      MarketParameters_.gtetRiskFreeRate() + RATE_BUMP,
      MarketParameters_.getVolatility(), option_.getMaturity(), stdError);
  double priceDownRate = computeRawPrice(
      MarketParameters_.getSpot(),
      MarketParameters_.gtetRiskFreeRate() - RATE_BUMP,
      MarketParameters_.getVolatility(), option_.getMaturity(), stdError);
  result.rho = (priceUpRate - priceDownRate) / (2 * RATE_BUMP);

  // Vega = dV/dsigma ~ (V(sigma+dv) - V(sigma-dv)) / (2*dv)
  double priceUpVol = computeRawPrice(
      MarketParameters_.getSpot(), MarketParameters_.gtetRiskFreeRate(),
      MarketParameters_.getVolatility() + VOL_BUMP, option_.getMaturity(),
      stdError);
  double priceDownVol = computeRawPrice(
      MarketParameters_.getSpot(), MarketParameters_.gtetRiskFreeRate(),
      MarketParameters_.getVolatility() - VOL_BUMP, option_.getMaturity(),
      stdError);
  result.vega = (priceUpVol - priceDownVol) / (2 * VOL_BUMP);

  return result;
}

PriceAndDelta PricerLookbackOption::priceAndDeltaForSpot(double spot) {
  double stdError;
  double price = computeRawPrice(spot, MarketParameters_.gtetRiskFreeRate(),
                                 MarketParameters_.getVolatility(),
                                 option_.getMaturity(), stdError);
  double error = stdError;
  // Compute Delta using finite difference
  double priceUp = computeRawPrice(
      spot * (1 + SPOT_BUMP), MarketParameters_.gtetRiskFreeRate(),
      MarketParameters_.getVolatility(), option_.getMaturity(), stdError);
  double priceDown = computeRawPrice(
      spot * (1 - SPOT_BUMP), MarketParameters_.gtetRiskFreeRate(),
      MarketParameters_.getVolatility(), option_.getMaturity(), stdError);
  double delta = (priceUp - priceDown) / (2 * spot * SPOT_BUMP);
  return {price, error, delta};
}
