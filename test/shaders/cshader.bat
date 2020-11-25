call glslangValidator render.vert --target-env spirv1.5 --client vulkan100 -o render.vert.spv
call glslangValidator render.frag --target-env spirv1.5 --client vulkan100 -o render.frag.spv

call glslangValidator ray-closest-hit.rchit --target-env spirv1.5 --client vulkan100 -o ray-closest-hit.rchit.spv
call glslangValidator ray-generation.rgen --target-env spirv1.5 --client vulkan100 -o ray-generation.rgen.spv
call glslangValidator ray-miss.rmiss --target-env spirv1.5 --client vulkan100 -o ray-miss.rmiss.spv

call glslangValidator ray-query.comp --target-env spirv1.5 --client vulkan100 -o ray-query.comp.spv

pause
