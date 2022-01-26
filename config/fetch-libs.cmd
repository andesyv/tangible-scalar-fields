rem @echo off

md ext
cd ext

rd /S /Q glfw
git clone --depth 1 --branch 3.3.6 https://github.com/glfw/glfw.git

rd /S /Q glm
rem Note: Whenever GLM releases 0.9.9.9 I should update this here. The master branch is the working revision of 0.9.9.9.
rem --branch 0.9.9.9
git clone --depth 1 https://github.com/g-truc/glm.git

rd /S /Q glbinding
git clone https://github.com/cginternals/glbinding.git
cd glbinding
rem Note: Latest working revision on the master branch at the time of writing
git checkout 70e76f9f1635652c83b2a6864bed3a50dd040ef6
cd ..

rd /S /Q globjects
git clone https://github.com/cginternals/globjects.git
cd globjects
rem Note: Latest working revision on the master branch at the time of writing
git checkout 72285e2e6c33670267d6f873a2aef43ba2d44b95
cd ..

cd ..
md lib
cd lib

rd /S /Q imgui
git clone --depth 1 --branch v1.86 https://github.com/ocornut/imgui.git

rd /S /Q lodepng
git clone https://github.com/lvandeve/lodepng.git

rd /S /Q portable-file-dialogs
git clone --depth 1 --branch 0.1.0 https://github.com/samhocevar/portable-file-dialogs.git

cd ..