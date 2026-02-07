#include "../include/AnalyticLookback.hpp"
#include "../include/Pricer.hpp"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

void printUsage(const char *programName) {
  std::cout << "Usage: " << programName << " [options]" << std::endl;
  std::cout << std::endl;
  std::cout << "Generate Price(S) and Delta(S) graphs for lookback options."
            << std::endl;
  std::cout << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "  --type TYPE       Option type: call or put (default: call)"
            << std::endl;
  std::cout << "  -T, --maturity T  Time to maturity in years (default: 1.0)"
            << std::endl;
  std::cout << "  -r, --rate RATE   Risk-free interest rate (default: 0.05)"
            << std::endl;
  std::cout << "  -v, --vol SIGMA   Volatility (default: 0.20)" << std::endl;
  std::cout << "  -n, --sims NUM    Number of MC simulations (default: 100000)"
            << std::endl;
  std::cout << "  -m, --steps NUM   Number of time steps (default: 100)"
            << std::endl;
  std::cout << "  --seed NUM        Random seed (default: 0 = random)"
            << std::endl;
  std::cout << "  --smin PRICE      Minimum spot price (default: 50)"
            << std::endl;
  std::cout << "  --smax PRICE      Maximum spot price (default: 200)"
            << std::endl;
  std::cout << "  --points NUM      Number of spot points (default: 30)"
            << std::endl;
  std::cout << "  --analytic        Also plot analytic solution" << std::endl;
  std::cout << "  -o, --output DIR  Output directory (default: .)" << std::endl;
  std::cout << "  -h, --help        Show this help message" << std::endl;
}

int main(int argc, char *argv[]) {
  OptionType type = OptionType::Call;
  double maturity = 1.0;
  double rate = 0.05;
  double volatility = 0.20;
  unsigned long numSims = 100000;
  int numSteps = 100;
  unsigned int seed = 0;
  double smin = 50.0;
  double smax = 200.0;
  int numPoints = 30;
  bool analytic = false;
  std::string outputDir = ".";

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-h" || arg == "--help") {
      printUsage(argv[0]);
      return 0;
    } else if (arg == "--type" && i + 1 < argc) {
      std::string t = argv[++i];
      if (t == "call" || t == "Call" || t == "CALL")
        type = OptionType::Call;
      else if (t == "put" || t == "Put" || t == "PUT")
        type = OptionType::Put;
      else {
        std::cerr << "Error: Invalid option type '" << t << "'" << std::endl;
        return 1;
      }
    } else if ((arg == "-T" || arg == "--maturity") && i + 1 < argc) {
      maturity = std::atof(argv[++i]);
    } else if ((arg == "-r" || arg == "--rate") && i + 1 < argc) {
      rate = std::atof(argv[++i]);
    } else if ((arg == "-v" || arg == "--vol") && i + 1 < argc) {
      volatility = std::atof(argv[++i]);
    } else if ((arg == "-n" || arg == "--sims") && i + 1 < argc) {
      numSims = static_cast<unsigned long>(std::atol(argv[++i]));
    } else if ((arg == "-m" || arg == "--steps") && i + 1 < argc) {
      numSteps = std::atoi(argv[++i]);
    } else if (arg == "--seed" && i + 1 < argc) {
      seed = static_cast<unsigned int>(std::atoi(argv[++i]));
    } else if (arg == "--smin" && i + 1 < argc) {
      smin = std::atof(argv[++i]);
    } else if (arg == "--smax" && i + 1 < argc) {
      smax = std::atof(argv[++i]);
    } else if (arg == "--points" && i + 1 < argc) {
      numPoints = std::atoi(argv[++i]);
    } else if (arg == "--analytic") {
      analytic = true;
    } else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
      outputDir = argv[++i];
    } else {
      std::cerr << "Error: Unknown argument '" << arg << "'" << std::endl;
      printUsage(argv[0]);
      return 1;
    }
  }

  if (maturity <= 0 || smin <= 0 || smax <= smin || numPoints < 2) {
    std::cerr << "Error: Invalid parameters (maturity>0, 0<smin<smax, "
                 "points>=2)"
              << std::endl;
    return 1;
  }

  std::string typeStr = (type == OptionType::Call) ? "Call" : "Put";

  std::cout << "Generating graphs for " << typeStr << " Lookback" << std::endl;
  std::cout << "  T=" << maturity << "y, r=" << rate << ", vol=" << volatility
            << ", sims=" << numSims << std::endl;
  std::cout << "  Spot range: [" << smin << ", " << smax << "] with "
            << numPoints << " points" << std::endl;
  if (analytic)
    std::cout << "  Analytic solution: enabled" << std::endl;
  std::cout << std::endl;

  // Setup pricer (spot is a placeholder, will be overridden per point)
  PricerLookbackOption pricer;
  pricer.setOption(type, maturity);
  pricer.setMarketParameters(smin, rate, volatility);
  pricer.setMonteCarloSimulator(numSims, numSteps, seed);

  // Storage
  std::vector<double> spots(numPoints);
  std::vector<double> mcPrices(numPoints);
  std::vector<double> mcPriceStds(numPoints);
  std::vector<double> mcDeltas(numPoints);
  std::vector<double> anPrices(numPoints);
  std::vector<double> anDeltas(numPoints);

  double step = (smax - smin) / (numPoints - 1);

  for (int i = 0; i < numPoints; ++i) {
    double s = smin + i * step;
    spots[i] = s;

    std::cout << "\r  Computing S=" << std::fixed << std::setprecision(1) << s
              << " (" << i + 1 << "/" << numPoints << ")..." << std::flush;

    PriceAndDelta pd = pricer.priceAndDeltaForSpot(s);
    mcPrices[i] = pd.price;
    mcPriceStds[i] = pd.priceStd;
    mcDeltas[i] = pd.delta;

    if (analytic) {
      anPrices[i] = LookbackAnalyticSolution::price(type, s, maturity,
                                                     volatility, rate);
      anDeltas[i] = LookbackAnalyticSolution::delta(type, s, maturity,
                                                     volatility, rate);
    }
  }
  std::cout << std::endl << "Done." << std::endl;

  // Write CSV
  std::string csvPath = outputDir + "/graph_data.csv";
  std::ofstream csv(csvPath);
  if (!csv.is_open()) {
    std::cerr << "Error: Cannot write to " << csvPath << std::endl;
    return 1;
  }

  csv << "Spot,MC_Price,MC_PriceStd,MC_Delta";
  if (analytic)
    csv << ",Analytic_Price,Analytic_Delta";
  csv << "\n";

  csv << std::fixed << std::setprecision(6);
  for (int i = 0; i < numPoints; ++i) {
    csv << spots[i] << "," << mcPrices[i] << "," << mcPriceStds[i] << ","
        << mcDeltas[i];
    if (analytic)
      csv << "," << anPrices[i] << "," << anDeltas[i];
    csv << "\n";
  }
  csv.close();
  std::cout << "CSV written to " << csvPath << std::endl;

  // Generate plots with gnuplot
  std::string pricePng = outputDir + "/price_vs_spot.png";
  std::string deltaPng = outputDir + "/delta_vs_spot.png";

  // Check if gnuplot is available
  if (std::system("which gnuplot > /dev/null 2>&1") != 0) {
    std::cout << "gnuplot not found, skipping plot generation." << std::endl;
    std::cout << "Install gnuplot and re-run, or plot manually from "
              << csvPath << std::endl;
    return 0;
  }

  // --- Price graph ---
  {
    FILE *gp = popen("gnuplot", "w");
    if (!gp) {
      std::cerr << "Error: failed to launch gnuplot" << std::endl;
      return 0;
    }

    fprintf(gp, "set terminal pngcairo size 900,600 enhanced font 'Arial,12'\n");
    fprintf(gp, "set output '%s'\n", pricePng.c_str());
    fprintf(gp, "set title '%s Lookback Option Price P(S, T_0)  "
                "(T=%.2f, r=%.2f, {/Symbol s}=%.2f)'\n",
            typeStr.c_str(), maturity, rate, volatility);
    fprintf(gp, "set xlabel 'Spot Price S'\n");
    fprintf(gp, "set ylabel 'Option Price P'\n");
    fprintf(gp, "set grid\n");
    fprintf(gp, "set datafile separator ','\n");
    fprintf(gp, "set key top left\n");

    if (analytic) {
      fprintf(gp,
              "plot '%s' using 1:2 skip 1 with linespoints pt 7 ps 0.6 "
              "lc rgb '#0060ad' title 'Monte Carlo', "
              "'' using 1:2:3 skip 1 with yerrorbars lc rgb '#0060ad' "
              "notitle, "
              "'' using 1:5 skip 1 with lines lw 2 lc rgb '#dd181f' "
              "title 'Analytic'\n",
              csvPath.c_str());
    } else {
      fprintf(gp,
              "plot '%s' using 1:2 skip 1 with linespoints pt 7 ps 0.6 "
              "lc rgb '#0060ad' title 'Monte Carlo', "
              "'' using 1:2:3 skip 1 with yerrorbars lc rgb '#0060ad' "
              "notitle\n",
              csvPath.c_str());
    }
    pclose(gp);
    std::cout << "Price graph saved to " << pricePng << std::endl;
  }

  // --- Delta graph ---
  {
    FILE *gp = popen("gnuplot", "w");
    if (!gp) {
      std::cerr << "Error: failed to launch gnuplot" << std::endl;
      return 0;
    }

    fprintf(gp, "set terminal pngcairo size 900,600 enhanced font 'Arial,12'\n");
    fprintf(gp, "set output '%s'\n", deltaPng.c_str());
    fprintf(gp, "set title '%s Lookback Option Delta {/Symbol D}(S, T_0)  "
                "(T=%.2f, r=%.2f, {/Symbol s}=%.2f)'\n",
            typeStr.c_str(), maturity, rate, volatility);
    fprintf(gp, "set xlabel 'Spot Price S'\n");
    fprintf(gp, "set ylabel 'Delta {/Symbol D}'\n");
    fprintf(gp, "set grid\n");
    fprintf(gp, "set datafile separator ','\n");
    fprintf(gp, "set key top left\n");

    if (analytic) {
      fprintf(gp,
              "plot '%s' using 1:4 skip 1 with linespoints pt 7 ps 0.6 "
              "lc rgb '#0060ad' title 'Monte Carlo', "
              "'' using 1:6 skip 1 with lines lw 2 lc rgb '#dd181f' "
              "title 'Analytic'\n",
              csvPath.c_str());
    } else {
      fprintf(gp,
              "plot '%s' using 1:4 skip 1 with linespoints pt 7 ps 0.6 "
              "lc rgb '#0060ad' title 'Monte Carlo'\n",
              csvPath.c_str());
    }
    pclose(gp);
    std::cout << "Delta graph saved to " << deltaPng << std::endl;
  }

  return 0;
}
