/*
 * Temperature sensor AT30TSE75x.
 */

#ifndef PLATFORM_TEMPERATURE_H_INCLUDED
#define PLATFORM_TEMPERATURE_H_INCLUDED

// Initialise TWIM module for AT30TSE75x.
// Return values:
// - STATUS_OK        Success
// - ERR_INVALID_ARG  Invalid arg resulting in wrong CWGR Exponential
status_code_t temperature_init(void);

// Read temperature in half-degrees Celsius.
// Return values:
// - STATUS_OK        Success
// - everything else  Error according to TWI module error codes
status_code_t temperature_read(int *temperature);

#endif
