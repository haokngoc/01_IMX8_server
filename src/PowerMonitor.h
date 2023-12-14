/*
 * PowerMonitor.h
 *
 *  Created on: Dec 14, 2023
 *      Author: hk
 */

#ifndef POWERMONITOR_H_
#define POWERMONITOR_H_

class PowerMonitor {
private:
    double battery_vol;
    double battery_ampere;
    double battery_avg_ampere;
    double capacity_remain;
    double capacity_full_charge;
    double charging_current;
    int battery_status;
    int cycle_count;
    int serial;
public:
    PowerMonitor(double vol, double ampere, double avgAmpere, double remain, double fullCharge,
            double chargeCurrent, int status, int count, int serialNum);
    double get_battery_vol() const;
	double get_battery_ampere() const;
	double get_avg_ampere() const;
	double get_remain() const;
};




#endif /* POWERMONITOR_H_ */
