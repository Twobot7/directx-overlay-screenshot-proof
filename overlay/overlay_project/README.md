# Overlay Application

## Overview

The Overlay Application is a Windows-based program designed to create a transparent, click-through overlay on the screen. It utilizes DirectX for rendering and desktop duplication to capture and analyze screen content. The application detects movement on the screen and renders semi-transparent boxes around moving objects.

## Features

- **Transparent Overlay**: The application creates a fully transparent window that overlays the entire screen. It is click-through, meaning it does not interfere with user interactions with other applications.
- **Movement Detection**: The application captures frames from the desktop and detects movement by comparing consecutive frames. It highlights areas of movement with semi-transparent boxes.
- **DirectX Rendering**: Utilizes DirectX for efficient rendering of graphics, ensuring smooth performance.
- **Desktop Duplication**: Leverages desktop duplication APIs to capture screen content without affecting performance.
- **Debug Console**: Provides detailed logging of application events and errors for debugging purposes.

## Components

### Main Components

- **DirectX Initialization**: Sets up DirectX device, swap chain, and render target for rendering the overlay.
- **Shader Compilation**: Compiles vertex and pixel shaders for rendering graphics.
- **Desktop Duplication**: Initializes desktop duplication to capture screen frames.
- **Movement Detection**: Compares pixel data between frames to detect movement and calculate bounding boxes.
- **Rendering**: Draws semi-transparent boxes around detected movement areas using DirectX.

### Functions

- `InitDirectX(HWND hwnd)`: Initializes DirectX components.
- `InitShaders()`: Compiles and sets up shaders for rendering.
- `InitDesktopDuplication(ID3D11Device* device)`: Sets up desktop duplication for frame capture.
- `CaptureFrame()`: Captures a frame from the desktop for analysis.
- `DetectMovement()`: Detects movement by comparing current and previous frames.
- `RenderOverlay()`: Renders boxes around detected movement areas.
- `RenderFrame()`: Clears the render target and draws the overlay.
- `UpdateObjectPositions()`: Updates positions of moving objects for demonstration purposes.
- `LogError(const std::string& message)`: Logs error messages to the console and displays a message box.
- `LogInfo(const std::string& message)`: Logs informational messages to the console.

## Usage

1. **Build the Application**: Compile the source code using a compatible C++ compiler with DirectX SDK.
2. **Run the Application**: Execute the compiled binary. The application will create a transparent overlay on the screen.
3. **Observe Movement Detection**: Move windows or objects on the screen to see the overlay highlight areas of movement.
4. **Debugging**: Use the console output to monitor application events and diagnose issues.

## Requirements

- Windows operating system
- DirectX SDK
- C++ compiler with support for C++11 or later

## Troubleshooting

- **White Screen**: Ensure that the window is set to be transparent and that the rendering logic maintains transparency.
- **Application Closes Unexpectedly**: Check the console output for error messages and ensure all resources are initialized correctly.
- **No Movement Detection**: Verify that desktop duplication is initialized and frames are being captured successfully.

## License

This project is licensed under the MIT License. 