forfiles /p glsl /m *.frag /c "cmd /c C:/VulkanSDK/1.3.283.0/Bin/glslc.exe @file -o ../spv/@fname.spv"
forfiles /p glsl /m *.vert /c "cmd /c C:/VulkanSDK/1.3.283.0/Bin/glslc.exe @file -o ../spv/@fname.spv"
forfiles /p glsl /m *.comp /c "cmd /c C:/VulkanSDK/1.3.283.0/Bin/glslc.exe @file -o ../spv/@fname.spv"


pause
