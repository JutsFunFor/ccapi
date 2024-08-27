#include "ccapi_cpp/ccapi_session.h"
#include <fstream>
#include <map>
#include <filesystem>
#include <vector>
#include <string>
#include <iostream>

namespace ccapi {
Logger* Logger::logger = nullptr;  // This line is needed.

class MyEventHandler : public EventHandler {
 public:
  MyEventHandler(const std::string& directory)
      : directory(directory) {
    std::filesystem::create_directories(directory);
  }

  ~MyEventHandler() {
    for (auto& pair : outFileMap) {
      if (pair.second.is_open()) {
        pair.second.close();
      }
    }
  }

  bool processEvent(const Event& event, Session* session) override {
    if (event.getType() == Event::Type::SUBSCRIPTION_STATUS) {
      std::cout << "Subscription status event received:\n" + event.toStringPretty(2, 2) << std::endl;
    } else if (event.getType() == Event::Type::SUBSCRIPTION_DATA) {
      for (const auto& message : event.getMessageList()) {
        std::string exchange = message.getCorrelationIdList().empty() ? "" : message.getCorrelationIdList().at(0);
        std::string instrument = message.getInstrument().empty() ? "UNKNOWN" : message.getInstrument();
        std::string timestamp = UtilTime::getISOTimestamp(message.getTime());

        std::string filename = exchange + "_" + instrument + "_market_data.csv";
        std::ofstream& outFile = outFileMap[filename];
        
        // If the file is not yet opened, open it and write the header
        if (!outFile.is_open()) {
          outFile.open(directory + "/" + filename, std::ios_base::app);
          outFile << "Exchange,Instrument,Time,Bid Price,Bid Size,Ask Price,Ask Size\n";
          std::cout << "Started logging data for " << exchange << " " << instrument << " to " << filename << std::endl;
        }

        for (const auto& element : message.getElementList()) {
          const std::map<std::string, std::string>& elementNameValueMap = element.getNameValueMap();
          std::string bidPrice = elementNameValueMap.count("BID_PRICE") ? elementNameValueMap.at("BID_PRICE") : "";
          std::string bidSize = elementNameValueMap.count("BID_SIZE") ? elementNameValueMap.at("BID_SIZE") : "";
          std::string askPrice = elementNameValueMap.count("ASK_PRICE") ? elementNameValueMap.at("ASK_PRICE") : "";
          std::string askSize = elementNameValueMap.count("ASK_SIZE") ? elementNameValueMap.at("ASK_SIZE") : "";

          // Write the data to the CSV file
          if (outFile.is_open()) {
            outFile << exchange << "," << instrument << "," << timestamp << "," 
                    << bidPrice << "," << bidSize << ","
                    << askPrice << "," << askSize << "\n";
          }
        }
      }
    }
    return true;
  }

 private:
  std::string directory;
  std::map<std::string, std::ofstream> outFileMap;  // Map to store file streams for each exchange-instrument pair
};
}  // namespace ccapi

using ::ccapi::MyEventHandler;
using ::ccapi::Session;
using ::ccapi::SessionConfigs;
using ::ccapi::SessionOptions;
using ::ccapi::Subscription;

int main(int argc, char** argv) {
  if (argc < 4) {
    std::cerr << "Usage: " << argv[0] << " <CSV_DIRECTORY> <DURATION_IN_SECONDS> <EXCHANGE:INSTRUMENT>...\n";
    return EXIT_FAILURE;
  }

  std::string directory = argv[1];
  int duration = std::stoi(argv[2]);

  std::vector<ccapi::Subscription> subscriptions;

  // Parse exchanges and instruments from command-line arguments
  for (int i = 3; i < argc; ++i) {
    std::string arg = argv[i];
    size_t pos = arg.find(":");
    if (pos != std::string::npos) {
      std::string exchange = arg.substr(0, pos);
      std::string instrument = arg.substr(pos + 1);
      subscriptions.emplace_back(exchange, instrument, "MARKET_DEPTH");
      std::cout << "Setting up subscription for " << exchange << " " << instrument << std::endl;
    } else {
      std::cerr << "Invalid format for exchange:instrument pair: " << arg << "\n";
      return EXIT_FAILURE;
    }
  }

  ccapi::SessionOptions sessionOptions;
  ccapi::SessionConfigs sessionConfigs;
  MyEventHandler eventHandler(directory);
  ccapi::Session session(sessionOptions, sessionConfigs, &eventHandler);

  // Subscribe to all exchanges and instruments
  session.subscribe(subscriptions);
  std::cout << "All subscriptions set up successfully. Collecting data...\n";

  // Sleep for the specified duration
  std::this_thread::sleep_for(std::chrono::seconds(duration));
  session.stop();

  std::cout << "Data collection completed. Files saved in " << directory << "\n";
  return EXIT_SUCCESS;
}
