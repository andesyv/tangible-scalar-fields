rem @echo off

md ext
cd ext

rd /S /Q glfw
git clone https://github.com/glfw/glfw.git

rd /S /Q glm
git clone https://github.com/g-truc/glm.git

rd /S /Q glbinding
git clone https://github.com/cginternals/glbinding.git

rd /S /Q globjects
git clone https://github.com/cginternals/globjects.git

cd ..
md lib
cd lib

rd /S /Q imgui
git clone https://github.com/ocornut/imgui.git

rd /S /Q lodepng
git clone https://github.com/lvandeve/lodepng.git

rd /S /Q portable-file-dialogs
git clone --branch 0.1.0 https://github.com/samhocevar/portable-file-dialogs.git

cd ..