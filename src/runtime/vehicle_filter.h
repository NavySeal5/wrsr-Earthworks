#pragma once

namespace ew {
bool AllowEarthworksVehicleType(void* vehicleType, char* rejectedType, unsigned rejectedTypeSize);
bool IsEarthworksVehicleType(void* vehicleType);
bool IsGroundworksMachineType(void* vehicleType);
}
