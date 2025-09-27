#include "clap-saw-demo.h"
#include "clap-saw-demo-gui.h"
#include <CLAPExport.h>

// Plugin metadata constants required by CLAPExport.h
const char* VendorURL = "https://madronalabs.com";
const char* PluginVersion = "1.0.0";
const char* PluginDescription = "A simple sawtooth oscillator plugin with GUI";

// One line export replaces entire pluginentry.cpp
MADRONALIB_EXPORT_CLAP_PLUGIN_WITH_GUI(ClapSawDemo, ClapSawDemoGUI, "Clap Saw Demo", "Madrona Labs")
