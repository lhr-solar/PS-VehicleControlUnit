#!/usr/bin/env bash

set -e

echo "Installing Arduino CLI and ESP32 support..."

OS="$(uname)"

install_arduino_cli_mac() {
    if ! command -v brew >/dev/null 2>&1; then
        echo "Homebrew not found. Please install Homebrew first:"
        echo "https://brew.sh"
        exit 1
    fi

    echo "Installing arduino-cli via Homebrew..."
    brew install arduino-cli
}

install_arduino_cli_linux() {
    if command -v arduino-cli >/dev/null 2>&1; then
        echo "arduino-cli already installed"
        return
    fi

    echo "Installing arduino-cli via official install script..."
    curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh

    sudo mv bin/arduino-cli /usr/local/bin/
}

setup_esp32() {
    echo "Updating Arduino core index..."
    arduino-cli core update-index

    echo "Installing ESP32 core..."
    arduino-cli core install esp32:esp32
}

case "$OS" in
    Darwin)
        install_arduino_cli_mac
        ;;
    Linux)
        install_arduino_cli_linux
        ;;
    *)
        echo "Unsupported OS: $OS"
        exit 1
        ;;
esac

setup_esp32

echo ""
echo "Installation complete."
echo "You can verify with:"
echo "  arduino-cli board list"