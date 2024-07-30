if [ -d "build" ]; then
    echo "Build directory exists"
    rm -rf build
    echo "Build directory removed"
    mkdir build
    echo "Build directory created"
else 
    mkdir build
    echo "Build directory created"
fi

cd build 

cmake ..

make

mkdir server_file # create a directory to store server files
