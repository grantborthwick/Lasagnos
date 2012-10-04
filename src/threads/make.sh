cd ../..
git pull origin p
cd src/threads
make clean
make
build/pintos -v -- run alarm-priority