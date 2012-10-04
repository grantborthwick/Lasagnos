cd ../..
git pull origin p2
cd src/threads
make clean
make
cd build
pintos -v -- run alarm-negative