#include "ccapi_cpp/ccapi_session.h"
#include <fstream>

namespace ccapi {
class MyEventHandler : public EventHandler {
 public:
  MyEventHandler(const std::string& fileName) : fileName(fileName) {
    // Open the CSV file and write the header
    csvFile.open(fileName);
    if (csvFile.is_open()) {
      csvFile << "Exchange,Instrument,Time,Bid Price,Bid Size,Ask Price,Ask Size\n";
    }
  }

  ~MyEventHandler() {
    if (csvFile.is_open()) {
      csvFile.close();
    }
  }

  bool processEvent(const Event& event, Session* session) override {
    if (event.getType() == Event::Type::SUBSCRIPTION_DATA) {
      for (const auto& message : event.getMessageList()) {
        std::string timestamp = UtilTime::getISOTimestamp(message.getTime());
        for (const auto& element : message.getElementList()) {
          const auto& elementNameValueMap = element.getNameValueMap();
          std::string exchange = message.getCorrelationIdList().empty() ? "" : message.getCorrelationIdList().at(0);
          std::string instrument = message.getInstrument();

          // Write to console
          std::cout << "Best bid and ask at " << timestamp << " on " << exchange << " for " << instrument << " are:\n";
          std::cout << "  " << toString(elementNameValueMap) << std::endl;

          // Write to CSV
          if (csvFile.is_open()) {
            csvFile << exchange << "," << instrument << "," << timestamp << ","
                    << elementNameValueMap.at("BID_PRICE") << "," << elementNameValueMap.at("BID_SIZE") << ","
                    << elementNameValueMap.at("ASK_PRICE") << "," << elementNameValueMap.at("ASK_SIZE") << "\n";
          }
        }
      }
    }
    return true;
  }

 private:
  std::ofstream csvFile;
  std::string fileName;
};
}  // namespace ccapi

using ::ccapi::MyEventHandler;
using ::ccapi::Session;
using ::ccapi::SessionConfigs;
using ::ccapi::SessionOptions;
using ::ccapi::Subscription;

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " <CSV_FILE> <DURATION_IN_SECONDS>\n";
    return EXIT_FAILURE;
  }

  std::string fileName = argv[1];
  int duration = std::stoi(argv[2]);

  ccapi::SessionOptions sessionOptions;
  ccapi::SessionConfigs sessionConfigs;
  MyEventHandler eventHandler(fileName);
  ccapi::Session session(sessionOptions, sessionConfigs, &eventHandler);

  // Subscriptions for multiple exchanges
  session.subscribe(ccapi::Subscription("okx", "BTC-USDT", "MARKET_DEPTH"));
  session.subscribe(ccapi::Subscription("binance", "ETH-USDT", "MARKET_DEPTH"));

  // Sleep for the specified duration
  std::this_thread::sleep_for(std::chrono::seconds(duration));
  session.stop();

  std::cout << "Data collection completed. Saved to " << fileName << "\n";
  return EXIT_SUCCESS;
}
