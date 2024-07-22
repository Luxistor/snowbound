Snowbound is a renderer in Vulkan I've been working on. 
I created it with the goal of using as few external libraries as possible; the only dependency is stb_image, objs are converted to sbm files offline. The meshes/textures used arent of my own creation, they were free assets I found.

The game I created as a demonstration is based on an older arcade game called "Thin Ice", which I used to really enjoy playing.

The renderer I implemented for the game itself is deferred, it has shadows, ambient occlusion, and blinn-phong lighting.

The engine itself can render an entire scene in a single indirect draw call; it's GPU driven. In the future, I'd like to add frustum and depth occlusion calling, for now, there isnt any culling that occurs in the compute shader.
It's bindless, meaning there's a single global descriptor set, one index buffer, one vertex buffer, and buffers are passed through Buffer Device Addresses (buffer pointers). This paired with the indirect drawing also allows me to bake command buffers and avoid re-recording every frame.

For per-material things like water, I take an "ubershader" approach, where instead of binding several pipelines there is a shader ID I pass in, and I perform a switch case statement to perform actions based on materials. Switching like this is nearly zero overhead on more recent gpus, and snowbound only targets gpus made in the last ~10 years anyway. 

It currently only supports Win32, but I've set it up in a way such that I can add support for other platforms in the future.

Video Demo:
https://www.youtube.com/watch?v=vayX9Duvcpg
