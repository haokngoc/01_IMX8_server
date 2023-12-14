#include "PowerMonitor.h"

PowerMonitor::PowerMonitor(double vol, double ampere, double avgAmpere, double remain,
                           double fullCharge, double chargeCurrent, int status, int count, int serialNum)
    : battery_vol(vol), battery_ampere(ampere), battery_avg_ampere(avgAmpere),
      capacity_remain(remain), capacity_full_charge(fullCharge), charging_current(chargeCurrent),
      battery_status(status), cycle_count(count), serial(serialNum) {}

double PowerMonitor::get_battery_vol() const {
    return battery_vol;
}

double PowerMonitor::get_battery_ampere() const {
    return battery_ampere;
}

double PowerMonitor::get_avg_ampere() const {
    return battery_avg_ampere;
}

double PowerMonitor::get_remain() const {
    return capacity_remain;
}
