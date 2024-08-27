#include "ccapi_cpp/ccapi_session.h"
#include <fstream>
#include <map>
#include <filesystem>  // C++17 feature
#include <vector>
#include <string>

namespace ccapi {
Logger* Logger::logger = nullptr;  // This line is needed.

class MyEventHandler : public EventHandler {
 public:
  MyEventHandler(const std::string& directory, const std::vector<std::string>& exchanges)
      : directory(directory), exchanges(exchanges) {
    // Ensure the directory exists
    std::filesystem::create_directories(directory);

    // Initialize CSV files for each exchange
    for (const auto& exchange : exchanges) {
      std::string fileName = directory + "/" + exchange + "_market_data.csv";
      std::ofstream& outFile = outFileMap[exchange];
      outFile.open(fileName, std::ios_base::app);

      // Write the header if the file is empty
      if (outFile.tellp() == 0) {
        outFile << "Exchange,Instrument,Time,Bid Price,Bid Size,Ask Price,Ask Size\n";
      }
    }
  }

  ~MyEventHandler() {
    // Close all open files
    for (auto& pair : outFileMap) {
      if (pair.second.is_open()) {
        pair.second.close();
      }
    }
  }

  bool processEvent(const Event& event, Session* session) override {
    if (event.getType() == Event::Type::SUBSCRIPTION_DATA) {
      for (const auto& message : event.getMessageList()) {
        std::string exchange = message.getCorrelationIdList().empty() ? "" : message.getCorrelationIdList().at(0);
        std::string timestamp = UtilTime::getISOTimestamp(message.getTime());

        for (const auto& element : message.getElementList()) {
          const std::map<std::string, std::string>& elementNameValueMap = element.getNameValueMap();

          // Extract bid and ask price/size
          std::string bidPrice = elementNameValueMap.count("BID_PRICE") ? elementNameValueMap.at("BID_PRICE") : "";
          std::string bidSize = elementNameValueMap.count("BID_SIZE") ? elementNameValueMap.at("BID_SIZE") : "";
          std::string askPrice = elementNameValueMap.count("ASK_PRICE") ? elementNameValueMap.at("ASK_PRICE") : "";
          std::string askSize = elementNameValueMap.count("ASK_SIZE") ? elementNameValueMap.at("ASK_SIZE") : "";

          // Write the data to the appropriate CSV file
          std::ofstream& outFile = outFileMap[exchange];
          if (outFile.is_open()) {
            outFile << exchange << "," << timestamp << "," 
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
  std::vector<std::string> exchanges;
  std::map<std::string, std::ofstream> outFileMap;  // Map to store file streams for each exchange
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
  
  std::vector<std::string> exchanges;
  std::vector<ccapi::Subscription> subscriptions;

  // Parse exchanges and instruments from command-line arguments
  for (int i = 3; i < argc; ++i) {
    std::string arg = argv[i];
    size_t pos = arg.find(":");
    if (pos != std::string::npos) {
      std::string exchange = arg.substr(0, pos);
      std::string instrument = arg.substr(pos + 1);
      exchanges.push_back(exchange);
      subscriptions.emplace_back(exchange, instrument, "MARKET_DEPTH");
    } else {
      std::cerr << "Invalid format for exchange:instrument pair: " << arg << "\n";
      return EXIT_FAILURE;
    }
  }

  ccapi::SessionOptions sessionOptions;
  ccapi::SessionConfigs sessionConfigs;
  MyEventHandler eventHandler(directory, exchanges);
  ccapi::Session session(sessionOptions, sessionConfigs, &eventHandler);

  // Subscribe to all exchanges and instruments
  session.subscribe(subscriptions);

  // Sleep for the specified duration
  std::this_thread::sleep_for(std::chrono::seconds(duration));
  session.stop();

  std::cout << "Data collection completed. Files saved in " << directory << "\n";
  return EXIT_SUCCESS;
}
