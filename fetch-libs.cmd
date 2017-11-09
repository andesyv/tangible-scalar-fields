@echo off

md 3rdparty
cd 3rdparty

rd /S /Q glfw
git clone https://github.com/glfw/glfw.git

rd /S /Q glm
git clone https://github.com/g-truc/glm.git

rd /S /Q glbinding
git clone https://github.com/cginternals/glbinding.git

rd /S /Q globjects
git clone https://github.com/cginternals/globjects.git
