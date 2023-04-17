cd external
git clone https://github.com/meltwater/served.git
cd served
mkdir cmake.build && cd cmake.build
cmake ../ && make
sudo make install
