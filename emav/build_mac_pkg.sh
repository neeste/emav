#!/bin/bash

# Create a temporary staging directory for the package root
echo "Setting up package payload..."
mkdir -p pkg_root/usr/local/bin
mkdir -p pkg_root/usr/local/etc

# Copy the compiled executable and config into the payload
cp emav pkg_root/usr/local/bin/
cp emav.ini pkg_root/usr/local/etc/

# Ensure permissions are correct
chmod 755 pkg_root/usr/local/bin/emav
chmod 644 pkg_root/usr/local/etc/emav.ini

# Build the .pkg installer using Apple's built-in pkgbuild
echo "Building EMAV.pkg..."
pkgbuild --identifier org.btnrh.emav \
         --version 3.37 \
         --root pkg_root \
         --install-location / \
         EMAV.pkg

# Cleanup the temporary directory
rm -rf pkg_root

echo "Done! The installer is available as EMAV.pkg"
