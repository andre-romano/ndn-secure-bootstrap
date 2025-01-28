#!/bin/bash

# Update the package list and upgrade installed packages
echo "Updating package lists and upgrading installed packages..."
sudo apt-get update -y

# Install Node.js and NPM
echo "Installing Node.js and NPM..."
sudo apt-get install -y nodejs npm

# Install Python and PIP
echo "Installing Python3 and PIP..."
sudo apt-get install -y python3 python3-pip

# Install GNUPlot
echo "Installing GNUPlot..."
sudo apt-get install -y gnuplot

# Install NPM dependencies (defined in package.json)
if [ -f "package.json" ]; then
  echo "Installing NPM dependencies..."
  npm install
else
  echo "No package.json file found. Skipping NPM dependency installation."
fi

# Install Python dependencies (defined in requirements.txt)
if [ -f "requirements.txt" ]; then
  echo "Installing Python dependencies..."
  pip3 install -r requirements.txt 
else
  echo "No requirements.txt file found. Skipping Python dependency installation."
fi

echo "Installation complete!"
