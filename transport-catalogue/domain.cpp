#include <string>
#include <vector>

#include "domain.h"

namespace domain {

Stop::Stop(std::string name, double latitude, double longitude)
: stop_name(std::move(name))
, stop_coordinates({latitude, longitude})
{
}

Bus::Bus(std::string bus, std::vector<Stop*> stops)
: bus_name(std::move(bus))
, bus_stops(std::move(stops))
{
}

}  // namespace domain