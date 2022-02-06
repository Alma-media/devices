#include <Arduino.h>

#define RX_CALLBACK_SIGNATURE void (*callback)(char*)
#define DEFAULT_DEBOUNCE_INTERVAL 500

#pragma pack(push, 1)
class Debouncer {
  public:
    Debouncer(RX_CALLBACK_SIGNATURE, uint16_t interval = DEFAULT_DEBOUNCE_INTERVAL);
    Debouncer(RX_CALLBACK_SIGNATURE);

    ~Debouncer();

    Debouncer& setCallback(RX_CALLBACK_SIGNATURE);
    Debouncer& setInterval(uint16_t timeout);
    
    void click(const unsigned long);

  private:
    uint32_t last_id = 0;
    uint32_t last_ts = 0;
    uint16_t interval;
    RX_CALLBACK_SIGNATURE;

    void _click(const uint32_t id);
};
#pragma pack(pop)
