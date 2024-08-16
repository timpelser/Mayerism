#!/bin/bash

echo "Formatting..."

clang-format -i $(git ls-files "*.h" "*.cpp") 

echo "Formating Complete!"
