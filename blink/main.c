#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include <hardware/structs/clocks.h>

#define PULSE_TIME_US 10

int timeout = 26100;

// Function to set the servo angle
void setServoAngle(uint gpio, int angle) {
    // Calculate the duty cycle for the given angle
    uint duty_cycle = (angle * 2000 / 180) + 1000;
    // Get the PWM slice for the given GPIO
    uint slice_num = pwm_gpio_to_slice_num(gpio);

    // Set the PWM duty cycle
    pwm_set_gpio_level(gpio, duty_cycle);
    pwm_set_enabled(slice_num, true);
}

int main() {
    stdio_init_all();

    // Define GPIO pins for sensors
    const uint sound_sensor = 6;
    const uint ultrasonic_trig = 26; // analog input A0
    const uint ultrasonic_echo = 27; // analog input A1

    // Define GPIO pins for actuators
    const uint buzzer = 9;
    const uint led_red = 22;
    const uint servo_motor = 28; // analog output 28

    // Initialize input pins
    gpio_init(sound_sensor);
    gpio_set_dir(sound_sensor, GPIO_IN);

    // Initialize output pins
    gpio_init(buzzer);
    gpio_set_dir(buzzer, GPIO_OUT);

    gpio_init(led_red);
    gpio_set_dir(led_red, GPIO_OUT);

    gpio_init(ultrasonic_trig);
    gpio_init(ultrasonic_echo);

    gpio_set_dir(ultrasonic_trig, GPIO_OUT);
    gpio_set_dir(ultrasonic_echo, GPIO_IN);

    // Trigger pin should start low
    gpio_put(ultrasonic_trig, 0);
    sleep_ms(30);  // Wait for sensor to settle

    // Initialize the chosen GPIO pin for PWM
    gpio_set_function(servo_motor, GPIO_FUNC_PWM);

    // Set the PWM frequency to 50Hz (standard for servos)
    uint slice_num = pwm_gpio_to_slice_num(servo_motor);
    pwm_set_wrap(slice_num, 39999);
    pwm_set_clkdiv(slice_num, 40.0f); // Set clock divider

    while (true) {
        // Send a 10us pulse to trigger the ultrasonic sensor
        gpio_put(ultrasonic_trig, 1);
        sleep_us(PULSE_TIME_US);
        gpio_put(ultrasonic_trig, 0);

        // Measure the duration of the echo pulse
        while (gpio_get(ultrasonic_echo) == 0);
        absolute_time_t start_time = get_absolute_time();

        while (gpio_get(ultrasonic_echo) == 1);
        absolute_time_t end_time = get_absolute_time();

        // Calculate distance of any wild animals from the farm gate
        uint32_t elapsed_time_us = absolute_time_diff_us(start_time, end_time);
        double distance = elapsed_time_us / 58.0;
        printf("Distance: %.2f cm\n", distance);

        if (gpio_get(sound_sensor)) {
            // if sound of wild animal is detected, alarm the farmer
            gpio_put(buzzer, true);
            gpio_put(led_red, true);
        }
        else {
            gpio_put(buzzer, false);
            gpio_put(led_red, false);
        }

        // Close the farm door if a wild animal is near the farm
        if (distance < 10) {
            setServoAngle(servo_motor, 210);
            sleep_ms(50);
        }
        // Open the farm door if no animal is near the farm
        else {
            setServoAngle(servo_motor, 30);
            sleep_ms(50);
        }

        sleep_ms(500);
    }
}