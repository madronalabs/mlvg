# CLAP Plugin Example

This example demonstrates how to create a CLAP (CLever Audio Plugin) using madronalib and mlvg.

## Features

- CLAP specification compliant plugin
- Real-time audio processing with madronalib DSP
- Optional GUI using mlvg
- Parameter automation support
- MIDI note input handling
- Plugin state persistence

## Building

### Prerequisites

1. Build and install madronalib with the `header-cleanup` branch:
   ```bash
   git clone https://github.com/johnmatter/madronalib.git
   cd madronalib
   git checkout header-cleanup
   mkdir build && cd build
   cmake -DCMAKE_BUILD_TYPE=Debug ..
   make -j
   sudo make install
   ```

2. Install Rust (for clap-validator testing):
   ```bash
   curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
   ```

### Build the Example

```bash
git clone https://github.com/johnmatter/madronalib.git
cd madronalib
git checkout add-clap-example
mkdir build && cd build
cmake -DBUILD_CLAP_EXAMPLE=ON ..
make -j
```

### Build Options

- `BUILD_CLAP_EXAMPLE`: Enable CLAP plugin example (default: OFF)
- `CLAP_DEMO_GUI`: Include GUI in the CLAP demo plugin (default: ON)
- `CLAP_DEMO_TESTS`: Enable CLAP validation testing (default: ON)

## Testing

The example includes several testing tools:

- **clap-validator**: Validates CLAP specification compliance
- **clap-host**: Simple host for testing plugins
- **clap-info**: Inspects plugin metadata

Run tests with:
```bash
# From build directory
make test-clap-plugin
make test-plugin-host
make inspect-plugin
```

## Plugin Features

- **Audio Processing**: Real-time sawtooth oscillator with parameter control
- **Parameters**: Frequency, gain, and bypass controls
- **GUI**: Optional mlvg-based interface with real-time parameter updates
- **MIDI**: Note input handling for frequency control
- **State**: Parameter persistence across sessions

## Architecture

The plugin follows CLAP specification requirements:

- Core plugin interface (`clap_plugin`)
- Parameters extension (`clap_plugin_params`)
- Audio ports extension (`clap_plugin_audio_ports`)
- Note ports extension (`clap_plugin_note_ports`)
- State extension (`clap_plugin_state`)
- GUI extension (`clap_plugin_gui`) - optional

## Integration with madronalib

This example demonstrates:
- SignalProcessor inheritance for parameter management
- DSPVector-based audio processing
- AudioContext for real-time processing
- Event handling for MIDI and automation

## Integration with mlvg

The GUI demonstrates:
- NativeDrawContext for platform-specific rendering
- Widget system for parameter controls
- Real-time parameter updates
- Resource management

## License

This example is provided under the same license as mlvg (GPL3).
