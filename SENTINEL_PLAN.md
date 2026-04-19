# SENTINEL — Autonomous Perimeter Detection & Response System

## Project Overview

SENTINEL is a bare-metal autonomous perimeter defense system built on the STM32F401RE. It combines a custom RTOS, multi-sensor fusion, real-time object tracking, and a Python ground station with a live radar display. The system autonomously detects, tracks, and responds to objects entering its perimeter.

The project includes **MicroBench**, an embedded benchmark suite that measures real-time performance metrics and compares the custom RTOS against FreeRTOS and bare-metal execution.

---

## System Architecture

```
┌──────────────────────────────────────────────────────────────┐
│                     SENSOR TOWER                              │
│                                                               │
│  ┌────────────────────────────────────────────────────────┐  │
│  │  STM32F401RE — Custom RTOS (SENTINEL-OS)               │  │
│  │                                                         │  │
│  │  Tasks:                                                 │  │
│  │  • Sweep Task       (50Hz)  — servo + ultrasonic        │  │
│  │  • Sensor Task      (100Hz) — distance + PIR reads      │  │
│  │  • Tracking Task    (50Hz)  — PID loop for laser lock   │  │
│  │  • State Machine    (20Hz)  — PATROL/ALERT/LOCKED/ALARM │  │
│  │  • Telemetry Task   (10Hz)  — WiFi data to ground       │  │
│  │  • Display Task     (5Hz)   — LCD status updates        │  │
│  └────────────────────────────────────────────────────────┘  │
│                          ↕                                    │
│  ┌────────────────────────────────────────────────────────┐  │
│  │  PERIPHERALS                                            │  │
│  │  • HC-SR04 Ultrasonic Sensor — distance measurement     │  │
│  │  • PIR Motion Sensor — wake-on-motion detection         │  │
│  │  • SG90 Servo Motor — 360° sweep / tracking             │  │
│  │  • Laser Diode Module — target marking                  │  │
│  │  • Buzzer — audio alert                                 │  │
│  │  • RGB LED Ring — status indicator                      │  │
│  │  • MH1602A LCD (I2C) — local status display             │  │
│  │  • ESP8266 (UART) — WiFi telemetry                      │  │
│  └────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────┘
                          ↓ WiFi
┌──────────────────────────────────────────────────────────────┐
│              GROUND STATION (Python / Mac)                     │
│                                                               │
│  ┌────────────────────┐  ┌──────────────────────────────┐    │
│  │   RADAR DISPLAY     │  │   EVENT LOG                  │    │
│  │   Green sweep       │  │   12:05:03 PATROL            │    │
│  │      ·              │  │   12:05:07 ALERT @ 45° 0.8m  │    │
│  │    · · ·            │  │   12:05:08 LOCKED             │    │
│  │   · ╳  · ← target  │  │   12:05:10 ALARM              │    │
│  │    · · ·            │  │                               │    │
│  └────────────────────┘  └──────────────────────────────┘    │
│                                                               │
│  [ARM] [DISARM] [SENSITIVITY] [REPLAY]                        │
└──────────────────────────────────────────────────────────────┘
```

---

## State Machine

```
                    ARM cmd
    ┌─────┐      ┌────────┐     object detected     ┌───────┐
    │ PAD │─────►│ PATROL │─────────────────────────►│ ALERT │
    └─────┘      └────────┘                          └───┬───┘
                   ▲    ▲                                 │
                   │    │         DISARM cmd               │ object confirmed
                   │    └─────────────────────┐           ▼
                   │                          │      ┌────────┐
                   │         timeout (clear)   ├─────│ LOCKED │
                   │                          │      └───┬────┘
                   │                          │          │
                   │                          │          │ object persists
                   │                          │          ▼
                   │                          │      ┌───────┐
                   └──────────────────────────┴──────│ ALARM │
                              manual reset           └───────┘
```

### State Definitions

| State   | Entry Condition              | Actions                                         | Exit Condition                    |
|---------|------------------------------|--------------------------------------------------|-----------------------------------|
| PAD     | Power on                     | LCD: "SENTINEL ONLINE", LED: off                 | ARM command from ground station   |
| PATROL  | ARM received                 | Servo sweeps 360°, LED: green, builds radar map  | Object detected within threshold  |
| ALERT   | Object detected for >100ms   | Servo pauses, LED: yellow flash, logs event      | Object confirmed for >300ms       |
| LOCKED  | Object confirmed             | Servo tracks object, laser ON, LED: red, PID loop| Object persists >3s               |
| ALARM   | Object in LOCKED for >3s     | Buzzer ON, LED: red flash, telemetry burst       | DISARM command or manual reset    |

---

## Hardware Bill of Materials

| Component              | Purpose                    | Interface   | Pin Mapping          | Cost  |
|------------------------|----------------------------|-------------|----------------------|-------|
| STM32 Nucleo F401RE    | Main controller            | —           | —                    | Have  |
| HC-SR04 Ultrasonic     | Distance measurement       | GPIO + Timer| PA0 (Trig), PA1 (Echo)| ~$2  |
| PIR Motion Sensor      | Wake-on-motion             | GPIO (EXTI) | PC13 or PB0         | ~$2   |
| SG90 Servo Motor       | Pan sweep / tracking       | PWM (Timer) | PA6 (TIM3_CH1)      | ~$3   |
| Laser Diode Module     | Target marking             | GPIO        | PB1                  | ~$2   |
| Passive Buzzer         | Audio alert                | PWM/GPIO    | PB2                  | ~$1   |
| RGB LED (or NeoPixel)  | Status indicator           | GPIO/PWM    | PA8, PA9, PA10       | ~$3   |
| MH1602A LCD            | Local status display       | I2C         | PB8 (SCL), PB9 (SDA)| Have  |
| ESP8266                | WiFi telemetry             | UART        | PA2 (TX), PA3 (RX)  | ~$4   |
| **Total additional**   |                            |             |                      |**~$17**|

### Wiring Diagram

```
STM32 Nucleo F401RE
┌──────────────────────┐
│                      │
│  PA0  ──────────────►│── HC-SR04 Trig
│  PA1  ◄──────────────│── HC-SR04 Echo (needs voltage divider 5V→3.3V)
│                      │
│  PA2  ──────────────►│── ESP8266 RX (UART2_TX)
│  PA3  ◄──────────────│── ESP8266 TX (UART2_RX)
│                      │
│  PA6  ──────────────►│── Servo PWM (TIM3_CH1)
│                      │
│  PA8  ──────────────►│── Red LED
│  PA9  ──────────────►│── Green LED
│  PA10 ──────────────►│── Blue LED
│                      │
│  PB0  ◄──────────────│── PIR Sensor (EXTI)
│  PB1  ──────────────►│── Laser Diode
│  PB2  ──────────────►│── Buzzer
│                      │
│  PB8  ──────────────►│── LCD SCL (I2C1)
│  PB9  ◄────────────►│── LCD SDA (I2C1)
│                      │
│  3.3V ──────────────►│── Power rail (LCD, PIR, ESP8266)
│  5V   ──────────────►│── HC-SR04 VCC, Servo VCC
│  GND  ──────────────►│── Common ground
└──────────────────────┘
```

---

## Phase-by-Phase Build Plan

### Phase 1: RTOS Core (Weeks 1-4)

**Goal:** Build a working preemptive RTOS from scratch on the STM32F401RE.

No new hardware needed — just the Nucleo board, LED, button, UART, and LCD you already have.

#### Week 1-2: Context Switching

**What you're building:**
- SysTick fires every 1ms
- SysTick_Handler saves current task's registers (context) onto its stack
- Loads the next task's registers from its stack
- CPU resumes the next task as if nothing happened

**Key concepts:**
- Each task has its own stack (carved out of SRAM)
- Context = registers R0-R12, LR, PC, PSR
- PendSV handler does the actual switch (lower priority than SysTick to avoid interrupting other ISRs)

**Deliverables:**
- `kernel/scheduler.c` — round-robin scheduler
- `kernel/context_switch.s` — assembly context switch (PendSV_Handler)
- `kernel/task.c` — task creation, stack initialization
- Proof: 3 tasks blinking LEDs at different rates, printing their task ID over UART

**File structure:**
```
SENTINEL/
├── Inc/
│   ├── os_kernel.h
│   ├── os_task.h
│   └── os_config.h
├── Src/
│   ├── os_kernel.c
│   ├── os_task.c
│   ├── os_context_switch.s
│   └── main.c
├── Drivers/
│   ├── Inc/
│   │   ├── gpio.h
│   │   ├── uart.h
│   │   ├── systick.h
│   │   ├── i2c.h
│   │   └── lcd.h
│   └── Src/
│       ├── gpio.c
│       ├── uart.c
│       ├── systick.c
│       ├── i2c.c
│       └── lcd.c
├── stm32_ls.ld
├── stm32f411_startup.c
└── Makefile
```

#### Week 3: Priority Scheduler

**What you're building:**
- Rate Monotonic Scheduling (RMS) — higher frequency tasks get higher priority
- Preemption: high-priority task can interrupt low-priority task mid-execution

**Deliverables:**
- Priority levels (at least 4)
- Task ready queue sorted by priority
- Preemptive scheduling: high-priority task runs immediately when it becomes ready
- Proof: high-priority task (fast LED) preempts low-priority task (slow UART print)

#### Week 4: Synchronization Primitives

**What you're building:**
- Mutexes — protect shared resources (like I2C bus, LCD)
- Semaphores — signal between tasks and ISRs
- Priority inheritance — prevent priority inversion

**Deliverables:**
- `os_mutex.c` — mutex with priority inheritance
- `os_semaphore.c` — binary and counting semaphores
- Proof: two tasks sharing the LCD via mutex, no corruption
- Proof: ISR signals semaphore, task wakes up and responds

---

### Phase 2: Sensor Integration (Weeks 5-8)

**Goal:** Get all sensors reading through the RTOS.

**Hardware to buy:** HC-SR04, PIR sensor, SG90 servo, laser diode, buzzer, RGB LEDs

#### Week 5: Servo Control (PWM)

**What you're building:**
- Timer 3 in PWM mode to control the SG90 servo
- Servo driver: `servo_set_angle(uint16_t degrees)`
- Sweep function: servo rotates from 0° to 180° and back

**Key concepts:**
- SG90 expects 50Hz PWM (20ms period)
- 1ms pulse = 0°, 1.5ms = 90°, 2ms = 180°
- TIM3 prescaler and ARR configured for 50Hz
- CCR value maps to pulse width

**Deliverables:**
- `drivers/servo.c` — bare-metal PWM driver
- Servo sweeps smoothly, angle displayed on LCD
- RTOS sweep task running at 50Hz

#### Week 6: Ultrasonic Sensor

**What you're building:**
- HC-SR04 driver using timer input capture
- Send 10µs trigger pulse on PA0
- Measure echo pulse width on PA1 using TIM2 input capture
- Calculate distance: `distance_cm = pulse_us / 58`

**Key concepts:**
- Input capture: timer records the counter value when a pin edge occurs
- Rising edge → start count, falling edge → stop count
- DMA can automate this (ties into DMA chapter)

**Important:** HC-SR04 echo pin is 5V — you need a voltage divider (2 resistors) to bring it down to 3.3V for the STM32.

**Deliverables:**
- `drivers/ultrasonic.c` — trigger + input capture driver
- Distance readings at 50Hz displayed on LCD and sent over UART
- Accurate to within 1cm at distances 2cm-400cm

#### Week 7: PIR + Laser + Buzzer + LEDs

**What you're building:**
- PIR sensor on EXTI (interrupt-driven, just like your button!)
- Laser diode: simple GPIO on/off
- Buzzer: PWM for different tone patterns
- RGB LED: PWM or GPIO for color mixing

**Deliverables:**
- PIR interrupt wakes the system from idle sweep
- Laser tracks servo angle
- Buzzer plays different patterns per state (short beep for ALERT, continuous for ALARM)
- LED colors match state (green=PATROL, yellow=ALERT, red=LOCKED, flashing red=ALARM)

#### Week 8: Integration

**What you're building:**
- All sensors running as RTOS tasks simultaneously
- Sensor task reads ultrasonic at each servo angle
- Builds a distance map: `uint16_t radar_map[180]` — distance at each degree
- Object detection: compare consecutive sweeps, flag new objects

**Deliverables:**
- All 6 RTOS tasks running concurrently without conflicts
- LCD shows: current angle, distance, state
- UART streams raw sensor data for debugging
- Radar map builds correctly over one full sweep

---

### Phase 3: State Machine & Tracking (Weeks 9-12)

**Goal:** Autonomous detection and response with PID tracking.

#### Week 9: State Machine

**What you're building:**
- State machine task running at 20Hz
- Implements full PATROL → ALERT → LOCKED → ALARM flow
- State transitions based on sensor data + timing

**State transition logic (pseudocode):**
```c
switch(current_state)
{
    case PATROL:
        if(object_detected && detection_time > 100ms)
            transition(ALERT);
        break;

    case ALERT:
        if(object_still_present && alert_time > 300ms)
            transition(LOCKED);
        if(!object_detected && clear_time > 500ms)
            transition(PATROL);
        break;

    case LOCKED:
        if(locked_time > 3000ms)
            transition(ALARM);
        if(!object_detected && clear_time > 1000ms)
            transition(PATROL);
        break;

    case ALARM:
        if(disarm_received)
            transition(PATROL);
        break;
}
```

**Deliverables:**
- State transitions work correctly
- LCD shows current state
- LED colors change per state
- Event log stored in RAM with timestamps

#### Week 10-11: PID Tracking

**What you're building:**
- When object detected, servo stops sweeping and tracks it
- PID controller adjusts servo angle to keep object centered
- Laser stays on target as object moves

**PID explained:**
```
error = target_angle - current_angle

P (proportional) = Kp * error              → "how far off am I?"
I (integral)     = Ki * sum_of_errors      → "have I been off for a while?"
D (derivative)   = Kd * (error - prev_err) → "am I getting closer or further?"

servo_adjustment = P + I + D
```

**Deliverables:**
- `control/pid.c` — generic PID controller
- Servo tracks a moving object smoothly
- Tune Kp, Ki, Kd for fast response without oscillation
- Laser stays within 5° of target

#### Week 12: Polish & Edge Cases

**Deliverables:**
- Handle sensor noise (moving average filter on ultrasonic data)
- Debounce object detection (don't trigger on momentary glitches)
- Multiple object handling (track closest, log all)
- Smooth transition between sweep and track modes
- Watchdog timer: system resets if any task hangs

---

### Phase 4: Telemetry & Ground Station (Weeks 13-16)

**Goal:** Wireless radar display and command interface.

#### Week 13: ESP8266 WiFi

**What you're building:**
- UART driver for ESP8266 AT commands
- Connect to local WiFi network
- Open TCP socket for telemetry streaming

**AT command sequence:**
```
AT+CWMODE=1              → station mode
AT+CWJAP="SSID","PASS"   → connect to WiFi
AT+CIPSTART="TCP","IP",PORT → open connection
AT+CIPSEND=<len>          → send data
```

**Deliverables:**
- ESP8266 connects to WiFi reliably
- Can send/receive data over TCP

#### Week 14: Telemetry Protocol

**What you're building:**
- Binary packet format for efficient transmission
- Sent at 10Hz from telemetry task

```c
typedef struct {
    uint8_t  sync;          // 0xAA
    uint8_t  state;         // current state enum
    uint16_t servo_angle;   // current angle in degrees
    uint16_t distance;      // distance in mm
    uint8_t  pir_active;    // PIR triggered flag
    uint16_t radar_map[36]; // compressed map (every 5°)
    uint32_t timestamp;     // ms since boot
    uint8_t  checksum;      // XOR of all bytes
} __attribute__((packed)) TelemetryPacket;
```

**Deliverables:**
- Packets stream reliably at 10Hz
- Checksum validates data integrity
- Dropped packet detection

#### Week 15-16: Python Ground Station

**What you're building:**
- Python app receives telemetry over TCP/WiFi
- Radar display: green sweep line, red blips for detected objects
- Event log panel with timestamps
- Command buttons: ARM, DISARM, SENSITIVITY

**Tech stack:**
- Python 3
- PyQt5 for GUI
- Matplotlib or custom QPainter for radar display
- Socket for TCP communication

**Ground station layout:**
```
┌──────────────────────────────────────────────────────┐
│  SENTINEL Ground Station v1.0                         │
├──────────────────────────────────────────────────────┤
│                                                       │
│  ┌──────────────┐  ┌───────────────────────────────┐ │
│  │              │  │  State: LOCKED                 │ │
│  │   RADAR      │  │  Angle: 47°                   │ │
│  │   DISPLAY    │  │  Distance: 0.82m              │ │
│  │              │  │  Uptime: 00:12:34             │ │
│  │   (green     │  ├───────────────────────────────┤ │
│  │    sweep     │  │  EVENT LOG                    │ │
│  │    with      │  │  12:05:03.145 PATROL started  │ │
│  │    blips)    │  │  12:05:07.302 ALERT @ 45°     │ │
│  │              │  │  12:05:07.812 LOCKED on target │ │
│  │              │  │  12:05:10.815 ALARM triggered  │ │
│  └──────────────┘  └───────────────────────────────┘ │
│                                                       │
│  [ARM] [DISARM] [SENSITIVITY ◄═══►] [REPLAY] [EXPORT]│
└──────────────────────────────────────────────────────┘
```

**Deliverables:**
- Real-time radar display with sweep animation
- Blips appear/disappear as objects enter/leave
- Event log with full history
- ARM/DISARM commands sent back to STM32
- Export radar data to CSV

---

### Phase 5: MicroBench — Benchmark Suite (Weeks 17-18)

**Goal:** Quantify SENTINEL-OS performance and compare against FreeRTOS and bare-metal.

#### Benchmarks

**1. Context Switch Time**
```
Measure time between: task A yields → task B starts executing
Method: toggle GPIO pin in each task, measure on oscilloscope or timer capture
Target: < 5µs
```

**2. Interrupt Latency**
```
Measure time between: external interrupt fires → ISR first instruction
Method: trigger EXTI, read cycle counter (DWT->CYCCNT) at ISR entry
Target: < 1µs
```

**3. Sweep-to-Detect Latency**
```
Measure time between: ultrasonic echo received → state transitions to ALERT
This is SENTINEL-specific: tests full pipeline from sensor → processing → decision
Target: < 20ms
```

**4. PID Loop Jitter**
```
Measure variation in PID task execution period
Should run every 20ms (50Hz) — how much does it vary?
Target: < 0.5ms jitter
```

**5. Mutex Overhead**
```
Measure time to acquire + release an uncontested mutex
Target: < 2µs
```

**6. Telemetry Throughput**
```
Measure maximum packets per second without drops
Target: > 20 packets/sec
```

#### Comparison Matrix

```
| Metric               | Bare Metal | SENTINEL-OS | FreeRTOS |
|----------------------|------------|-------------|----------|
| Context Switch       | N/A        | ? µs        | ? µs     |
| Interrupt Latency    | ? µs       | ? µs        | ? µs     |
| Sweep-to-Detect      | ? ms       | ? ms        | ? ms     |
| PID Jitter           | ? ms       | ? ms        | ? ms     |
| Mutex Overhead       | N/A        | ? µs        | ? µs     |
| Telemetry Throughput | ? pkt/s    | ? pkt/s     | ? pkt/s  |
| Flash Usage          | ? KB       | ? KB        | ? KB     |
| RAM Usage            | ? KB       | ? KB        | ? KB     |
```

**Deliverables:**
- Automated benchmark runner
- Results printed over UART in formatted table
- Comparison with FreeRTOS port of the same application
- Performance analysis document

---

## Repository Structure

```
sentinel-system/
├── firmware/
│   ├── Inc/
│   │   ├── os_kernel.h
│   │   ├── os_task.h
│   │   ├── os_mutex.h
│   │   ├── os_semaphore.h
│   │   └── os_config.h
│   ├── Src/
│   │   ├── os_kernel.c
│   │   ├── os_task.c
│   │   ├── os_mutex.c
│   │   ├── os_semaphore.c
│   │   ├── os_context_switch.s
│   │   └── main.c
│   ├── Drivers/
│   │   ├── Inc/
│   │   │   ├── gpio.h
│   │   │   ├── uart.h
│   │   │   ├── i2c.h
│   │   │   ├── lcd.h
│   │   │   ├── systick.h
│   │   │   ├── servo.h
│   │   │   ├── ultrasonic.h
│   │   │   ├── pir.h
│   │   │   └── esp8266.h
│   │   └── Src/
│   │       ├── gpio.c
│   │       ├── uart.c
│   │       ├── i2c.c
│   │       ├── lcd.c
│   │       ├── systick.c
│   │       ├── servo.c
│   │       ├── ultrasonic.c
│   │       ├── pir.c
│   │       └── esp8266.c
│   ├── Tasks/
│   │   ├── sweep_task.c
│   │   ├── sensor_task.c
│   │   ├── tracking_task.c
│   │   ├── state_machine.c
│   │   ├── telemetry_task.c
│   │   └── display_task.c
│   ├── Control/
│   │   └── pid.c
│   ├── Bench/
│   │   ├── microbench.c
│   │   └── microbench.h
│   ├── stm32_ls.ld
│   ├── stm32f411_startup.c
│   └── Makefile
│
├── ground-station/
│   ├── sentinel_gs.py
│   ├── radar_widget.py
│   ├── telemetry_parser.py
│   ├── command_sender.py
│   └── requirements.txt
│
├── docs/
│   ├── architecture.md
│   ├── wiring_diagram.png
│   ├── benchmark_results.md
│   └── demo_script.md
│
└── README.md
```

---

## Demo Video Script (3-5 minutes)

```
[0:00-0:15] Cold open
  Close-up of SENTINEL powering on. LCD: "SENTINEL ONLINE".
  LEDs glow green. Servo starts sweeping.

[0:15-0:30] Ground station
  Cut to laptop. Radar display shows green sweep line. Clean — no targets.
  "This is SENTINEL — an autonomous perimeter defense system I built from scratch."

[0:30-1:00] Architecture overview
  Voice over hardware closeup.
  "Custom RTOS running 6 concurrent tasks on a $15 microcontroller.
   Every driver written bare-metal — no HAL, no libraries."

[1:00-1:30] Detection demo
  Place a box in front of the sensor.
  Radar shows blip. LED goes yellow. LCD: "ALERT @ 45°".
  "The ultrasonic sensor detected an object at 45 degrees, 80cm away."

[1:30-2:15] Tracking demo
  Slowly move the box left and right.
  Servo follows. Laser stays on target. LED is red.
  "The PID controller tracks the object in real-time.
   That red dot is a laser marking the target."

[2:15-2:45] Alarm demo
  Leave the box stationary.
  Buzzer sounds. LEDs flash red. Ground station shows ALARM.
  "After 3 seconds in LOCKED state, the system escalates to ALARM."
  Press DISARM on ground station. System returns to PATROL.

[2:45-3:30] Benchmark results
  Show MicroBench output on UART terminal.
  "I built a benchmark suite to measure performance.
   My RTOS achieves Xµs context switches — Y% faster than FreeRTOS
   on the same hardware running the same workload."

[3:30-4:00] Code walkthrough
  Quick scroll through GitHub repo.
  "6,000+ lines of bare-metal C. Custom RTOS, PID controller,
   binary telemetry protocol, Python ground station."

[4:00-4:30] Closing
  Wide shot of the full setup — sensor tower + ground station.
  "SENTINEL demonstrates real-time embedded systems engineering:
   custom OS design, sensor fusion, control theory, and wireless telemetry.
   All from scratch on a $15 microcontroller."
```

---

## Success Metrics

### Technical
- [ ] Custom RTOS with preemptive priority scheduling
- [ ] 6 concurrent tasks running without deadline misses
- [ ] < 5µs context switch time
- [ ] < 20ms sweep-to-detect latency
- [ ] PID tracking within 5° accuracy
- [ ] 10Hz telemetry with < 1% packet loss
- [ ] Watchdog recovery from task hang
- [ ] MicroBench comparison vs FreeRTOS complete

### Portfolio
- [ ] GitHub repo with 100+ commits
- [ ] Professional README with architecture diagram
- [ ] 3-5 minute demo video
- [ ] Technical write-up / blog post
- [ ] Ground station in separate repo
- [ ] MicroBench results published

### Interview Talking Points
- "I built a preemptive RTOS from scratch and proved it outperforms FreeRTOS on real workloads"
- "The system autonomously detects, tracks, and responds to objects using PID control"
- "I designed a binary telemetry protocol with checksums for reliable wireless data streaming"
- "Every driver is bare-metal — I2C, UART, PWM, input capture, EXTI — no HAL libraries"
- "I created MicroBench, an embedded benchmark suite, to quantify real-time performance"

---

## Timeline Summary

| Weeks | Phase | Deliverable |
|-------|-------|-------------|
| 1-4   | RTOS Core | Context switching, priority scheduler, mutex, semaphore |
| 5-8   | Sensors | Servo PWM, ultrasonic, PIR, laser, buzzer, LEDs, integration |
| 9-12  | Intelligence | State machine, PID tracking, edge case handling, watchdog |
| 13-16 | Connectivity | ESP8266 WiFi, telemetry protocol, Python ground station |
| 17-18 | Benchmarks | MicroBench suite, FreeRTOS comparison, performance analysis |
| 19+   | Polish | Demo video, documentation, blog post, portfolio update |
