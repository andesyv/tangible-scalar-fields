#!/usr/bin/bash
set -e # Abort script on non-zero exit codes

echo "Build started."

export PROJECT_HOME=`pwd`
echo "PROJECT_HOME=$PROJECT_HOME"

cd ext

declare -A options
options[glfw]="-DBUILD_SHARED_LIBS=OFF -DGLFW_BUILD_DOCS=OFF -DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_TESTS=OFF"
options[glm]="-DBUILD_SHARED_LIBS=OFF -DBUILD_STATIC_LIBS=ON -DGLM_TEST_ENABLE=OFF"
options[glbinding]="-DBUILD_SHARED_LIBS=OFF -DOPTION_BUILD_EXAMPLES=OFF -Dglfw3_DIR=$PROJECT_HOME/lib/lib64/cmake/glfw3 -DOPTION_BUILD_TOOLS=ON" # -DCMAKE_CXX_FLAGS=\"-w\"
options[globjects]="-DBUILD_SHARED_LIBS=OFF -DOPTION_BUILD_EXAMPLES=OFF" # -DCMAKE_CXX_FLAGS=\"-w\"

for name in glfw glm glbinding globjects
do
    echo "Configuring $name:"
    cd $PROJECT_HOME/ext/$name
    [ -d ./build ] && rm -rf build
    mkdir build && cd build
    cmake .. -DCMAKE_INSTALL_PREFIX:PATH="$PROJECT_HOME/lib/$name" -DCMAKE_PREFIX_PATH="$PROJECT_HOME/lib" ${options[$name]}
    echo "Building $name"
    make -j install
    echo "Done."
done

echo "Done."

