#3D Collectibles Game – Ancient East Asian Warriors

📜 Overview

This project is a 3D collectibles game developed using OpenGL.
Players explore four unique platforms inspired by Ancient East Asian warrior culture — collecting items, triggering animations, and racing against time.

🎮 Game Concept

The player controls a 3D character that can move across four distinct platforms.

Each platform features:

A unique 3D object built entirely from OpenGL primitives.

Several collectible items (minimum 3 per platform).

When all collectibles on a platform are gathered:

The associated 3D object performs a unique continuous animation:

Rotation

Scaling

Transformation

Color change

Once all collectibles are gathered on the final platform:

A “Game Win” message appears while player control remains active.

If time expires before collecting all items:

The scene transitions to a “Game Over” screen.

🧩 Features

Fully modeled with OpenGL primitives — no imported assets.

Camera system supporting multiple views:

Top, Side, and Front perspectives.

Keyboard & mouse controls for both movement and camera.

Collision detection preventing wall and object penetration.

Animations toggleable after collectibles are gathered.

Optional bonus features:

Sound effects for background music, item collection, winning, and losing.

Advanced graphics with highly detailed models (≥10 primitives each).

⌨️ Controls
Action	Key(s)
Move	W, A, S, D or Arrow Keys
Toggle Platform Animations	Assigned keys (custom per platform)
Camera Control	Keyboard and/or Mouse
Change View	Top / Side / Front

🏗️ Technical Requirements

Language: C++

Graphics Library: OpenGL

Minimum objects:

3 bounding walls

1 ground plane

1 main character (≥6 primitives)

4 unique 3D objects (≥4 primitives each)

4 platforms (≥2 primitives each)

12+ collectibles (≥3 primitives each)

⚙️ Build & Run Instructions
🪟 Windows (Visual Studio)

Install Visual Studio with the Desktop development with C++ workload.

Install FreeGLUT or GLFW libraries.

Link the required libraries in your project:

opengl32.lib

glu32.lib

freeglut.lib (or glfw3.lib if using GLFW)

Place the header and DLL files in your project’s include/lib folders.

Compile and run PXX_YYYY.cpp.

🐧 Linux / macOS (g++)

Ensure OpenGL and GLUT are installed:

sudo apt install freeglut3-dev


Compile your file with:

g++ PXX_YYYY.cpp -o game -lGL -lGLU -lglut


Run the game:

./game


🕹️ Gameplay Flow

Move the player across platforms and collect all items.

Trigger animations after completing each platform.

Achieve a full win by collecting all collectibles before time runs out.

Experience a “Game Over” screen if time expires.

🧠 Notes

All modeling and animation are implemented manually using OpenGL primitives.

No pre-built models or external assets are allowed.

The project focuses on 3D modeling, camera control, and basic game mechanics.
