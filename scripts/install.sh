export CC=clang
export CXX=clang++
export npm_config_clang=1

git clone https://github.com/creationix/nvm.git .nvm
./.nvm
source ./.nvm/nvm.sh

if [ "$RUNTIME" == "electron" ]; then
  echo "Building electron $TARGET"
  export npm_config_target=$TARGET
  export npm_config_arch=$TARGET_ARCH
  export npm_config_target_arch=$TARGET_ARCH
  export npm_config_disturl=https://atom.io/download/electron
  export npm_config_runtime=electron
  export npm_config_build_from_source=true
fi

nvm install $NODE_VERSION

if [ -z "$TARGET" ]; then
  export TARGET=$(node -v | sed -e '1s/^.//')
fi

HOME=~/.electron-gyp npm install --build-from-source
