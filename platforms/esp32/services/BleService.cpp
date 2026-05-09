#include "BleService.hpp"

Status BleService::init(const char *)
{
#if defined(CONFIG_BT_ENABLED) && defined(CONFIG_BT_NIMBLE_ENABLED)
    return Status::Unsupported;
#else
    return Status::Unsupported;
#endif
}

Status BleService::startAdvertising(const BleAdvertisingConfig &)
{
    return Status::Unsupported;
}

Status BleService::addGattService(std::uint16_t)
{
    return Status::Unsupported;
}

bool BleService::available() const
{
#if defined(CONFIG_BT_ENABLED) && defined(CONFIG_BT_NIMBLE_ENABLED)
    return true;
#else
    return false;
#endif
}
