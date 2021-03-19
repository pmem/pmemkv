# Testing Storage Engines for pmemkv

- [dram_vcmap](#dram_vcmap)

# dram_vcmap

A volatile concurrent engine backed by std::allocator. Data written using this engine is stored entierly in DRAM and lost after the database is closed.

This engine is variant of vcmap, which uses std::allocator to allocate memory. That means it is built on top of tbb::concurrent\_hash\_map data structure; std::basic\_string is used as a type of a key and a value.
TBB package is required.

This engine do not require any config parameters

