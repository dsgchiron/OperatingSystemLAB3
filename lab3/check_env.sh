#!/bin/sh
# Перевіряє, що середовище придатне для ЛР3. Запуск: sh check_env.sh
status=0
need() {
    if command -v "$1" >/dev/null 2>&1; then
        echo "ok      $1"
    else
        echo "MISSING $1  ->  sudo apt install $2"
        status=1
    fi
}
need gcc build-essential
need make build-essential
need gdb gdb
need timeout coreutils
need taskset util-linux
need setarch util-linux
if dpkg -s libc6-dbg >/dev/null 2>&1; then
    echo "ok      libc6-dbg"
else
    echo "MISSING libc6-dbg  ->  sudo apt install libc6-dbg"
    status=1
fi

cpus=$(nproc)
if [ "$cpus" -ge 2 ]; then
    echo "ok      $cpus vCPU"
else
    echo "FAIL    лише $cpus vCPU: додайте VM щонайменше 2, інакше потоки не виконуватимуться паралельно"
    status=1
fi

tmp=$(mktemp -d) || exit 1
printf '#include <pthread.h>\nint shared;\nvoid *f(void *a){shared++; return a;}\nint main(void){pthread_t t; pthread_create(&t, 0, f, 0); shared++; pthread_join(t, 0); return 0;}\n' > "$tmp/t.c"
if gcc -fsanitize=thread -g -pthread "$tmp/t.c" -o "$tmp/t" 2>"$tmp/build.log" &&
    ! "$tmp/t" >"$tmp/tsan.log" 2>&1 &&
    grep -q 'WARNING: ThreadSanitizer: data race' "$tmp/tsan.log"; then
    echo "ok      ThreadSanitizer"
else
    echo "FAIL    ThreadSanitizer не виявив race condition у тестовій програмі; див. діагностику нижче."
    cat "$tmp/build.log" "$tmp/tsan.log" 2>/dev/null
    if grep -q 'unexpected memory mapping' "$tmp/tsan.log" 2>/dev/null; then
        echo "Виконайте: sudo sysctl vm.mmap_rnd_bits=28"
    fi
    status=1
fi
rm -rf "$tmp"

echo "kernel: $(uname -r), arch: $(uname -m), gcc: $(gcc -dumpversion)"
[ $status -eq 0 ] && echo "Усе готово." || echo "Є проблеми, див. вище."
exit $status
