This folder contains project for running forward and backward compatibility tests between
different versions of pmemkv.

To run those tests:

Make sure that pmemkv-1.0 is installed and run:
```
mkdir build
cd build
cmake .. -DPMEMKV_PATHS="/opt/pmemkv-1.0.1-rc1;/opt/pmemkv-1.0.1"
make
ctest
```

where PMEMKV_PATHS contains list of paths to pmemkv installations which should be tested.
Those paths should NOT contain 'lib' component (e.g. the path should be /opt/pmemkv rather
than /opt/pmemkv/lib).
