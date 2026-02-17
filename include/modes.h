/**
 * @file modes.h
 * @brief Satellite operational modes header file
 * @author Floyd DSouza
 * @date 2025
 *
 * This header defines all the operational mode functions for the CADSE v5
 * Space Electronics satellite. Each mode represents a different operational
 * state with specific sensor monitoring and display capabilities.
 */

#ifndef MODES_H
#define MODES_H

/**
 * @defgroup Modes Satellite Operational Modes
 * @brief All six operational modes for the satellite
 * @{
 */

/**
 * @brief Mode 0: Basic Monitoring
 *
 * Displays comprehensive system status including:
 * - Battery voltage
 * - USB voltage
 * - WiFi connection status and signal strength
 * - MQTT connection status
 * - Uptime in minutes and seconds
 *
 * Updates every second on the OLED display.
 */
void runBasicMonitoring();

/**
 * @brief Mode 1: Micro-Gravity Detection
 *
 * Simulates a free-fall detection system that would detect microgravity
 * conditions (acceleration < 1G). Uses touch sensor differential as a
 * proxy for acceleration measurement.
 *
 * Features:
 * - Monitors touch sensor changes
 * - Detects "free fall" conditions above threshold
 * - Activates buzzer when free-fall is detected
 * - Displays current status and detection indicator
 *
 * This mode can be tested by creating sudden touch input changes.
 */
void runMicroGravityDetection();

/**
 * @brief Mode 2: Pressure Monitoring (Cat Safety)
 *
 * Critical safety mode that monitors atmospheric pressure for cabin
 * integrity. Ensures the spacecraft's cat remains safe by detecting
 * sudden pressure drops that would indicate structural failure.
 *
 * Features:
 * - Sets baseline pressure on mode entry
 * - Monitors for pressure drops exceeding 10 hPa
 * - Triggers visual (LED) and audible (buzzer) alarms on detection
 * - Displays current, baseline, and delta pressure values
 *
 * Test by creating a partial vacuum in a sealed container with the board.
 */
void runPressureMonitoring();

/**
 * @brief Mode 3: Attitude Indicator (Artificial Horizon)
 *
 * Provides a flight instrument display showing spacecraft orientation
 * for landing maneuvers. Calculates roll angle from acceleration data
 * and displays an artificial horizon line.
 *
 * Features:
 * - Displays artificial horizon based on roll angle
 * - Shows aircraft centerline indicator
 * - Displays roll angle in degrees
 * - Touch sensors (left/right) allow manual roll adjustment
 * - Smooth 50ms update rate for responsive feedback
 *
 * Assumes gravity acts primarily in Z-axis (vertical) with 9.81 m/s² magnitude.
 */
void runAttitudeIndicator();

/**
 * @brief Mode 4: Rolling Plotter (Real-time Graph)
 *
 * Displays real-time rolling graph visualization of selected sensor data.
 * Allows dynamic selection of which environmental parameter to monitor.
 *
 * Features:
 * - Plots temperature, humidity, or pressure
 * - Touch RIGHT to plot temperature
 * - Touch LEFT to plot humidity
 * - Touch nothing to plot pressure variations
 * - Auto-scaling graph based on current data range
 * - Includes axis labels and current values
 *
 * Updates every 100ms with smooth graph scrolling.
 */
void runRollingPlotter();

/**
 * @brief Mode 5: Creative Mode - Orbit Simulator
 *
 * Bonus creative mode demonstrating satellite orbital mechanics.
 * Shows Earth with a satellite orbiting around it with rotating solar panels.
 *
 * Features:
 * - Animated Earth (circle with shading)
 * - Satellite in circular orbit around Earth
 * - Rotating solar panel indicators
 * - Adjustable orbital speed via touch sensors
 * - Touch RIGHT to increase orbit speed
 * - Touch LEFT to decrease orbit speed
 * - Speed range: 0.1 to 5.0 degrees per update
 *
 * This mode demonstrates understanding of orbital mechanics and creative
 * implementation of satellite features.
 */
void runCreativeMode();

/** @} */

#endif // MODES_H
