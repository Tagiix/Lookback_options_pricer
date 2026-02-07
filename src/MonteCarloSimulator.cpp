#include "../include/MonteCarloSimulator.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>

// for random number generation, we used some code from
// https://www.learncpp.com/cpp-tutorial/global-random-numbers-random-h/
namespace Random {
inline std::mt19937 generate() {
  std::random_device rd{};

  // Create seed_seq with clock and 7 random numbers from std::random_device
  std::seed_seq ss{
      static_cast<std::seed_seq::result_type>(
          std::chrono::steady_clock::now().time_since_epoch().count()),
      rd(),
      rd(),
      rd(),
      rd(),
      rd(),
      rd(),
      rd()};

  return std::mt19937{ss};
}
} // namespace Random

MonteCarloSimulator::MonteCarloSimulator()
    : numSimulations_(10000), numTimeSteps_(1000), rng_(Random::generate()),
      normDist_(0.0, 1.0) {}

MonteCarloSimulator::MonteCarloSimulator(unsigned long numSimulations,
                                         unsigned long numTimeSteps,
                                         unsigned int seed)
    : numSimulations_(numSimulations), numTimeSteps_(numTimeSteps),
      normDist_(0.0, 1.0) {
  if (seed == 0) {
    rng_ = Random::generate();
  } else {
    rng_.seed(seed);
  }
}

void MonteCarloSimulator::setNumSimulations(unsigned long numSimulations) {
  numSimulations_ = numSimulations;
}

void MonteCarloSimulator::setNumTimeSteps(unsigned long numTimeSteps) {
  numTimeSteps_ = numTimeSteps;
}

void MonteCarloSimulator::setRandomSeed(unsigned int seed) {
  if (seed == 0) {
    rng_ = Random::generate();
  } else {
    rng_.seed(seed);
  }
}

unsigned long MonteCarloSimulator::getNumSimulations() const {
  return numSimulations_;
}

unsigned long MonteCarloSimulator::getNumTimeSteps() const {
  return numTimeSteps_;
}

double MonteCarloSimulator::generateStandardNormal() const {
  return normDist_(rng_);
}

SpotResults
MonteCarloSimulator::simulateSinglePath(double initialSpot, double rate,
                                        double volatility,
                                        double timeToMaturity) const {

  SpotResults results;

  double dt = timeToMaturity / numTimeSteps_;
  double S = initialSpot;
  double S_max = S;
  double S_min = S;

  double drift = (rate - 0.5 * volatility * volatility) * dt;
  double sigma = volatility * std::sqrt(dt);

  for (unsigned long i = 0; i < numTimeSteps_; ++i) {
    double dW = generateStandardNormal();
    S *= std::exp(drift + sigma * dW);
    S_max = std::max(S_max, S);
    S_min = std::min(S_min, S);
  }

  results.spotAtMaturity = S;
  results.maximumSpot = S_max;
  results.minimumSpot = S_min;

  return results;
}

std::vector<SpotResults>
MonteCarloSimulator::simulatePaths(double initialSpot, double rate,
                                   double volatility,
                                   double timeToMaturity) const {
  std::vector<SpotResults> allResults;
  allResults.reserve(numSimulations_);

  for (unsigned long i = 0; i < numSimulations_; ++i) {
    allResults.push_back(
        simulateSinglePath(initialSpot, rate, volatility, timeToMaturity));
  }
  return allResults;
}
