#ifndef MCC_OBJECTS_H
#define MCC_OBJECTS_H

struct scenario_object_datum;
boolean mcc_vehicle_placement_rules(void);
boolean mcc_vehicle_placement_allowed(struct scenario_object_datum const *placement);

#endif
