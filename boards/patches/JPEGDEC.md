# JPEGDEC compatibility patch

Pinned revision: see `dependencies.json`. `prepare.py` replaces the progressive refinement expression `((-1) << cApproxBitsLow)` with `-iPositive`, using the positive value computed immediately above it. This preserves the intended value and avoids a signed-negative left shift rejected by GCC 15. Copyright and Apache-2.0 notices remain in the dependency. The replacement is idempotent and fails closed if its context is absent.
