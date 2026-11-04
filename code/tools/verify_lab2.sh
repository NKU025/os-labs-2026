#!/usr/bin/env bash
# Repeatable tests. Run from WSL: bash tools/verify_lab2.sh
set -euo pipefail
cd "$(dirname "$0")/.."
report=../report
mkdir -p "$report"
python3 tools/check_page_init.py 2>&1 | tee "$report/page-init-boundaries.log"
for manager in first_fit best_fit buddy; do
    make -j2 PMM="$manager" 2>&1 | tee "$report/build-$manager.log"
    for ram in 64 128; do
        log="$report/$manager-${ram}M.log"
        echo "QEMU: PMM=$manager RAM=${ram}MiB" | tee "$log"
        rc=0
        timeout 8s "${QEMU:-qemu-system-riscv64}" -machine virt -m "$ram" \
            -nographic -bios default \
            -device loader,file=bin/ucore.img,addr=0x80200000 >> "$log" 2>&1 || rc=$?
        # Kernel intentionally spins after the checks. Timeout is expected.
        [ "$rc" -eq 124 ] || { cat "$log"; exit 1; }
        grep -q "memory management: ${manager/first_fit/default}_pmm_manager" "$log"
        grep -q 'check_alloc_page() succeeded!' "$log"
        grep -q '\[SLUB\] ALL TESTS PASSED' "$log"
        if [ "$manager" = buddy ]; then grep -q '\[Buddy\] ALL TESTS PASSED' "$log"; fi
        if grep -Eiq 'panic|FAIL' "$log"; then cat "$log"; exit 1; fi
        grep -E 'memory management|free memory|PASS' "$log"
    done
done
# Default best-fit is the course grading target. Always restore it afterwards.
make PMM=best_fit grade GRADE_TMP="/tmp/lab2-grade-$(id -u)-$$" 2>&1 | tee "$report/grade.log"
echo 'make grade exit code: 0' | tee -a "$report/grade.log"
make -j2 PMM=best_fit 2>&1 | tee "$report/build.log"
printf 'VERIFICATION PASSED: boundary tests, 6 QEMU runs, course grade\n'
