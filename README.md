# Focus Driving Simulator

A gamified focus tracker that visualizes your focus sessions as driving a car through a futuristic instrument cluster.

## Requirements

- Qt 6.x (Qt Widgets + Qt Multimedia modules)
- C++17 compatible compiler
- Qt Creator 8+ recommended

## How to Open

1. Launch **Qt Creator**
2. `File → Open File or Project`
3. Select `FocusDrivingSimulator.pro`
4. Configure the kit (Qt 6.x Desktop)
5. Build & Run (`Ctrl+R`)

## Features

### Focus Session
- Click **▶ START** to begin a focus session
- The speedometer fills as your focus time grows (momentum curve: `speed = 180 * (1 - e^(-t/1200))`)
- Every **25 minutes** (1500 seconds) the gear shifts up, triggering a dashboard flash and animation

### Distraction Detection
- **Window focus loss**: switching away deducts 10 km/h
- **Excessive clicking** (>5 clicks/sec): deducts 5 km/h
- **Idle** (no keyboard/mouse for 30+ seconds): speed decreases at 0.2 km/h per idle second

### Journey Map
- Watch a small car travel from **HOME → FOREST → MOUNTAIN → TUNNEL → CITY → DESTINATION**
- Progress bar shows session completion percentage

### Controls
| Button | Action |
|--------|--------|
| ▶ START | Begin focus session |
| ⏸ PAUSE | Pause timer |
| ↺ RESET | Reset everything to zero |

## Architecture

```
main.cpp                  - Entry point
mainwindow.h/cpp          - Top-level QMainWindow
dashboardwidget.h/cpp     - Main UI container, layout & flash effects
focusengine.h/cpp         - Timer logic, speed model, distraction/idle detection
speedgaugewidget.h/cpp    - Animated analog speedometer (QPainter)
gearwidget.h/cpp          - Gear indicator with shift animation
mapwidget.h/cpp           - Journey map with animated car icon
```

## Visual Style

- Dark futuristic instrument cluster aesthetic
- Neon blue / cyan / orange color palette
- Smooth needle animation via `QPropertyAnimation`
- Glow effects on gear shifts
- Grid-overlay HUD background
