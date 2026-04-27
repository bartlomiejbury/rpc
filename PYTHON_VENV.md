# Python Virtual Environment Setup

This project uses a Python virtual environment to manage dependencies and ensure consistency across different systems.

## Quick Start

```bash
# Create virtual environment
python3 -m venv venv

# Activate it
source venv/bin/activate  # On Windows: venv\Scripts\activate

# Install dependencies
pip install -r requirements.txt
```

## Usage

### Running Python Server
```bash
# Make sure venv is activated
source venv/bin/activate

cd PythonServer
python rpc_server.py
```

### Running Python Client
```bash
# Make sure venv is activated
source venv/bin/activate

cd PythonClient
python rpc_client.py
```

### Running Tests

The test script automatically uses the virtual environment if it exists:

```bash
./test_all_combinations.sh
```

## Deactivating

When you're done:
```bash
deactivate
```

## Dependencies

See [requirements.txt](requirements.txt) for the list of Python packages installed:
- `pyzmq` - ZeroMQ Python bindings

## Why Virtual Environment?

- **Isolation**: Dependencies don't interfere with system Python packages
- **Reproducibility**: Everyone uses the same package versions
- **Clean**: Easy to remove (just delete `venv/` directory)
- **Standard**: Best practice for Python projects

## Troubleshooting

### venv not found
If `python3 -m venv` doesn't work, install it:
```bash
# Ubuntu/Debian
sudo apt-get install python3-venv

# macOS (usually included)
# Windows (usually included with Python installer)
```

### Permission errors
If you get permission errors when installing packages system-wide, use venv (recommended) instead of `sudo pip`.

### Different Python version
To use a specific Python version:
```bash
python3.9 -m venv venv  # Replace with your version
```
