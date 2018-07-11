/*
  additional arming checks for plane
 */
#include "AP_Arming.h"
#include "Plane.h"

const AP_Param::GroupInfo AP_Arming_Plane::var_info[] = {
    // variables from parent vehicle
    AP_NESTEDGROUPINFO(AP_Arming, 0),

    // @Param: RUDDER
    // @DisplayName: Rudder Arming
    // @Description: Control arm/disarm by rudder input. When enabled arming is done with right rudder, disarming with left rudder. Rudder arming only works in manual throttle modes with throttle at zero +- deadzone (RCx_DZ)
    // @Values: 0:Disabled,1:ArmingOnly,2:ArmOrDisarm
    // @User: Advanced
    AP_GROUPINFO("RUDDER",       3,     AP_Arming_Plane,  rudder_arming_value,     ARMING_RUDDER_ARMONLY),

    AP_GROUPEND
};

/*
  additional arming checks for plane

 */
bool AP_Arming_Plane::pre_arm_checks(bool display_failure)
{
    // call parent class checks
    bool ret = AP_Arming::pre_arm_checks(display_failure);

    // Check airspeed sensor
    ret &= AP_Arming::airspeed_checks(display_failure);

    if (plane.g.fs_timeout_long < plane.g.fs_timeout_short && plane.g.fs_action_short != FS_ACTION_SHORT_DISABLED) {
        check_failed(ARMING_CHECK_NONE, display_failure, "FS_LONG_TIMEOUT < FS_SHORT_TIMEOUT");
        ret = false;
    }

    if (plane.aparm.roll_limit_cd < 300) {
        check_failed(ARMING_CHECK_NONE, display_failure, "LIM_ROLL_CD too small (%u)", plane.aparm.roll_limit_cd);
        ret = false;
    }

    if (plane.aparm.pitch_limit_max_cd < 300) {
        check_failed(ARMING_CHECK_NONE, display_failure, "LIM_PITCH_MAX too small (%u)", plane.aparm.pitch_limit_max_cd);
        ret = false;
    }

    if (plane.aparm.pitch_limit_min_cd > -300) {
        check_failed(ARMING_CHECK_NONE, display_failure, "LIM_PITCH_MIN too large (%u)", plane.aparm.pitch_limit_min_cd);
        ret = false;
    }

    if (plane.channel_throttle->get_reverse() && 
        plane.g.throttle_fs_enabled &&
        plane.g.throttle_fs_value < 
        plane.channel_throttle->get_radio_max()) {
        check_failed(ARMING_CHECK_NONE, display_failure, "Invalid THR_FS_VALUE for rev throttle");
        ret = false;
    }

    if (plane.quadplane.available() && plane.scheduler.get_loop_rate_hz() < 100) {
        check_failed(ARMING_CHECK_NONE, display_failure, "quadplane needs SCHED_LOOP_RATE >= 100");
        ret = false;
    }

    if (plane.control_mode == AUTO && plane.mission.num_commands() <= 1) {
        check_failed(ARMING_CHECK_NONE, display_failure, "No mission loaded");
        ret = false;
    }

    // check adsb avoidance failsafe
    if (plane.failsafe.adsb) {
        check_failed(ARMING_CHECK_NONE, display_failure, "ADSB threat detected");
        ret = false;
    }

#if HAVE_PX4_MIXER
    if (plane.last_mixer_crc == -1) {
        check_failed(ARMING_CHECK_NONE, display_failure, "Mixer error");
        ret = false;
    }
#endif // CONFIG_HAL_BOARD

    return ret;
}

bool AP_Arming_Plane::ins_checks(bool display_failure)
{
    // call parent class checks
    if (!AP_Arming::ins_checks(display_failure)) {
        return false;
    }

    // additional plane specific checks
    if ((checks_to_perform & ARMING_CHECK_ALL) ||
        (checks_to_perform & ARMING_CHECK_INS)) {
        if (!ahrs.healthy()) {
            const char *reason = ahrs.prearm_failure_reason();
            if (reason == nullptr) {
                reason = "AHRS not healthy";
            }
            check_failed(ARMING_CHECK_INS, display_failure, "%s", reason);
            return false;
        }
    }

    return true;
}
