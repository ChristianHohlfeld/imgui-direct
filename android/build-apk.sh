#!/usr/bin/env bash
set -euo pipefail
: "${ANDROID_HOME:?`ANDROID_HOME` is required}"
: "${ANDROID_NDK_HOME:?`ANDROID_NDK_HOME` is required}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TOOLS="$(find "$ANDROID_HOME/build-tools" -mindepth 1 -maxdepth 1 -type d | sort -V | tail -1)"
PLATFORM="$ANDROID_HOME/platforms/android-35/android.jar"
if [[ ! -f "$PLATFORM" ]]; then
  echo "Android SDK platform android-35 must be installed" >&2
  exit 1
fi
mkdir -p "$ROOT/build-android-java/classes" "$ROOT/build-android-java/dex" "$ROOT/build-android-java/package/lib/arm64-v8a"
javac -source 8 -target 8 -cp "$PLATFORM" -d "$ROOT/build-android-java/classes" \
  "$ROOT/android/ImGuiInputActivity.java"
mapfile -t CLASSES < <(find "$ROOT/build-android-java/classes" -name '*.class')
"$TOOLS/d8" --min-api 26 --lib "$PLATFORM" --output "$ROOT/build-android-java/dex" "${CLASSES[@]}"
"$TOOLS/aapt2" link -o "$ROOT/build-android-java/base.apk" -I "$PLATFORM" \
  --manifest "$ROOT/android/AndroidManifest.xml" --min-sdk-version 26 --target-sdk-version 35
cp "$ROOT/build-android/libimgui_direct.so" "$ROOT/build-android-java/package/lib/arm64-v8a/"
cp "$ROOT/build-android-java/dex/classes.dex" "$ROOT/build-android-java/package/"
(cd "$ROOT/build-android-java/package" && zip -q -r "$ROOT/build-android-java/base.apk" .)
"$TOOLS/zipalign" -f 4 "$ROOT/build-android-java/base.apk" "$ROOT/build-android-java/aligned.apk"
KEY="$ROOT/build-android-java/debug.keystore"
if [[ ! -e "$KEY" ]]; then
  keytool -genkeypair -noprompt -keystore "$KEY" -storepass android -keypass android \
    -alias androiddebugkey -dname "CN=Android Debug,O=ImGui Direct,C=DE" \
    -keyalg RSA -keysize 2048 -validity 3650
fi
"$TOOLS/apksigner" sign --ks "$KEY" --ks-key-alias androiddebugkey \
  --ks-pass pass:android --key-pass pass:android \
  --out "$ROOT/build-android-java/imgui-direct-debug.apk" "$ROOT/build-android-java/aligned.apk"
"$TOOLS/apksigner" verify "$ROOT/build-android-java/imgui-direct-debug.apk"
echo "APK: $ROOT/build-android-java/imgui-direct-debug.apk"
