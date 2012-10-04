cd ../..
git pull origin p
cd src/threads
make
cd build
pintos -v -- run alarm-multiple