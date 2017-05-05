nvm unload || true
rm -rf ./__nvm/ && git clone --depth 1 https://github.com/creationix/nvm.git ./__nvm
source ./__nvm/nvm.sh
nvm install ${NODE_VERSION}
nvm use ${NODE_VERSION}
node --version
npm --version
which node

if [ "$RUNTIME" == "electron" ]; then
  echo "Building electron $TARGET"
  export npm_config_target=$TARGET
  export npm_config_arch=$TARGET_ARCH
  export npm_config_target_arch=$TARGET_ARCH
  export npm_config_disturl=https://atom.io/download/electron
  export npm_config_runtime=electron
  export npm_config_build_from_source=true
fi

if [ -z "$TARGET" ]; then
  export TARGET=$(node -v | sed -e '1s/^.//')
fi

HOME=~/.electron-gyp npm install --build-from-source
