#pragma once
#include "WiFi.h"
struct DNSServer { bool start(uint16_t, const char *, IPAddress) { return true; } void processNextRequest() {} };
