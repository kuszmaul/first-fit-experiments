How good is first fit?

The best experimental paper Bradley has found is
 On the external storage fragmentation produced by first-fit and best-fit allocation strategies
 John E. Shore
 Communications of the ACM, Volume 18, Issue 8, Aug. 197, 5pp 433–440
 https://doi.org/10.1145/360933.360949

Bradley's interpretation is that Shore found that the greater the variation, the
better first-fit allocation performs compared to best fit.  For uniform
distributions (small variance) best fit is better.  For exponential distribuions
(medium variance), they are about the same.  For hyperexponential distributions
first fit is better.

Shore also conjectures that first fit wins because it tends to put large objects
far to the right.

One shortcoming of SHore is that the memory was only 32KiB.  It seems likely
that things are quantitatively different for 64GiB than 32KiB (2^36 vs 2^15).

This code is an attempt to redo some of Shore's experiments with larger memory
and more modern data structures.
