# Homepage Rust Project

## Project Overview
A Rust-based homepage server that displays system metrics (CPU usage, temperature, fan speed) via WebSocket connections. Supports both HTTP and HTTPS with optional mutual TLS authentication.

## Architecture
- **Framework**: Axum web framework with WebSocket support
- **Metrics**: System information via sysinfo crate
- **TLS**: rustls for HTTPS and mutual TLS
- **Deployment**: Kubernetes with dual-stack IPv4/IPv6 support

## Development Commands

### Build & Run
```bash
cargo build --release
cargo run
```

### Testing
```bash
cargo check
cargo test
```

### Linting
```bash
cargo fmt
cargo clippy
```

## Environment Variables

### Protocol Configuration
- `HTTPS_ONLY`: Set to "true" for HTTPS-only mode (default: false for HTTP)

### TLS Configuration (required when HTTPS_ONLY=true)
- `TLS_CERT_FILE`: Path to server certificate file
- `TLS_KEY_FILE`: Path to server private key file
- `CLIENT_CA_CERT`: Path to client CA certificate (optional, enables mutual TLS)

## Server Behavior

### HTTP Mode (default)
- Listens on `[::]:8080` (dual-stack IPv4/IPv6)
- No TLS encryption
- Suitable for development

### HTTPS Mode
- Listens on `[::]:6443` (dual-stack IPv4/IPv6)
- TLS encryption with server certificates
- Optional mutual TLS if `CLIENT_CA_CERT` is provided
- Suitable for production

## Endpoints
- `GET /`: Serves the homepage HTML
- `GET /ws`: WebSocket endpoint for real-time system metrics


## Kubernetes Deployment
- Located in `kubernetes/homepage/deployment.yaml`
- Configured for HTTPS with mutual TLS in production
- Uses secrets for TLS certificates
- Includes hardware monitoring volume mounts

## System Metrics
- **CPU Usage**: Global CPU utilization percentage
- **CPU Temperature**: Hardware temperature (when available)
- **Fan Speed**: System fan RPM (supports Raspberry Pi and generic hwmon)

## Development Notes
- Uses dual-stack networking (`[::]:port`) for automatic IPv4/IPv6 support
- TLS certificates must be in PEM format
- Hardware monitoring paths are auto-detected (Raspberry Pi and generic hwmon)
- WebSocket updates every 2 seconds