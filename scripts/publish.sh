if [ -z "$TARGET" ]; then
  export TARGET=$(node -v | sed -e '1s/^.//')
fi

./node_modules/.bin/node-pre-gyp package --runtime=$RUNTIME --target=$TARGET
./node_modules/.bin/node-pre-gyp publish --runtime=$RUNTIME --target=$TARGET
