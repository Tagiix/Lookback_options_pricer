#ifndef MARKET_PARAMETERS_HPP
#define MARKET_PARAMETERS_HPP

class MarketParameters {
public:
  MarketParameters();
  MarketParameters(double spot, double riskFreeRate, double volatility);
  ~MarketParameters() = default;

  void setSpot(double spot);
  void setRiskFreeRate(double riskFreeRate);
  void setVolatility(double volatility);

  double getSpot() const;
  double gtetRiskFreeRate() const;
  double getVolatility() const;

private:
  double spot_;
  double riskFreeRate_;
  double volatility_;
};

#endif // !MARKET_PARAMETERS_HPP
