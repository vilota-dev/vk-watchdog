<div align="center">
  <a href="https://github.com/vilota-dev/vk-watchdog">
    <img src="resources/logo.png" alt="Logo" width="200" height="200">
  </a>

<h1 align="center">vk-watchdog</h1>
<p align="center">
Watchdog application to run on Vilota's vk suite of products, batteries included!</p>
</div>

This project includes various endpoints to handle system commands, system statistics, and file uploads.

Dependencies 
- [Crow](https://github.com/CrowCpp/Crow) (for web server functionality)
- [eCAL](https://github.com/eclipse-ecal/ecal/) (for process communication and monitoring)

## Build & Run
Make sure you have the Crow library, eCAL, so that the `find_package` command in CMakeLists.txt doesn't fail.
```shell
git submodule update --init --recursive
mkdir build && cd build
cmake ..
cmake --build . -j
```

| Endpoint         | Method     | Description                                                                                               |
|------------------|------------|-----------------------------------------------------------------------------------------------------------|
| `/`              | GET        | Renders the main HTML page with the WebSocket interface.                                                  |
| `/lsusb`         | GET        | Executes `lsusb` command on the host and returns the result as a plain text.                              |
| `/system_stats`  | GET        | Returns a string representing the system statistics, including uptime, RAM usage, and CPU loads.          |
| `/ws`            | WebSocket  | Manages WebSocket connections, enabling the execution of system commands and broadcasting results to all connected users. |
| `/upload_file`   | POST       | Accepts a multipart POST request with a file and saves it to the server.                                  |
| `/upload_json`   | POST       | Accepts a JSON object and prints it to the log.                                                           |


# Additional Information
- WebSocket Endpoint: The application supports WebSockets to execute shell commands provided by connected clients and returns the results. Sort of like a mini "shell"
- JSON Upload: A simple route to accept JSON data from clients.
- File Upload: Functionality to accept file uploads and save them on the server. Copied to the same directory as where the vk-wathcdog is running



