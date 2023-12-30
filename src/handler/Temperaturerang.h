#ifndef _TEMPERATURERANG_H_
#define _TEMPERATURERANG_H_

#include <SinricProDevice.h>
#include <Capabilities/PowerStateController.h>
#include <Capabilities/RangeController.h>

class Temperaturerang 
: public SinricProDevice
, public PowerStateController<Temperaturerang>
, public RangeController<Temperaturerang> {
  friend class PowerStateController<Temperaturerang>;
  friend class RangeController<Temperaturerang>;
public:
  Temperaturerang(const String &deviceId) : SinricProDevice(deviceId, "Temperaturerang") {};
};

#endif
