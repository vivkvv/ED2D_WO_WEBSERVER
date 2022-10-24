set PATH=%PATH%;c:\Python27;d:\Utils\emsdk;d:\GitLabProjects\electodynamics2d_websocket\WebClient\protobuf-3.6.1\src\;
call emsdk activate latest
call emsdk_env
call emcc -Id:\GitLabProjects\electodynamics2d_websocket\WebClient\protobuf-3.6.1\src\ -o d:\GitLabProjects\electodynamics2d_websocket\WebClient\js\toJS.js d:\GitLabProjects\electodynamics2d_websocket\Room\toJS.cpp -O1 -s WASM=1 -s EXPORTED_FUNCTIONS="['_recalculate', '_recalculate_test']" -s NO_EXIT_RUNTIME=1  -s "EXTRA_EXPORTED_RUNTIME_METHODS=['ccall', 'cwarp']" -std=c++11