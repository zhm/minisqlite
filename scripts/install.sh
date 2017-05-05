export CC=clang
export CXX=clang++
export npm_config_clang=1

HOME=~/.electron-gyp npm install --build-from-source
