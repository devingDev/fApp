rm -rf build/
mkdir build
cd build
cmake ..
make -j64

echo " BUILD FINISHED AT : $(date)"
