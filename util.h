#pragma once

#define DIV_ROUND_UP(n, d) ({ \
    typeof (n) _n = (n); \
    typeof (d) _d = (d); \
    (_n + _d - 1) / _d; \
})

#define MIN(a, b) ({ \
    typeof (a) _a = (a); \
    typeof (b) _b = (b); \
    _a < _b ? _a : _b; \
})

#define MAX(a, b) ({ \
    typeof (a) _a = (a); \
    typeof (b) _b = (b); \
    _a > _b ? _a : _b; \
})
