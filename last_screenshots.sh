#!/bin/bash
N=${1:-1}
ls -t screenshots/*.png 2>/dev/null | head -n "$N"
