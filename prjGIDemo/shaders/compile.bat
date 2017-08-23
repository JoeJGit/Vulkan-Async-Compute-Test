del *.spv

glslangValidator -V texUB.vert -o texUB-vert.spv
glslangValidator -V texSB.vert -o texSB-vert.spv
glslangValidator -V tex.frag -o tex-frag.spv
glslangValidator -V vis.vert -o vis-vert.spv
glslangValidator -V vis.frag -o vis-frag.spv
glslangValidator -V imgui.vert -o imgui-vert.spv
glslangValidator -V imgui.frag -o imgui-frag.spv



del test0-64.comp
mcpp -a -D "API_VK" -D "TASK = 0" -D "WG_WIDTH = 256"  gi\test0.comp.glsl -P test0-64.comp
glslangValidator -V test0-64.comp  -o test0-64-comp.spv

del test1-64.comp
mcpp -a -D "API_VK" -D "TASK = 1" -D "WG_WIDTH = 64"  gi\test0.comp.glsl -P test1-64.comp
glslangValidator -V test1-64.comp  -o test1-64-comp.spv

del test2-64.comp
mcpp -a -D "API_VK" -D "TASK = 2" -D "WG_WIDTH = 64"  gi\test0.comp.glsl -P test2-64.comp
glslangValidator -V test2-64.comp  -o test2-64-comp.spv

del test3-64.comp
mcpp -a -D "API_VK" -D "TASK = 3" -D "WG_WIDTH = 64"  gi\test0.comp.glsl -P test3-64.comp
glslangValidator -V test3-64.comp  -o test3-64-comp.spv


