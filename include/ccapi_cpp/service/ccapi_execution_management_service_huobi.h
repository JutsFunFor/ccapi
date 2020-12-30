#ifndef INCLUDE_CCAPI_CPP_SERVICE_CCAPI_EXECUTION_MANAGEMENT_SERVICE_HUOBI_H_
#define INCLUDE_CCAPI_CPP_SERVICE_CCAPI_EXECUTION_MANAGEMENT_SERVICE_HUOBI_H_
#ifdef CCAPI_ENABLE_SERVICE_EXECUTION_MANAGEMENT
#ifdef CCAPI_ENABLE_EXCHANGE_HUOBI
#include "ccapi_cpp/service/ccapi_execution_management_service.h"
namespace ccapi {
class ExecutionManagementServiceHuobi CCAPI_FINAL : public ExecutionManagementService {
 public:
  ExecutionManagementServiceHuobi(std::function<void(Event& event)> eventHandler, SessionOptions sessionOptions,
                                      SessionConfigs sessionConfigs, ServiceContextPtr serviceContextPtr)
      : ExecutionManagementService(eventHandler, sessionOptions, sessionConfigs, serviceContextPtr) {
    CCAPI_LOGGER_FUNCTION_ENTER;
    this->name = CCAPI_EXCHANGE_NAME_HUOBI;
    this->baseUrlRest = this->sessionConfigs.getUrlRestBase().at(this->name);
    this->setHostFromUrl(this->baseUrlRest);
    this->apiKeyName = CCAPI_HUOBI_API_KEY;
    this->apiSecretName = CCAPI_HUOBI_API_SECRET;
    this->setupCredential({this->apiKeyName, this->apiSecretName});
    this->createOrderTarget = "/v1/order/orders/place";
    this->cancelOrderTarget = "/v1/order/orders/{order-id}/submitcancel";
    this->cancelOrderByClientOrderIdTarget = "/v1/order/orders/submitCancelClientOrder";
    this->getOrderTarget = "/v1/order/orders/{order-id}";
    this->getOrderByClientOrderIdTarget = "/v1/order/orders/getClientOrder";
    this->getOpenOrdersTarget = "/v1/order/openOrders";
    CCAPI_LOGGER_FUNCTION_EXIT;
  }

 protected:
  void signRequest(http::request<http::string_body>& req, const std::string& path, const std::map<std::string, std::string>& queryParamMap, const std::map<std::string, std::string>& credential) {
    std::string preSignedText;
    preSignedText += std::string(request.method_string());
    preSignedText += "\n";
    preSignedText += this->host;
    preSignedText += "\n";
    preSignedText += path;
    preSignedText += "\n";
    std::string queryString;
    int i = 0;
    for (const auto& kv : queryParamMap) {
      queryString += kv.first;
      queryString += "=";
      queryString += kv.second;
      if (i < queryParamMap.size() - 1) {
        queryString += "&";
      }
    }
    preSignedText += queryString;
    auto apiSecret = mapGetWithDefault(credential, this->apiSecretName, {});
    auto signature = UtilAlgorithm::base64Encode(Hmac::hmac(Hmac::ShaVersion::SHA256, apiSecret, preSignedText));
    queryString += "&signature=";
    queryString += Url::urlEncode(signature);
    req.target(path + "?" + queryString);
  }
  void appendParam(rj::Document& document, rj::Document::AllocatorType& allocator, const std::map<std::string, std::string>& param, const std::map<std::string, std::string> regularizationMap = {}) {
    for (const auto& kv : param) {
      auto key = regularizationMap.find(kv.first) != regularizationMap.end() ? regularizationMap.at(kv.first) : kv.first;
      auto value = kv.second;
      if (key == "type") {
        value = value == CCAPI_EM_ORDER_SIDE_BUY ? "buy-limit" : "sell-limit";
      }
      document.AddMember(rj::Value(key.c_str(), allocator).Move(), rj::Value(value.c_str(), allocator).Move(), allocator);
    }
  }
  void appendParam(std::map<std::string, std::string>& queryParamMap, const std::map<std::string, std::string>& param, const std::map<std::string, std::string> regularizationMap = {}) {
    for (const auto& kv : param) {
      queryParamMap.insert(
          std::make_pair(
              regularizationMap.find(kv.first) != regularizationMap.end() ? regularizationMap.at(kv.first) : kv.first,
              Url::urlEncode(kv.second)
          )
      );
    }
  }
  void appendSymbolId(rj::Document& document, rj::Document::AllocatorType& allocator, const std::string symbolId) {
    document.AddMember("symbol", rj::Value(symbolId.c_str(), allocator).Move(), allocator);
  }
  void appendSymbolId(std::map<std::string, std::string>& queryParamMap, const std::string symbolId) {
    queryParamMap.insert(std::make_pair("symbol", Url::urlEncode(symbol)));
  }
  void convertReq(const Request& request, const TimePoint& now, http::request<http::string_body>& req, const std::map<std::string, std::string>& credential, const std::string& symbolId, const Request::Operation operation) override {
    req.set(beast::http::field::content_type, "application/json");
    auto apiKey = mapGetWithDefault(credential, this->apiKeyName, {});
    std::map<std::string, std::string> queryParamMap;
    queryParamMap.insert(std::make_pair("AccessKeyId", apiKey));
    queryParamMap.insert(std::make_pair("SignatureMethod", "HmacSHA256"));
    queryParamMap.insert(std::make_pair("SignatureVersion", "2"));
    std::string timestamp = UtilTime::getISOTimestamp<std::chrono::seconds>(now, "%FT");
    queryParamMap.insert(std::make_pair("Timestamp", Url::urlEncode(timestamp)));
    switch (operation) {
      case Request::Operation::CREATE_ORDER:
      {
        req.method(http::verb::post);
        const std::map<std::string, std::string>& param = request.getParamList().at(0);
        rj::Document document;
        document.SetObject();
        rj::Document::AllocatorType& allocator = document.GetAllocator();
        this->appendParam(document, allocator, param, {
            {CCAPI_EM_ORDER_SIDE , "type"},
            {CCAPI_EM_ORDER_QUANTITY , "amount"},
            {CCAPI_EM_ORDER_LIMIT_PRICE , "price"},
            {CCAPI_EM_CLIENT_ORDER_ID , "client-order-id"},
            {CCAPI_EM_ACCOUNT_ID , "account-id"}
        });
        this->appendSymbolId(document, allocator, symbolId);
        rj::StringBuffer stringBuffer;
        rj::Writer<rj::StringBuffer> writer(stringBuffer);
        document.Accept(writer);
        auto body = stringBuffer.GetString();
        req.body() = body;
        req.prepare_payload();
        this->signRequest(req, this->createOrderTarget, queryParamMap, credential);
      }
      break;
      case Request::Operation::CANCEL_ORDER:
      {
        req.method(http::verb::post);
        const std::map<std::string, std::string>& param = request.getParamList().at(0);
        auto shouldUseOrderId = param.find(CCAPI_EM_ORDER_ID) != param.end();
        std::string id = shouldUseOrderId ? param.at(CCAPI_EM_ORDER_ID)
            : param.find(CCAPI_EM_CLIENT_ORDER_ID) != param.end() ? "client:" + param.at(CCAPI_EM_CLIENT_ORDER_ID)
            : "";
        auto target = shouldUseOrderId ? std::regex_replace(this->cancelOrderTarget, std::regex("\\{order\\-id\\}"), id) : this->cancelOrderTargetByClientId;
        if (!shouldUseOrderId) {
          rj::Document document;
          document.SetObject();
          rj::Document::AllocatorType& allocator = document.GetAllocator();
          this->appendParam(document, allocator, param, {
              {CCAPI_EM_CLIENT_ORDER_ID , "client-order-id"}
          });
          rj::StringBuffer stringBuffer;
          rj::Writer<rj::StringBuffer> writer(stringBuffer);
          document.Accept(writer);
          auto body = stringBuffer.GetString();
          req.body() = body;
          req.prepare_payload();
        }
        this->signRequest(req, target, queryParamMap, credential);
      }
      break;
      case Request::Operation::GET_ORDER:
      {
        req.method(http::verb::get);
        const std::map<std::string, std::string>& param = request.getParamList().at(0);
        auto shouldUseOrderId = param.find(CCAPI_EM_ORDER_ID) != param.end();
        std::string id = shouldUseOrderId ? param.at(CCAPI_EM_ORDER_ID)
            : param.find(CCAPI_EM_CLIENT_ORDER_ID) != param.end() ? "client:" + param.at(CCAPI_EM_CLIENT_ORDER_ID)
            : "";
        auto target = shouldUseOrderId ? std::regex_replace(this->getOrderTarget, std::regex("\\{order\\-id\\}"), id) : this->getOrderTargetByClientId;
        req.target(target);
        if (!shouldUseOrderId) {
          this->appendParam(queryParamMap, param, {
              {CCAPI_EM_ACCOUNT_ID , "account-id"}
          });
          queryParamMap.insert(std::make_pair("clientOrderId", Url::urlEncode(id)));
        }
        this->signRequest(req, target, queryParamMap, credential);
      }
      break;
      case Request::Operation::GET_OPEN_ORDERS:
      {
        req.method(http::verb::get);
        const std::map<std::string, std::string>& param = request.getParamList().at(0);
        this->appendParam(queryParamMap, param, {
            {CCAPI_EM_ACCOUNT_ID , "account-id"}
        });
        if (!symbolId.empty()) {
          this->appendSymbolId(queryParamMap, symbolId);
        }
        this->signRequest(req, this->getOpenOrdersTarget, queryParamMap, credential);
      }
      break;
      default:
      CCAPI_LOGGER_FATAL(CCAPI_UNSUPPORTED_VALUE);
    }
  }
  std::vector<Element> extractOrderInfo(const Request::Operation operation, const rj::Document& document) override {
    const std::map<std::string, std::pair<std::string, JsonDataType> >& extractionFieldNameMap = {
      {CCAPI_EM_ORDER_ID, std::make_pair("id", JsonDataType::STRING)},
      {CCAPI_EM_CLIENT_ORDER_ID, std::make_pair("client_oid", JsonDataType::STRING)},
      {CCAPI_EM_ORDER_SIDE, std::make_pair("side", JsonDataType::STRING)},
      {CCAPI_EM_ORDER_QUANTITY, std::make_pair("size", JsonDataType::STRING)},
      {CCAPI_EM_ORDER_LIMIT_PRICE, std::make_pair("price", JsonDataType::STRING)},
      {CCAPI_EM_ORDER_CUMULATIVE_FILLED_QUANTITY, std::make_pair("filled_size", JsonDataType::STRING)},
      {CCAPI_EM_ORDER_CUMULATIVE_FILLED_PRICE_TIMES_QUANTITY, std::make_pair("executed_value", JsonDataType::STRING)},
      {CCAPI_EM_ORDER_STATUS, std::make_pair("status", JsonDataType::STRING)},
      {CCAPI_EM_ORDER_INSTRUMENT, std::make_pair("product_id", JsonDataType::STRING)}
    };
    std::vector<Element> elementList;
    if (document.IsObject()) {
      elementList.emplace_back(ExecutionManagementService::extractOrderInfo(document, extractionFieldNameMap));
    } else {
      for (const auto& x : document.GetArray()) {
        elementList.emplace_back(ExecutionManagementService::extractOrderInfo(x, extractionFieldNameMap));
      }
    }
    return elementList;
  }
  std::string cancelOrderByClientOrderIdTarget;
  std::string getOrderByClientOrderIdTarget;
};
} /* namespace ccapi */
#endif
#endif
#endif  // INCLUDE_CCAPI_CPP_SERVICE_CCAPI_EXECUTION_MANAGEMENT_SERVICE_HUOBI_H_
