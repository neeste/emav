import os

# Create abrav/build_mac_app_pkg.sh
script_content = """#!/bin/bash

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
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0</string>
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
cp 93I14A00.ABR "$APP_DIR/Contents/Resources/" 2>/dev/null || true

# Build the .pkg installer
echo "Building ABRAV_App.pkg..."
pkgbuild --identifier org.btnrh.abrav.app \
         --version 1.0 \
         --root pkg_root \
         --install-location / \
         ABRAV_App.pkg

# Cleanup
rm -rf pkg_root

echo "Done! The new App Installer is available as ABRAV_App.pkg"
"""

with open('abrav/build_mac_app_pkg.sh', 'w') as f:
    f.write(script_content)

os.chmod('abrav/build_mac_app_pkg.sh', 0o755)

# Update Makefiles
for fn in ['Makefile', 'makefile.mac']:
    with open(fn, 'r') as f:
        content = f.read()
    
    old_target = """mac_deploy_abrav: abrav/abrav
\trm -f ABRAV_Mac.zip
\tzip -j ABRAV_Mac.zip abrav/abrav
\tcurl --ftp-create-dirs -v -T ABRAV_Mac.zip "ftp://audres_deploy%40bonkachen.com:BTNRH1982%21@bonkachen.com/downloads/ABRAV_Mac.zip\""""
    
    new_target = """mac_deploy_abrav: abrav/abrav
\tcd abrav && ./build_mac_app_pkg.sh
\trm -f ABRAV_Mac.zip
\tzip -j ABRAV_Mac.zip abrav/ABRAV_App.pkg
\tcurl --ftp-create-dirs -v -T ABRAV_Mac.zip "ftp://audres_deploy%40bonkachen.com:BTNRH1982%21@bonkachen.com/downloads/ABRAV_Mac.zip\""""

    content = content.replace(old_target, new_target)
    
    with open(fn, 'w') as f:
        f.write(content)

