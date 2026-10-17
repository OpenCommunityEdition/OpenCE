#ifndef MCC_OBJECTS_H
#define MCC_OBJECTS_H

struct scenario_object_datum;
struct powered_mass_point_datum;
union real_rgb_color;
/* Called only for the boat controller's three-point scratch array. */
void mcc_boat_controls_initialize(struct powered_mass_point_datum *state);
boolean mcc_vehicle_placement_rules(void);
boolean mcc_item_small_scale(real scale);
boolean mcc_vehicle_placement_allowed(struct scenario_object_datum const *placement);
boolean mcc_object_color_extrapolate(union real_rgb_color *result, unsigned long flags,
    union real_rgb_color const *lower, union real_rgb_color const *upper, real fraction);

#endif
