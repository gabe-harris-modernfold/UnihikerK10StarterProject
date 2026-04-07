# Unihiker K10 Starter Project

An embedded systems starter project for the **Unihiker K10** DFRobot device (ESP32-S3) that provides modular hardware abstraction and 10 independent component tests. Designed for developers to explore sensor and application logic without wrestling with low-level hardware details.

## Quick Start

1. **Compile**: Arduino IDE (ESP32 board support required)
2. **Upload**: Select unihiker k10 as target board
3. **Run**: Press button A/B to navigate between 10 hardware tests

## Project Purpose

This starter project solves a key problem: **How do you verify all hardware works while building application logic?**

The answer: **Modular tests that run in parallel with your development.**

Rather than replacing tests when you add features, this framework treats tests as:
- **Debugging toolkit**: Verify components work before integration
- **Driver templates**: Copy existing drivers as starting points
- **Documentation**: Each test shows how to interact with hardware
- **Integration validators**: Tests catch hardware regressions

## Architecture: Hardware Tests as Module Verification

The project implements a **rotating menu interface** that cycles through 10 independent hardware component tests. Each test is:
- **Self-contained**: Owned by a dedicated header file, zero external dependencies
- **Visually isolated**: Renders to a shared canvas sprite with consistent UI
- **Independently triggered**: Updates on timer or button press, can be interrupted
- **Easily modifiable**: Can be edited or deleted without breaking other tests

## The 10 Hardware Tests

| Test | Component | Driver File | Purpose |
|------|-----------|-------------|---------|
| 0 | Temperature/Humidity Sensor (AHT20) | `aht20.h` | I2C communication, real-time temp (°C/°F) and humidity readings |
| 1 | Ambient Light Sensor (LTR-303) | `ltr303.h` | Light detection in lux with IR ratio compensation |
| 2 | 3-Axis Accelerometer (SC7A20H) | `sc7a20h.h` | Gravity vector in g-forces (X/Y/Z) |
| 3 | Micro SD Card (HSPI) | `sd_card.h` | Card mounting, capacity detection, root file listing |
| 4 | RGB LEDs + Success LED | `leds.h` | WS2812 NeoPixel control with color cycling and GPIO output |
| 5 | Camera (GC2145, DVP parallel) | `camera.h` | Live HQVGA (240×176) video preview with DMA acceleration |
| 6 | Speaker (NS4168 amplifier, I2S TX) | `speaker.h` | Audio synthesis: 440 Hz sine wave with volume fade |
| 7 | Dual Microphones (MSM381, I2S RX) | `mic.h` | VU metering with RMS + peak hold in dBFS |
| 8 | Font ROM (GT30L24A3W, SPI) | `fontchip.h` | SPI access to 24Mb GB2312/GBK font database |
| 9 | GPIO Expander (XL9535) | `xl9535_test.h` | I2C button state polling and register inspection |

**Navigation**: Press button **A** (next) / **B** (previous) to cycle through tests.

## Hardware Abstraction Layers

All drivers are **header-only** with minimal dependencies, making them lightweight and easy to adapt:

### I2C Sensors (GPIO 47 SDA / 48 SCL)
- `aht20.h` — Temperature & humidity (address: 0x38)
- `ltr303.h` — Ambient light (address: 0x29)
- `sc7a20h.h` — 3-axis accelerometer (address: 0x19)
- `xl9535.h` / `xl9535_test.h` — GPIO expander for buttons & backlight (address: 0x20)

### I2S Audio Subsystem
- `i2s_audio.h` — Shared driver helpers for speaker TX and mic RX
- `speaker.h` — Sine wave generator with frequency/amplitude control
- `mic.h` — VU meter with dBFS calculation and peak hold

### SPI Peripherals
- `sd_card.h` — SD card via HSPI (shared CS pin GPIO 40)
- `fontchip.h` — Font ROM via SPI with NPN inverter control (GPIO 40)
- `camera.h` — DVP parallel interface (separate I2C driver lifecycle)

### Display & UI
- `lgfx_config.h` — LovyanGFX driver for ILI9341 TFT (FSPI 40MHz, DMA enabled)
- `display.h` — Cyberpunk UI primitives (header, footer, bars, fonts)
- `leds.h` — Adafruit_NeoPixel WS2812 RGB LED control

## Critical Hardware Constraints

⚠️ **GPIO 38**: Shared between **I2S LRCK** and **Font Chip CS** — cannot use audio and font ROM simultaneously

⚠️ **GPIO 40**: Shared between **SD_CS** and **Font Chip CS** with NPN inverter — HIGH selects font, LOW selects SD

⚠️ **I2C Bus** (GPIO 47/48): Multiple devices share this bus; camera driver temporarily removes/recreates it during init/deinit

These constraints are handled in the code, but important to know when extending functionality.

## File Structure

```
UnihikerK10StarterProject/
├── UnihikerK10StarterProject.ino    # Main sketch (246 lines)
│                                     # Entry point, button dispatcher, timing
│
├── config.h                          # Pin & device map (single source of truth)
├── globals.h                         # Extern state declarations
│
├── DISPLAY & UI
│   ├── lgfx_config.h                 # LovyanGFX ILI9341 TFT configuration
│   └── display.h                     # UI primitives + cyberpunk palette
│
├── SENSORS (I2C)
│   ├── aht20.h                       # Temperature & humidity
│   ├── ltr303.h                      # Ambient light
│   ├── sc7a20h.h                     # Accelerometer
│   └── xl9535.h                      # GPIO expander driver
│
├── AUDIO (I2S)
│   ├── i2s_audio.h                   # I2S driver installation/cleanup
│   ├── speaker.h                     # Speaker synthesis
│   └── mic.h                         # Microphone VU meter
│
├── STORAGE & ROM
│   ├── sd_card.h                     # SD card filesystem
│   └── fontchip.h                    # Font ROM access
│
├── PERIPHERALS
│   ├── camera.h                      # GC2145 camera with FreeRTOS task
│   ├── leds.h                        # WS2812 RGB LED control
│   └── xl9535_test.h                 # GPIO expander test screen
│
└── README.md
```

## Getting Started: From Tests to Your Application

### Option 1: Keep Tests, Build On Top ✅ **Recommended**
Tests remain as debugging tools while you add application screens:

1. Choose components you need (e.g., sensors + SD card)
2. Add a new test screen in `UnihikerK10StarterProject.ino`
3. Copy driver patterns from existing tests
4. Existing tests stay available for verification

**Benefit**: Your app has built-in diagnostics.

### Option 2: Remove Unused Components
If you only need a few components:

1. Identify which 3–4 tests matter to your application
2. Delete unused `.h` files (e.g., remove `fontchip.h` if you don't need fonts)
3. Simplify the dispatcher loop in `.ino`
4. Add application-specific screens in their place

**Benefit**: Cleaner, smaller codebase.

### Option 3: Extend with Documentation
Document the tests for team collaboration:

1. Add comments to each test explaining what it demonstrates
2. Create a lookup table mapping test → application use case
3. Add example code snippets (read sensor, log to SD, etc.)
4. Document your GPIO constraints prominently

**Benefit**: Onboarding and maintenance become easier.

## Reusable Components

**Don't start from scratch—leverage what's already here:**

| Component | Location | Reuse Pattern |
|-----------|----------|---|
| **UI Framework** | `display.h` | Copy header/footer drawing code; extend with custom widgets |
| **I2C Initialization** | `xl9535.h` | Reference for I2C device setup and register manipulation |
| **I2S Audio** | `i2s_audio.h` | Use install/uninstall helpers; adapt buffer sizes for your needs |
| **SPI Communication** | `sd_card.h` / `fontchip.h` | Template for SPI device lifecycle |
| **Pin Mapping** | `config.h` | Single source of truth—update here when adding hardware |
| **State Management** | `globals.h` | Pattern for FreeRTOS-safe shared state |

## Verification Checklist

Before shipping your application:

- [ ] Compile without errors (Arduino IDE)
- [ ] All active sensors read plausible values
- [ ] SD card mounts and writes files correctly
- [ ] Camera displays live preview without freezing
- [ ] Audio I/O works as expected (speaker plays, mic records)
- [ ] LEDs respond to GPIO commands
- [ ] Button navigation works smoothly
- [ ] GPIO 38/40 multiplexing doesn't cause conflicts
- [ ] Code compiles to <1.5 MB (stays within FLASH)

## Development Tips

✅ **Keep the test framework** — it's your debugging sandbox
✅ **Reference test drivers** as usage examples when writing new code
✅ **Maintain `config.h`** as hardware specifications evolve
✅ **Use the shared `canvas` sprite** for consistent UI

❌ **Don't delete all tests immediately** — they verify hardware works
❌ **Don't bypass the abstraction layers** — drivers are minimal for a reason
❌ **Don't ignore GPIO conflicts** — GPIO 38/40 constraints are real

## Next Steps

1. **Compile & upload** this starter project
2. **Test each screen** (buttons A/B to navigate)
3. **Pick a component** to build your application around
4. **Copy a driver** as a template for custom logic
5. **Add a new test screen** for your feature
6. **Iterate** with tests running alongside development

## Hardware Specs

- **MCU**: ESP32-S3 (240 MHz dual-core)
- **Display**: ILI9341 TFT 240×320 @ 40 MHz (FSPI, DMA)
- **Camera**: GC2145 (240×176 HQVGA, DVP parallel 8-bit)
- **Audio**: I2S TX (44.1 kHz speaker) / RX (16 kHz dual mic)
- **Storage**: Micro SD (HSPI) + 24 Mb Font ROM (SPI)
- **Sensors**: Temp/humidity, light, 3-axis accel
- **I2C**: 6 devices on shared GPIO 47/48
- **Buttons**: 2× GPIO via XL9535 expander
- **RAM**: 512 KB SRAM (+ PSRAM for canvas)

## Resources

- **LovyanGFX Library**: https://github.com/lovyan03/LovyanGFX
- **Adafruit_NeoPixel**: https://github.com/adafruit/Adafruit_NeoPixel
- **ESP32 Arduino Core**: https://github.com/espressif/arduino-esp32

##References
- https://www.unihiker.com/wiki/K10/HardwareReference/img/hardwarereference_onboard/UnihikerK10Schematic.pdf
- https://www.unihiker.com/wiki/K10/HardwareReference/hardwarereference_specs/#system-framwork
- https://www.makerbrains.com/projects/learn-edge-ai-on-unihiker-k10
- https://github.com/MukeshSankhla/Learn-Edge-AI-on-Unihiker-K10-Edge-Impulse-Beginner-Tutorial-
- https://hackaday.io/project/203864-light-meter-unihiker-k10-with-arduino-libraries/details
- Thank you, Mukesh Sankhla!
---

**Happy hacking!** This starter project is designed to be modified. Use it as a foundation, delete what you don't need, and build something cool. 🚀
