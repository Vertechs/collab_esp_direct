#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "driver/gptimer.h"
#include "soc/gpio_reg.h"
#include "cfg_dispatcher.h"
#include "esp_timer.h"

#define NUM_AXES 4

#define MAX_NAME_LENGTH 16
#define MAX_UNITS_LENGTH 4


typedef enum{
    AXIS_CTRL_HOLD = 0x00,
    AXIS_CTRL_VELOCITY = 0x01,
    AXIS_CTRL_POSITION = 0x02,
} saxis_state_t;


typedef struct{
    bool dir;
    bool en;
    bool step;
} saxis_pin_states_t;

typedef enum{
    AXIS_FAULT_OUTPUT_FAILED = 0x00,
    AXIS_FAULT_ODOM_LOST = 0x01,
    AXIS_FAULT_WD = 0x10

    //TODO more faults
}saxis_fault_code_t;

typedef struct{
    saxis_fault_code_t code;
    uint64_t time;
} saxis_fault_info;

typedef struct{
    gpio_num_t pin;
    bool level;
} saxis_homing_pin;

typedef struct{

    // discovery
    char name_str[MAX_NAME_LENGTH];
    char unit_str[MAX_UNITS_LENGTH];

    // enable state and command?
    bool enable : 1; // allowed to run, enable pin true

    // state flags
    bool initialized : 1; // data may not be valid if false, timer should not be allocated
    bool fault_okay : 1; // faulted when false, disable all pins, latch on, enable false
    bool homed : 1;

    // TODO, need these?
    bool running : 1; // tracking target, may be stationary if in idle or position control
    bool moving : 1; // timer running, step pin pulsed 

    // hardware definitions    
    gpio_num_t dir_pin;
    bool dir_pin_inv;
    gpio_num_t step_pin;
    gpio_num_t en_pin;
    uint16_t factor_microstep;

    saxis_homing_pin home_pin;

    // Control mode
    saxis_state_t mode;
    saxis_state_t mode_cmd;

    // Commands
    float target_velocity_cmd; // in EU, TODO set to 0 once eval?
    float target_position_cmd; // in EU

    // Parameters
    float factor_steps_per_unit;

    // Internal targets
    int32_t target_position_steps; // step pulses per second
    int32_t target_velocity_steps; // step pulses per second
    int8_t target_direction_actual; //-1 or 1, set when velocity is set

    // Odometry
    int32_t odom_steps_act;
    int32_t home_steps; // odom reset to 0 when homed, using this just for history
    saxis_fault_info fault_info;

    // resources
    gptimer_handle_t hardware_timer;
    uint64_t wd_last_scan_us;
    uint32_t wd_timeout_us;
    saxis_pin_states_t pin_states; //always set when writing to pin, dont read pin in case electrical fault
    uint32_t vel_timer_period;

}saxis_t;


err_json_tuple_t saxis_set_dir_pin(saxis_t *axis, gpio_num_t pin_num);
err_json_tuple_t saxis_set_step_pin(saxis_t *axis, gpio_num_t pin_num);
err_json_tuple_t saxis_set_enable_pin(saxis_t *axis, gpio_num_t pin_num);
err_json_tuple_t saxis_set_steps_per_eu(saxis_t *axis, float steps_to_eu);
err_json_tuple_t saxis_set_microstep(saxis_t *axis, float microstep_factor);

err_json_tuple_t saxis_set_vel_cmd(saxis_t *axis, float vel_cmd);
err_json_tuple_t saxis_set_pos_cmd(saxis_t *axis, float pos_cmd);
err_json_tuple_t saxis_set_mode_cmd(saxis_t *axis, saxis_state_t mode_cmd);
err_json_tuple_t saxis_set_enable(saxis_t *axis, bool enable_cmd);

esp_err_t saxis_cfg_initialize(saxis_t *axis);
esp_err_t saxis_cfg_deinitialize(saxis_t *axis);
esp_err_t saxis_main_scan(saxis_t *axis);
bool saxis_timer_scan(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx);
bool saxis_timer_scan_pos(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx);