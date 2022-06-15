/**
 * @file       TinyGsmClientSIM76xx.h
 * @author     Nedko Boshkilov
 * @license    LGPL-3.0
 * @copyright  Copyright (c) 2022 Nedko Boshkilov
 * @date       Feb 2022
 */

#ifndef SRC_TINYGSMCLIENTSIM76XX_H_
#define SRC_TINYGSMCLIENTSIM76XX_H_

// #define TINY_GSM_DEBUG Serial
// #define TINY_GSM_USE_HEX

#include "TinyGsmBattery.tpp"
#include "TinyGsmCalling.tpp"
#include "TinyGsmGPRS.tpp"
#include "TinyGsmGSMLocation.tpp"
#include "TinyGsmModem.tpp"
#include "TinyGsmSMS.tpp"
#include "TinyGsmTCP.tpp"
#include "TinyGsmTemperature.tpp"
#include "TinyGsmTime.tpp"
#include "TinyGsmNTP.tpp"


#define GSM_NL "\r\n"
static const char GSM_OK[] TINY_GSM_PROGMEM    = "OK" GSM_NL;
static const char GSM_ERROR[] TINY_GSM_PROGMEM = "ERROR" GSM_NL;
#if defined       TINY_GSM_DEBUG
static const char GSM_CME_ERROR[] TINY_GSM_PROGMEM = GSM_NL "+CME ERROR:";
static const char GSM_CMS_ERROR[] TINY_GSM_PROGMEM = GSM_NL "+CMS ERROR:";
#endif

enum RegStatus {
  REG_NO_RESULT    = -1,
  REG_UNREGISTERED = 0,
  REG_SEARCHING    = 2,
  REG_DENIED       = 3,
  REG_OK_HOME      = 1,
  REG_OK_ROAMING   = 5,
  REG_UNKNOWN      = 4,
};

template <class modemType>
class TinyGsmSim76xx : public TinyGsmModem<TinyGsmSim76xx<modemType>>,
                       public TinyGsmGPRS<TinyGsmSim76xx<modemType>>,
                       public TinyGsmSMS<TinyGsmSim76xx<modemType>>,
                       public TinyGsmGSMLocation<TinyGsmSim76xx<modemType>>,
                       public TinyGsmTime<TinyGsmSim76xx<modemType>>,
                       public TinyGsmNTP<TinyGsmSim76xx<modemType>>,
                       public TinyGsmBattery<TinyGsmSim76xx<modemType>>,
                       public TinyGsmTemperature<TinyGsmSim76xx<modemType>>,
                       public TinyGsmCalling<TinyGsmSim76xx<modemType>> {
  friend class TinyGsmModem<TinyGsmSim76xx<modemType>>;
  friend class TinyGsmGPRS<TinyGsmSim76xx<modemType>>;
  friend class TinyGsmSMS<TinyGsmSim76xx<modemType>>;
  friend class TinyGsmGSMLocation<TinyGsmSim76xx<modemType>>;
  friend class TinyGsmTime<TinyGsmSim76xx<modemType>>;
  friend class TinyGsmNTP<TinyGsmSim76xx<modemType>>;
  friend class TinyGsmBattery<TinyGsmSim76xx<modemType>>;
  friend class TinyGsmTemperature<TinyGsmSim76xx<modemType>>;
  friend class TinyGsmCalling<TinyGsmSim76xx<modemType>>;

  /*
   * CRTP Helper
   */
 protected:
  inline const modemType& thisModem() const {
    return static_cast<const modemType&>(*this);
  }
  inline modemType& thisModem() {
    return static_cast<modemType&>(*this);
  }

  /*
   * Constructor
   */
 public:
  explicit TinyGsmSim76xx(Stream& stream) : stream(stream) {}

  /*
   * Basic functions
   */
 protected:
  bool initImpl(const char* pin = NULL) {
    return thisModem().initImpl(pin);
  }

  String getModemNameImpl() {
    String name = "SIMCom SIM76xx";

    thisModem().sendAT(GF("+CGMM"));
    String res2;
    if (waitResponse(1000L, res2) != 1) { return name; }
    res2.replace(GSM_NL "OK" GSM_NL, "");
    res2.replace("_", " ");
    res2.trim();

    name = res2;
    DBG("### Modem:", name);
    return name;
  }

  bool factoryDefaultImpl() {  // these commands aren't supported
    return false;
  }

  /*
   * Power functions
   */
 protected:
  bool restartImpl(const char* pin = NULL) {
    if (!thisModem().testAT()) { return false; }
    thisModem().sendAT(GF("+CRESET"));
    // After booting, modem sends out messages as each of its
    // internal modules loads.  The final message is "PB DONE".
    if (waitResponse(40000L, GF(GSM_NL "PB DONE")) != 1) { return false; }
    return thisModem().initImpl(pin);
  }

  bool powerOffImpl() {
    thisModem().sendAT(GF("+CPOF"));
    return waitResponse() == 1;
  }

  bool radioOffImpl() {
    if (!thisModem().setPhoneFunctionality(4)) { return false; }
    delay(3000);
    return true;
  }

  bool sleepEnableImpl(bool enable = true) {
    thisModem().sendAT(GF("+CSCLK="), enable);
    return waitResponse() == 1;
  }

  bool setPhoneFunctionalityImpl(uint8_t fun, bool reset = false) {
    thisModem().sendAT(GF("+CFUN="), fun, reset ? ",1" : "");
    return waitResponse(10000L) == 1;
  }

  /*
   * Generic network functions
   */
 public:
  RegStatus getRegistrationStatus() {
    return (RegStatus)thisModem().getRegistrationStatusXREG("CGREG");
  }

 protected:
  bool isNetworkConnectedImpl() {
    RegStatus s = getRegistrationStatus();
    return (s == REG_OK_HOME || s == REG_OK_ROAMING);
  }

 public:
  String getNetworkModes() {
    thisModem().sendAT(GF("+CNMP=?"));
    if (waitResponse(GF(GSM_NL "+CNMP:")) != 1) { return ""; }
    String res = stream.readStringUntil('\n');
    waitResponse();
    return res;
  }

  int16_t getNetworkMode() {
    thisModem().sendAT(GF("+CNMP?"));
    if (waitResponse(GF(GSM_NL "+CNMP:")) != 1) { return false; }
    int16_t mode = thisModem().streamGetIntBefore('\n');
    waitResponse();
    return mode;
  }

  bool setNetworkMode(uint8_t mode) {
    thisModem().sendAT(GF("+CNMP="), mode);
    return waitResponse() == 1;
  }

  String getLocalIPImpl() {
    return thisModem().getLocalIPImpl();
  }

  /*
   * GPRS functions
   */
 protected:
  bool gprsConnectImpl(const char* apn, const char* user = NULL,
                       const char* pwd = NULL) {
    return thisModem().gprsConnectImpl(apn, user, pwd);
  }

  bool gprsDisconnectImpl() {
    return thisModem().gprsDisconnectImpl();
  }

  bool isGprsConnectedImpl() {
    return thisModem().isGprsConnectedImpl();
  }

  /*
   * SIM card functions
   */
 protected:
  // Gets the CCID of a sim card via AT+CCID
  String getSimCCIDImpl() {
    thisModem().sendAT(GF("+CICCID"));
    if (waitResponse(GF(GSM_NL "+ICCID:")) != 1) { return ""; }
    String res = stream.readStringUntil('\n');
    waitResponse();
    res.trim();
    return res;
  }

  /*
   * Phone Call functions
   */
 protected:
  bool callHangupImpl() {
    thisModem().sendAT(GF("+CHUP"));
    return waitResponse() == 1;
  }

  /*
   * Messaging functions
   */
 protected:
  // Follows all messaging functions per template

  /*
   * GSM Location functions
   */
 protected:
  // Can return a GSM-based location from CLBS as per the template

  /*
   * Time functions
   */
 protected:
  // Can follow the standard CCLK function in the template

  /*
   * NTP server functions
   */
  // Can sync with server using CNTP as per template

  /*
   * Battery functions
   */
 protected:
  // returns volts, multiply by 1000 to get mV
  uint16_t getBattVoltageImpl() {
    thisModem().sendAT(GF("+CBC"));
    if (waitResponse(GF(GSM_NL "+CBC:")) != 1) { return 0; }

    // get voltage in VOLTS
    float voltage = thisModem().streamGetFloatBefore('\n');
    // Wait for final OK
    waitResponse();
    // Return millivolts
    uint16_t res = voltage * 1000;
    return res;
  }

  int8_t getBattPercentImpl() TINY_GSM_ATTR_NOT_AVAILABLE;

  uint8_t getBattChargeStateImpl() TINY_GSM_ATTR_NOT_AVAILABLE;

  bool getBattStatsImpl(uint8_t& chargeState, int8_t& percent,
                        uint16_t& milliVolts) {
    chargeState = 0;
    percent     = 0;
    milliVolts  = thisModem().getBattVoltage();
    return true;
  }

  /*
   * Temperature functions
   */
 protected:
  // get temperature in degree celsius
  uint16_t getTemperatureImpl() {
    thisModem().sendAT(GF("+CPMUTEMP"));
    if (waitResponse(GF(GSM_NL "+CPMUTEMP:")) != 1) { return 0; }
    // return temperature in C
    uint16_t res = thisModem().streamGetIntBefore('\n');
    // Wait for final OK
    waitResponse();
    return res;
  }

  /*
   * Utilities
   */
 public:
  // should implement in sub-classes
  int8_t waitResponse(uint32_t timeout_ms, String& data,
                      GsmConstStr r1 = GFP(GSM_OK),
                      GsmConstStr r2 = GFP(GSM_ERROR),
#if defined TINY_GSM_DEBUG
                      GsmConstStr r3 = GFP(GSM_CME_ERROR),
                      GsmConstStr r4 = GFP(GSM_CMS_ERROR),
#else
                      GsmConstStr r3 = NULL, GsmConstStr r4 = NULL,
#endif
                      GsmConstStr r5 = NULL) {
    return thisModem().waitResponse(timeout_ms, data, r1, r2, r3, r4, r5);
  }

  int8_t waitResponse(uint32_t timeout_ms, GsmConstStr r1 = GFP(GSM_OK),
                      GsmConstStr r2 = GFP(GSM_ERROR),
#if defined TINY_GSM_DEBUG
                      GsmConstStr r3 = GFP(GSM_CME_ERROR),
                      GsmConstStr r4 = GFP(GSM_CMS_ERROR),
#else
                      GsmConstStr r3 = NULL, GsmConstStr r4 = NULL,
#endif
                      GsmConstStr r5 = NULL) {
    String data;
    return thisModem().waitResponse(timeout_ms, data, r1, r2, r3, r4, r5);
  }

  int8_t waitResponse(GsmConstStr r1 = GFP(GSM_OK),
                      GsmConstStr r2 = GFP(GSM_ERROR),
#if defined TINY_GSM_DEBUG
                      GsmConstStr r3 = GFP(GSM_CME_ERROR),
                      GsmConstStr r4 = GFP(GSM_CMS_ERROR),
#else
                      GsmConstStr r3 = NULL, GsmConstStr r4 = NULL,
#endif
                      GsmConstStr r5 = NULL) {
    return thisModem().waitResponse(1000, r1, r2, r3, r4, r5);
  }

 public:
  Stream& stream;

 protected:
  const char* gsmNL = GSM_NL;
};

#endif  // SRC_TINYGSMCLIENTSIM76xx_H_
