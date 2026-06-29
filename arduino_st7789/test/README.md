# Introduction
A small test harness to simulate the st7789 display and visualise it in the browser.

## Build instructions
1. Start shell with development environment containing C++ compiler
2. Configure cmake: ```cmake . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=1```
3. Build program: ```cmake --build build```

## Server instructions
1. Change to ```scripts``` folder: ```cd ./scripts```
2. Create virtual environment: ```python -m venv venv```
3. Activate virtual environmetn: ```source ./venv/scripts/activate```
4. Install python packages: ```pip install -r requirements.txt```
5. Start server: ```python ./server.py```
6. Open browser to visualiser: ```http://localhost:8080```
7. Press run to execute program and simulate the st7789's response after recompiling
