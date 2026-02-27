# CS 523 Project Log
### By: Stephen Duffy

# Project Proposal
- This project will be conducted on a project named particle life. This is a particle simulation program that simulates simple interactions between a few types of particles.
- To make this project more efficient I will use threads to do physics computations in parallel reducing the time it takes to compute 1 physics frame
- My work can be found at the following repository: [Link](https://github.com/Stephen-Duffy-SunyPoly/particle-life-parallel)

---

## Week 1 (1/25/2026 - 1/31/2026)
- Start looking for things that can be / are parallelized   


## Week 2 (2/1/2026 - 2/7/2026)
- Branch out into different topic idea categories: 
  - Image processing program
  - Text processing tools
  - Compression tool - this seems extremely challenging 
  - Ray tracing simulation - ok now this one would be extremely challenging 
    - potential project programs: [raytracing.github.io](https://github.com/RayTracing/raytracing.github.io) based off books so potential for good documentaiton
  - Cellular automata
- I found this particle simulation that looks really cool and I think I will do the project on this [particle-life](https://github.com/hunar4321/particle-life)

## Week 3 (2/8/2026 - 2/14/2026)
- Started to work with particle life a bit
- Got the project building as is on my system
- started working on converting the project to a more portable CMake project
- Successfully converted to a CMake project 
notably: when compiling a debug binary the performance is significantly worse

## Week 4 (2/15/2025 - 2/21/20206)
- Initial measured performance: 
  - 960 of each particle type: ~38FPS physics: ~25ms
  - 1984 of each particle type: ~10FPS physics: ~95ms
  - 3968 of each particle type: ~2FPS physics: ~350ms
- Changed the particle colors to be the same as on the web demo
- General concept for parallelizing:
  - Use regular threads for parallelism
  - 2 different classes of threads:
    - Compute thread: These threads will calculate the velocity change for each particle of a particular color from all particles of another color. In addition, they will wait to be triggered by a parent thread.
    - Color manager thread: These threads will do what compute threads do as well as reduce the output for each of the compute threads it controls, and wait to be triggered by the main thread. These threads will be responsible for creating all the compute threads necessary to have 1 thread per color to check against.
  - The main render thread will bre respectables for creating 1 Color management thread for each color of particles.
  - In effect this will create a tree like structure of threads where the main thread creates the 4 manager threads and each manager creates 3 compute thread. Each computation frame will be triggered in the same manor.
  
## Week 5 (2/22/2026 - 2/28/2026)
- worked on getting the code building on Linux:
  - needed to system install the following libraries:
    - libglu1-mesa-dev 
    - freeglut3-dev 
    - mesa-common-dev
    - libfontconfig1-dev
  - I ran into an issue where an instance variable in one of the libraries was not getting initialized and was for some reason null.
  - This seg fault appears to be caused by a class whose this pointer spontaneously becomes null. After chasing this issue for a long time I have decided that Linux support is not viable at this time and as such I will not pursue it any longer