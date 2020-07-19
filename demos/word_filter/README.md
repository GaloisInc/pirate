This directory contains different versions of a word filter
for assessing Pirate annotations.

* `unpartitioned` contains a version with a single executable and no
  partititoning.
* `enclaves` uses enclaves and `libpirategetopt`.
* `pal` uses enclaves and `pal`.
* `aadl` contains an AADL model.

The unpartitioned version is significantly simpler than the other
two due to the partitioning code and channel work.  We are
investigating approaches to simplifying it.