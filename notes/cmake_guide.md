# Useful cmake commands

- Configure cmake to export compile commands for embedded systems: ```cmake . -B build -G Ninja -DCMAKE_BUILD_TYPE=MinimumSize -DCMAKE_EXPORT_COMPILE_COMMANDS=1```
- Cmake build: ```cmake --build build```
- Clang tidy with cmake compile commands: ```clang-tidy -p build/compile_commands.json --checks=misc-include-cleaner `find <dir0> <dir1> -name *.cpp -o -name *.h -type f` ```