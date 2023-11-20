# fuzz-target-func-finder

## Overview
This tool finds file open-close function pairs, which can be used in fuzzing.
It gives information address of CreateFile(), CloseHandle() at same file, and parent function of them.
When the target program is 32bit executable, use TargetFinder32.exe. Otherwise(64bit executable), use TargetFinder64.exe

## Examples

### Normal Usage
![image](https://github.com/oriotie21/fuzz-target-func-finder/assets/79503775/bf074622-4db3-4f9e-8618-13af766b8307)

### Use with filename argument(-f)
![image](https://github.com/oriotie21/fuzz-target-func-finder/assets/79503775/8d1cbdf4-27f6-4967-a29e-ed3e9064e526)
