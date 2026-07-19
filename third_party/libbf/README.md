# LibBF

This directory vendors LibBF 2025-06-03 by Fabrice Bellard from
<https://bellard.org/libbf/>.

Only `libbf.c`, `libbf.h`, `cutils.c`, and `cutils.h` are included. The local
build disables FFT multiplication because the calculator uses fewer than 100
binary limbs. Feature switches around the upstream `USE_FFT_MUL` and
`USE_BF_DEC` definitions are the only source changes; decimal support remains
enabled because LibBF's base-10 formatter shares parts of that implementation.

LibBF is distributed under the MIT license. See [LICENSE](LICENSE).
