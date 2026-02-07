#ifndef MONTE_CARLO_SIMULATOR_HPP
#define MONTE_CARLO_SIMULATOR_HPP

#include <random>
#include <vector>

struct SpotResults {
  double spotAtMaturity;
  double maximumSpot;
  double minimumSpot;
};

class MonteCarloSimulator {
private:
  unsigned long numSimulations_;
  unsigned long numTimeSteps_;
  mutable std::mt19937 rng_;
  mutable std::normal_distribution<double>
      normDist_; // norm_Dist_ = (\mu = 0, \sigma = 1)

public:
  MonteCarloSimulator();
  MonteCarloSimulator(unsigned long numSimulations, unsigned long numTimeSteps,
                      unsigned int seed = 0);

  void setNumSimulations(unsigned long numSimulations);
  void setNumTimeSteps(unsigned long numTimeSteps);
  void setRandomSeed(unsigned int seed);

  unsigned long getNumSimulations() const;
  unsigned long getNumTimeSteps() const;

  SpotResults simulateSinglePath(double initialSpot, double rate,
                                 double volatility,
                                 double timeToMaturity) const;

  std::vector<SpotResults> simulatePaths(double initialSpot, double rate,
                                         double volatility,
                                         double timeToMaturity) const;

  double generateStandardNormal() const;
};
#endif // !MONTE_CARLO_SIMULATOR_HPP
