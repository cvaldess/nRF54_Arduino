#include <Arduino.h>
#include <bluefruit.h>

// Link coverage for the BLE OTA DFU path.
//
// BLEDfu's write-authorize callback references the __bootloader_peer_data
// linker symbol. Nothing else in the framework links that service, so a wrong
// symbol name there compiles fine and only fails at link time in a downstream
// sketch.
// tests/blink never pulls BLEDfu in; this does.
//
// Keep bledfu.begin() in here: it is what installs the write-authorize
// callback, which is what keeps the callback (and the symbol reference) alive
// through --gc-sections.

BLEDfu bledfu;

void setup()
{
    Bluefruit.begin();
    bledfu.begin();
}

void loop()
{
}
