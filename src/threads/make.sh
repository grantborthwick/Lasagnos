cd ../..
git pull origin p
cd src/threads
make clean
make
cd build
pintos -v -- run alarm-priority