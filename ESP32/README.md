# esp32 ROM symbols

https://github.com/espressif/esp-idf/tree/master/components/esp_rom/esp32/ld

We want to extract and recognize all the ROM functions and assign them a "call" numberf for our ALF.


This directory has a cache of source files wherefrom we extract the symbols and generate such an .h file for use in ALF.
