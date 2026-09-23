#!/bin/bash

# Create the macOS Application Bundle structure
APP_DIR="pkg_root/Applications/BTNRH/ABRAV.app"
mkdir -p "$APP_DIR/Contents/MacOS"
mkdir -p "$APP_DIR/Contents/Resources"

# Create a basic Info.plist
cat <<PLIST_EOF > "$APP_DIR/Contents/Info.plist"
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>ABRAV_Launcher</string>
    <key>CFBundleIdentifier</key>
    <string>org.btnrh.abrav</string>
    <key>CFBundleName</key>
    <string>ABRAV</string>
    <key>CFBundleIconFile</key>
    <string>abrav.icns</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>3.37</string>
    <key>LSMinimumSystemVersion</key>
    <string>10.13</string>
</dict>
</plist>
PLIST_EOF

# Create a Launcher Script to set ABRAV.INI and change to user's home dir
cat <<'LAUNCHER_EOF' > "$APP_DIR/Contents/MacOS/ABRAV_Launcher"
#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$HOME/Documents"
exec env "ABRAV.INI=$DIR/../Resources/abrav.ini" "$DIR/abrav"
LAUNCHER_EOF
chmod +x "$APP_DIR/Contents/MacOS/ABRAV_Launcher"

# Copy the actual binary and resources
cp abrav "$APP_DIR/Contents/MacOS/"
chmod +x "$APP_DIR/Contents/MacOS/abrav"
cp abrav.ini "$APP_DIR/Contents/Resources/"
cp abrav.icns "$APP_DIR/Contents/Resources/"
cp 93I14A00.ABR "$APP_DIR/Contents/Resources/" 2>/dev/null || true

# Build the .pkg installer
echo "Building ABRAV_App.pkg..."
pkgbuild --identifier org.btnrh.abrav.app          --version 1.0          --root pkg_root          --install-location /          ABRAV_App.pkg

# Cleanup
rm -rf pkg_root

echo "Done! The new App Installer is available as ABRAV_App.pkg"
