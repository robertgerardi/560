#ifndef CONFIG_H
#define CONFIG_H

#define OPT_DIM_BACKLIGHT 1 // Reduce BL to 15%
#define OPT_LCD_SLEEP 1 // Put LCD controller into sleep mode
#define OPT_WFI 1 // Use WFI instruction to wait or stop
#define OPT_NO_BLUE_LED 1 // Turn off blue LED
#define OPT_SCALE_CLOCK 1 // Adjust clocks between 48/24 MHz and 3/0.75 MHz
#define OPT_PARTIAL_FIELD_UPDATE 1 //
#define OPT_PROFILE_ONCE 1 // Turn off profiler after one run
#define OPT_DYNAMIC_TASK_PERIODS 1 // Adjust task period (read ts) if LCD is off

#define OPT_VLP 0 // Use VLP modes. not working

#endif // CONFIG_H
