#ifndef ANALYTIC_LOOKBACK_HPP
#define ANALYTIC_LOOKBACK_HPP
#include "OptionParameters.hpp"

class LookbackAnalyticSolution {
public:
  static double price(OptionType option, double S0, double T, double sigma,
                      double r);

  static double delta(OptionType option, double S0, double T, double sigma,
                      double r);

  static double gamma(OptionType option, double S0, double T, double sigma,
                      double r);

  static double vega(OptionType option, double S0, double T, double sigma,
                     double r);

  static double theta(OptionType option, double S0, double T, double sigma,
                      double r);

  static double rho(OptionType option, double S0, double T, double sigma,
                    double r);

private:
  static double delta_plus(double tau, double s, double r, double sigma);
  static double delta_minus(double tau, double s, double r, double sigma);

  // The cumulative distribution function (CDF) and probability density function
  // (PDF)
  static double normalCDF(double x);
  static double normalPDF(double x);
};
#endif
