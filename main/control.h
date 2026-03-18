// control.h
#ifndef CONTROL_H
#define CONTROL_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void set_pump(bool on);
void set_fan(bool on);

#ifdef __cplusplus
}
#endif

#endif // LED_CONTROL_H