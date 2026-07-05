# Introduction
A small test harness to simulate the st7789 display and visualise it in the browser.

## 1. Setup python environment
1. Change to ```scripts``` folder: ```cd ./scripts```
2. Create virtual environment: ```python -m venv venv```
3. Activate virtual environmetn: ```source ./venv/scripts/activate```
4. Install python packages: ```pip install -r requirements.txt```

## Build instructions
1. Activate python environment
2. Generate font glyphs: ```cd scripts && ./render_all_glyphs.sh```
3. Start shell with development environment containing C++ compiler
4. Configure cmake: ```cmake . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=1```
5. Build program: ```cmake --build build```

## Server instructions
1. Activate python environment
2. Start server: ```python ./server.py```
3. Open browser to visualiser: ```http://localhost:8080```
4. Press run to execute program and simulate the st7789's response after recompiling
