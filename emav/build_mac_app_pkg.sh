#!/bin/bash

# Create the macOS Application Bundle structure
APP_DIR="pkg_root/Applications/BTNRH/EMAV.app"
mkdir -p "$APP_DIR/Contents/MacOS"
mkdir -p "$APP_DIR/Contents/Resources"

# Create a basic Info.plist
cat <<EOF > "$APP_DIR/Contents/Info.plist"
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>EMAV_Launcher</string>
    <key>CFBundleIdentifier</key>
    <string>org.btnrh.emav</string>
    <key>CFBundleName</key>
    <string>EMAV</string>
    <key>CFBundleIconFile</key>
    <string>emav.icns</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>3.37</string>
    <key>LSMinimumSystemVersion</key>
    <string>10.13</string>
</dict>
</plist>
EOF

# Create a Launcher Script to set EMAV.INI and change to user's home dir
# (so data files default to saving in their Documents folder rather than root)
cat <<'EOF' > "$APP_DIR/Contents/MacOS/EMAV_Launcher"
#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$HOME/Documents"
exec env "EMAV.INI=$DIR/../Resources/emav.ini" "$DIR/emav"
EOF
chmod +x "$APP_DIR/Contents/MacOS/EMAV_Launcher"

# Copy the actual binary and resources
cp emav "$APP_DIR/Contents/MacOS/"
chmod +x "$APP_DIR/Contents/MacOS/emav"
cp emav.ini "$APP_DIR/Contents/Resources/"
cp emav.icns "$APP_DIR/Contents/Resources/"

# Build the .pkg installer
echo "Building EMAV_App.pkg..."
pkgbuild --identifier org.btnrh.emav.app \
         --version 3.37 \
         --root pkg_root \
         --install-location / \
         EMAV_App.pkg

# Cleanup
rm -rf pkg_root

echo "Done! The new App Installer is available as EMAV_App.pkg"
