#!/bin/bash

TEST_CASES=("" "-a" "-A" "-l" "-lh" "-la" "-lt" "-lS" "-lF" "-R")
TARGET="." 

echo "=== BẮT ĐẦU KIỂM THỬ TỰ ĐỘNG ==="

for flag in "${TEST_CASES[@]}"; do
    echo -n "Đang kiểm tra: ./my_ls $flag ... "
    
    ls $flag "$TARGET" | grep -v "^total" > /tmp/expected.txt
    ./my_ls $flag "$TARGET" | grep -v "^total" > /tmp/actual.txt
    
    # So sánh
    if diff -u /tmp/expected.txt /tmp/actual.txt > /tmp/diff_res.txt; then
        echo -e "\033[32m[PASS]\033[0m"
    else
        echo -e "\033[31m[FAIL]\033[0m"
        cat /tmp/diff_res.txt | head -n 10
    fi
done

rm -f /tmp/expected.txt /tmp/actual.txt /tmp/diff_res.txt
