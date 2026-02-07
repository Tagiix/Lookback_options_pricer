#ifndef OPTION_PARAMETERS_HPP
#define OPTION_PARAMETERS_HPP

enum class OptionType { Call, Put };

class LookbackOption {
private:
  OptionType optionType_;
  double maturity_;

public:
  LookbackOption();
  LookbackOption(OptionType optionType, double maturity);

  void setType(OptionType optionType);
  void setMaturity(double maturity);

  double getMaturity() const;
  OptionType getType() const;

  // Payoff function for lookback option : depends on the type of option and the
  // extremum of the spot price during the life of the option For a call option,
  // the payoff is max(0, SpotAtMaturity - min(SpotPrice during life of option))
  // For a put option, the payoff is max(0, max(SpotPrice during life of option)
  // - SpotAtMaturity)
  double payoff(double extremumOfSpot, double SpotAtMaturity) const;
};

#endif // OPTION_PARAMETERS_HPP
