#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "driver/gptimer.h"
#include "soc/gpio_reg.h"
#include "stepper_driver.h"
#include "cfg_dispatcher.h"
#include "esp_timer.h"
#include "cJSON.h"

#define debug 1

#define PIN_LOWER_OK 13
#define PIN_UPPER_OK 33
#define PIN_SAFE_MASK 0b0000'0000'0000'0111'1111'1111'1111'1111'1000

#define TIMER_PERIOD_MAX 60000000 // 60s
#define TIMER_PERIOD_MIN 100 // 10kHz

// hardware array lives here? makes sense, but need extern?
saxis_t g_axis_data_array[NUM_AXES] = {0};

saxis_t *g_axis_ptr_array[NUM_AXES];

static const char *TAG = "stepper_driver";

/*
=========
Misc utility functions
=========
*/

// run at beginning of main app to setup axis arrays
void saxis_setup_all(void) {
    memset(g_axis_ptr_array, 0, sizeof(g_axis_ptr_array));

    for(int i=0;i<NUM_AXES;i++){
        g_axis_ptr_array[i] = &g_axis_data_array[i];
    }
}

bool saxis_util_pin_check(gpio_num_t pin_num){
    return 0;
    //TODO
}

/*
=====
Time Critical Functions
=====
*/
// call when enable is off or any time to stop axis
esp_err_t saxis_estop(saxis_t *axis){
    gpio_set_level(axis->step_pin, 0);
    gpio_set_level(axis->en_pin, 0);

    axis->enable = false;

    gptimer_stop(axis->hardware_timer);

    return ESP_OK;
}
esp_err_t saxis_fault(saxis_t *axis, saxis_fault_code_t code){
    axis->fault_okay = false;
    axis->fault_info.code = code;
    axis->fault_info.time = esp_timer_get_time();

    return ESP_OK;
}

/*
=====
Mode control functions
=====
*/

// Enable pin on, timer stopped
esp_err_t saxis_ctrl_hold(saxis_t *axis){
    gptimer_stop(axis->hardware_timer);

    gpio_set_level(axis->step_pin, 0);
    #if debug
    ESP_LOGI(TAG, "Axis [%s] holding position", axis->name_str, axis->vel_timer_period);
    #endif

    axis->mode = AXIS_CTRL_HOLD;
    return ESP_OK;

}

esp_err_t saxis_ctrl_vel(saxis_t *axis){
    // DIRECTION
    bool dir_level = axis->target_direction_actual > 0 ? 1 : 0; // set in velocity set function
    dir_level = axis->dir_pin_inv ? !dir_level : dir_level;
    gpio_set_level(axis->dir_pin, dir_level); //TODO might need this is timer scan

    // STEP RATE
    axis->vel_timer_period = (1e6 / (axis->target_velocity_steps))/2;
    if (axis->vel_timer_period > TIMER_PERIOD_MAX){
        axis->vel_timer_period = TIMER_PERIOD_MAX;
        ESP_LOGW(TAG, "Axis [%s] step clock period too long, limited to %dus",axis->name_str,TIMER_PERIOD_MAX);
    }else if (axis->vel_timer_period < TIMER_PERIOD_MIN){
        axis->vel_timer_period = TIMER_PERIOD_MIN;
        ESP_LOGW(TAG, "Axis [%s] step clock period too short, limited to %dus",axis->name_str,TIMER_PERIOD_MIN);
    }


    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0, // Reset count to 0
        .alarm_count = axis->vel_timer_period, // timer period, 1us resolution
        .flags.auto_reload_on_alarm = true, // auto reload timer
    };

    // TIMER SETUP
    esp_err_t timer_err = gptimer_set_alarm_action(axis->hardware_timer, &alarm_config);
    if (timer_err==ESP_OK){timer_err = gptimer_start(axis->hardware_timer);}

    if (timer_err!=ESP_OK){
        ESP_LOGW(TAG, "Axis [%s] timer fault", axis->name_str, axis->vel_timer_period);
        saxis_fault(axis, AXIS_FAULT_TIMER);
        return timer_err;
    }

    #if debug
    ESP_LOGI(TAG, "Axis [%s] timer start with period of %lu us", axis->name_str, axis->vel_timer_period);
    #endif

    axis->mode = AXIS_CTRL_VELOCITY;

    return ESP_OK;
}

esp_err_t saxis_ctrl_pos(saxis_t *axis){
    return ESP_FAIL;
    //TODO code for position run, set different function for timer scan
}


/*
=========
Parameter and signal set functions
Should evaluate quickly to be safe for configs and commands
=========
*/

//TODO check for existing pins, global pin array?

// no cJSON parsing or handling, keep light if can
esp_err_t saxis_set_dir_pin(saxis_t *axis, gpio_num_t pin_num){
    // TODO, add more checks for existing allocations
    if ((pin_num <= PIN_UPPER_OK) && (pin_num >= PIN_LOWER_OK)){
        axis->dir_pin = pin_num;
        return ESP_OK;
    }else{
        return ESP_ERR_INVALID_ARG;
    }
}
esp_err_t saxis_set_step_pin(saxis_t *axis, gpio_num_t pin_num){
    // TODO, add more checks for existing allocations
    if ((pin_num <= PIN_UPPER_OK) && (pin_num >= PIN_LOWER_OK)){
        axis->step_pin = pin_num;
        return ESP_OK;
    }else{
        return ESP_ERR_INVALID_ARG;
    }
}
esp_err_t saxis_set_enable_pin(saxis_t *axis, gpio_num_t pin_num){
    // TODO, add more checks for existing allocations
    if ((pin_num <= PIN_UPPER_OK) && (pin_num >= PIN_LOWER_OK)){
        axis->en_pin = pin_num;
        return ESP_OK;
    }else{
        return ESP_ERR_INVALID_ARG;
    }
}
esp_err_t saxis_set_steps_per_eu(saxis_t *axis, float steps_per_eu){
    // TODO, add more checks
    if ((steps_per_eu <= 1e10) && (steps_per_eu >= -1e10)){
        axis->factor_steps_per_unit = steps_per_eu;
        return ESP_OK;
    }else{
        return ESP_ERR_INVALID_ARG;
    }
}
esp_err_t saxis_set_microstep(saxis_t *axis, int microstep){
    // TODO, add more checks
    if ((microstep <= 128) && (microstep >= 0)){
        axis->factor_microstep = microstep;
        return ESP_OK;
    }else{
        return ESP_ERR_INVALID_ARG;
    }
}

esp_err_t saxis_set_mode_cmd(saxis_t *axis, saxis_state_t mode_cmd){
    if (mode_cmd <AXIS_CTRL_HOLD || mode_cmd > AXIS_CTRL_POSITION){
        return ESP_ERR_INVALID_ARG;
    }else{
        axis->mode_cmd = mode_cmd;
        return ESP_OK;
    }
}

esp_err_t saxis_set_vel_cmd(saxis_t *axis, float vel_cmd){
    if (false){ // TODO (vel_cmd < axis->limit_velocity_low || vel_cmd > axis->limit_velocity_high){
        return ESP_ERR_INVALID_ARG;
    }else{
        axis->target_velocity_cmd = vel_cmd;
        axis->target_velocity_steps = vel_cmd * axis->factor_steps_per_unit * axis->factor_microstep;
        axis->target_direction_actual = (vel_cmd > 0) ? 1 : -1;
        return ESP_OK;
    }
}

esp_err_t saxis_set_pos_cmd(saxis_t *axis, float pos_cmd){
    if (false){ // TODO (vel_cmd < axis->limit_velocity_low || vel_cmd > axis->limit_velocity_high){
        return ESP_ERR_INVALID_ARG;
    }else{
        axis->target_position_cmd = pos_cmd;
        axis->target_position_steps = pos_cmd * axis->factor_steps_per_unit * axis->factor_microstep;
        return ESP_OK;
    }
}

esp_err_t saxis_set_enable(saxis_t *axis, bool enable_cmd){
    if (!axis->fault_okay && enable_cmd !=0 ){ // TODO (vel_cmd < axis->limit_velocity_low || vel_cmd > axis->limit_velocity_high){
        return ESP_ERR_INVALID_ARG;
    }else{
        axis->enable = enable_cmd;
        return ESP_OK;
    }
}


/*
=========
Parameter and signal get functions
Should evaluate quickly to be safe for configs and commands
=========
*/
float saxis_get_vel_act(saxis_t *axis){
    // TODO better estimate?
    return axis->target_velocity_steps / (axis->factor_steps_per_unit * axis->factor_microstep);
}

float saxis_get_pos_act(saxis_t *axis){
    return axis->odom_steps_act / (axis->factor_steps_per_unit * axis->factor_microstep);
}

/*
=========
Main control functions
=========
*/

bool IRAM_ATTR saxis_timer_scan(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx){
    // called from timer function to update gpio
    saxis_t *axis = (saxis_t *)user_ctx;

    // not checking, might regret that
    // if ((axis->step_pin < PIN_LOWER_OK) || (axis->step_pin > PIN_UPPER_OK)){return 0;}

    // exit if not enabled
    if(!axis->enable){return 0;}
    
    axis->pin_states.step = !axis->pin_states.step;

    if(axis->pin_states.step){
        REG_WRITE(GPIO_OUT_W1TS_REG, (1UL << axis->step_pin));
    }else{
        REG_WRITE(GPIO_OUT_W1TC_REG, (1UL << axis->step_pin));
    }

    // increment step counter only on transition from low to high
    axis->odom_steps_act += axis->pin_states.step * axis->target_direction_actual;

    return 0;
}

bool IRAM_ATTR saxis_timer_scan_pos(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx){
    // timer function for position control
    // called from timer function to update gpio

    return 0;
}


esp_err_t saxis_cfg_initialize(saxis_t *axis){
    #if debug
    ESP_LOGI(TAG, "Initializing axis: %s, dir=%d, stp=%d, en=%d", axis->name_str, axis->dir_pin, axis->step_pin, axis->en_pin);
    #endif

    // TODO add more error checks and deallocate if failed

    // set pin modes for step, dir, and enable pin
    int pin_err = 0; 
    pin_err |= gpio_reset_pin(axis->dir_pin);
    pin_err |= gpio_set_direction(axis->dir_pin, GPIO_MODE_OUTPUT);
    pin_err |= gpio_set_level(axis->dir_pin, 0);
    pin_err |= gpio_reset_pin(axis->step_pin);
    pin_err |= gpio_set_direction(axis->step_pin, GPIO_MODE_OUTPUT);
    pin_err |= gpio_set_level(axis->step_pin, 0);
    pin_err |= gpio_reset_pin(axis->en_pin);
    pin_err |= gpio_set_direction(axis->en_pin, GPIO_MODE_OUTPUT);
    pin_err |= gpio_set_level(axis->en_pin, 0);

    if (pin_err) {return pin_err;}
    
    // set default states
    axis->pin_states.dir = 0;
    axis->pin_states.step = 0;
    axis->pin_states.en = 0;

    axis->mode = AXIS_CTRL_HOLD;

    //clear odometry
    axis->odom_steps_act = 0;
    axis->enable = 0;

    //set default hardware
    axis->wd_timeout_us = 10000000;
    axis->vel_timer_period = 1000000;
    axis->factor_microstep = 1;
    axis->factor_steps_per_unit = 1.0;


    // create hardware timer for step pin
    axis->hardware_timer = NULL;
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT, // default hardware clock
        .direction = GPTIMER_COUNT_UP,      // count up
        .resolution_hz = 1 * 1000 * 1000,   // Resolution is 1us / @1MHz
    };

    // Create a timer instance
    esp_err_t timer_err;

    timer_err = gptimer_new_timer(&timer_config, &axis->hardware_timer);
    if (timer_err) { return timer_err;}

    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0,      // When the alarm event occurs, the timer will automatically reload to 0
        .alarm_count = TIMER_PERIOD_MAX, // Set the actual alarm period, resolution is 1us
        .flags.auto_reload_on_alarm = true, // Enable auto-reload function
    };

    // Set the timer's alarm action
    timer_err = gptimer_set_alarm_action(axis->hardware_timer, &alarm_config);
    if (timer_err) { return timer_err;}

    gptimer_event_callbacks_t cbs = {
        .on_alarm = saxis_timer_scan, // Call the user callback function when the alarm event occurs
    };

    timer_err = gptimer_register_event_callbacks(axis->hardware_timer, &cbs, axis);
    if (timer_err) { return timer_err;}

    // Enable the timer
    timer_err = gptimer_enable(axis->hardware_timer);
    if (timer_err) { return timer_err;}

    // If all ok, set as ready
    axis->initialized = true;
    axis->fault_okay = 1;

    ESP_LOGI(TAG, "Axis [%s] init complete", axis->name_str);

    return ESP_OK;

}

esp_err_t saxis_cfg_deinitialize(saxis_t *axis){
    #if debug
    ESP_LOGI(TAG, "Freeing [%s]", axis->name_str);
    #endif

    esp_err_t err = 0;
    
    gpio_reset_pin(axis->en_pin);
    gpio_reset_pin(axis->dir_pin);
    gpio_reset_pin(axis->step_pin);

    axis->dir_pin = 999;
    axis->step_pin = 999;
    axis->en_pin = 999;

        // stop and remove hardware timer
    err |= gptimer_disable(axis->hardware_timer);
    err |= gptimer_del_timer(axis->hardware_timer);

    axis->initialized = false;
    axis->fault_okay = false;
    axis->running = false;
    axis->moving = false;
    axis->enable = false;

    if (err){ESP_LOGE(TAG, "Timer error while freeing [%s]", axis->name_str);}

    return err ? ESP_FAIL : ESP_OK;
}

esp_err_t saxis_reset(saxis_t *axis){
    //TODO
    axis->fault_okay = 1;
    return ESP_FAIL;
}

esp_err_t saxis_main_scan(saxis_t *axis){
    // nothing set in this function, only calls to set functions

    // set last scan even if not initialized
    uint64_t now = esp_timer_get_time();
    uint64_t prev_scan = axis->wd_last_scan_us;
    axis->wd_last_scan_us = now;

    // skip wd and fault checks if not initialized
    if (!axis->initialized){return ESP_FAIL;}

    #if debug
    ESP_LOGI(TAG, "Scanning [%s], odom=%"PRId64", mode=%d/%d, enable=%d, fault_ok=%d, init=%d", 
        axis->name_str, 
        axis->odom_steps_act, 
        axis->mode,
        axis->mode_cmd,
        axis->enable, 
        axis->fault_okay,
        axis->initialized
    );
    #endif

    // TODO not actually a watchdog timer, should set this up to call elsewhere

    if (now > prev_scan + axis->wd_timeout_us){
        saxis_estop(axis);
        saxis_fault(axis, AXIS_FAULT_WD);
        ESP_LOGE(TAG, "Axis [%s] watchdog overrun at %d microseconds", axis->name_str, now - axis->wd_last_scan_us);
        return ESP_FAIL;
    }
    
    // TODO fault logic
    if (false){
        saxis_estop(axis);
        saxis_fault(axis, AXIS_FAULT_ODOM_LOST);
        ESP_LOGE(TAG, "Axis [%s] faulted, stopping", axis->name_str);
        return ESP_FAIL;
    }
    
    // eval mode command and call mode switch function if allowed
    esp_err_t mode_err = ESP_OK;
    if (axis->fault_okay){
        if (axis->mode_cmd != axis->mode){
            switch (axis->mode_cmd){
                case AXIS_CTRL_HOLD:
                    mode_err = saxis_ctrl_hold(axis);
                    break;
                case AXIS_CTRL_POSITION:
                    mode_err = saxis_ctrl_pos(axis);
                    break;
                case AXIS_CTRL_VELOCITY:
                    mode_err = saxis_ctrl_vel(axis);
                    break;
                default:
                    mode_err = ESP_ERR_INVALID_ARG;
            }
            
        }
    }

    // #if debug
    // // uint64_t timer_val;
    // // ESP_ERROR_CHECK(gptimer_get_raw_count(axis->gptimer, &timer_val));
    // // ESP_LOGI(TAG, "Axis [%s] scan timer at %llu counts", axis->name_str, timer_val);
    // ESP_LOGI(TAG, "Scanned [%s], odom=%"PRId64", mode=%d/%d, enable=%d, fault_ok=%d", 
    //     axis->name_str, 
    //     axis->odom_steps_act, 
    //     axis->mode,
    //     axis->mode_cmd,
    //     axis->enable, 
    //     axis->fault_okay
    // );
    // #endif

    return mode_err;

}
