# pico-ds3231
This is a work in progress Raspberry Pi Pico library for DS3231 real time clock module.
Current features are:
\n->Configuring time.
\n->Reading time.
\n->Setting alarm 1 and alarm 2.
\n->Setting alarm interrupt from SQW pin.
\n->AM/PM mode.
\n->Setting output of square wave signal from SQW pin.
\n->Enabling-Disabling oscillator when on powered by on-board battery.

#How to Add to Project:
-
1. Copy ds3231 folder under libraries folder.
2. Add "add_subdirectory(ds3231)" to your CMakeLists.txt file.
3. add "pico_ds3231" in your CMake "target_link_libraries" line 
