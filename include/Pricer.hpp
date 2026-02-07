#ifndef PRICER_HPP
#define PRICER_HPP

#include "MarketParameters.hpp"
#include "MonteCarloSimulator.hpp"
#include "OptionParameters.hpp"

struct PriceAndDelta {
  double price;
  double priceStd;
  double delta;
};

struct PricingResult {
  double
      price; // V = S(T) - min S(t) or max S(t) - S(T) depending on Call / Put
  double priceStd;
  double delta; // \partial V / \partial S
  double gamma; // \partial2 V / \partial S2
  double
      theta;  // - \partial V / \partial T (up to sign depending on conventions)
  double rho; // \partial V / \partial r
  double vega; // \partia V / \partial \sigma
};

class PricerLookbackOption {
private:
  LookbackOption option_;
  MarketParameters MarketParameters_;
  MonteCarloSimulator simulator_;

  // Bump sizes for finite difference Greeks
  static constexpr double SPOT_BUMP = 0.01;        // 1% for Delta/Gamma
  static constexpr double VOL_BUMP = 0.01;         // 1% absolute for Vega
  static constexpr double RATE_BUMP = 0.0001;      // 1bp for Rho
  static constexpr double TIME_BUMP = 1.0 / 365.0; // 1 day for Theta

  // Helper to compute raw price from simulations, private as it is used
  // internally for Greeks computation
  double computeRawPrice(double spot, double rate, double volatility,
                         double maturity, double &stdError);

public:
  PricerLookbackOption();
  PricerLookbackOption(LookbackOption option, MarketParameters marketParameters,
                       MonteCarloSimulator simulator);

  void setOption(LookbackOption option);
  void setOption(OptionType type, double maturity);

  void setMarketParameters(MarketParameters marketParameters);
  void setMarketParameters(double spot, double rate, double volatility);

  void setMonteCarloSimulator(unsigned long numSimulations,
                              unsigned long numTimeSteps,
                              unsigned long seed = 0);

  const LookbackOption &getOption() const;
  const MarketParameters &getMarketParameters() const;
  const MonteCarloSimulator &getSimulator() const;

  // Simulation of price and Greeks using monte carlo simulator
  PricingResult compute();

  // Simulation of price and Delta for a given spot S => for graphs
  PriceAndDelta priceAndDeltaForSpot(double spot);
};

#endif // !PRICER_HPP
