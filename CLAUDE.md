# Homepage C++ Project

## Project Overview

A modern C++ server that displays real-time system metrics (CPU usage, temperature, fan speed) via WebSocket
connections. Built with the Drogon framework and designed for high-performance, low-latency metrics streaming. Features
comprehensive Kubernetes deployment with Envoy proxy for TLS termination, compression, and load balancing.

## Architecture

### Core Framework

- **Framework**: Drogon v1.9.8 (High-performance C++ web framework)
- **Language**: C++20
- **Build System**: CMake 3.14+
- **Dependency Management**: CPM (CMake Package Manager)
- **Logging**: spdlog with structured JSON logging

### System Architecture

```
[Client] → [Envoy Proxy] → [Homepage App]
           ↓ 8443 HTTPS     ↓ 8080 HTTP
           (TLS + Compression)
```

## Project Structure

### Source Code Organization

```
├── include/homepage/
│   ├── controllers/          # HTTP & WebSocket controllers
│   │   ├── home_controller.hpp      # Serves main HTML page
│   │   └── metrics_websocket.hpp    # Real-time metrics WebSocket
│   ├── services/
│   │   └── metrics_service.hpp      # System metrics collection
│   ├── templates/
│   │   └── index_template.hpp       # HTML template rendering
│   └── utils/
│       └── config.hpp               # Environment configuration
├── src/                      # Implementation files
│   ├── controllers/
│   ├── services/
│   │   ├── metrics_service.cpp         # Base metrics service
│   │   ├── metrics_service_linux.cpp   # Linux-specific metrics
│   │   └── metrics_service_mac.cpp     # macOS-specific metrics
│   ├── utils/
│   └── main.cpp             # Application entry point
└── kubernetes/              # K8s deployment manifests
```

### Key Components

#### 1. MetricsService

- **Base class** (`metrics_service.cpp`): Common interface and JSON serialization
- **Platform-specific implementations**:
    - `metrics_service_linux.cpp`: Uses `/proc/stat`, `/sys/class/hwmon`
    - `metrics_service_mac.cpp`: Uses system APIs for macOS
- **Metrics collected**:
    - CPU usage percentage (calculated from idle/total time delta)
    - CPU temperature (when available via hardware monitoring)
    - Fan speed in RPM (supports Raspberry Pi and generic hwmon interfaces)

#### 2. MetricsWebSocket Controller

- **Custom constructor**: Requires `MetricsService` dependency injection
- **Manual registration**: Uses `WebSocketController<T, false>` to disable auto-creation
- **Real-time streaming**: Sends JSON metrics every 2 seconds
- **Connection management**: Handles new connections, disconnections, graceful shutdown
- **Threading**: Each connection runs in its own detached thread

#### 3. HomeController

- Serves the main HTML page with embedded JavaScript
- WebSocket client implementation for real-time metrics display
- Responsive web interface for viewing system metrics

## Environment Variables

### Basic Configuration

- `PORT`: Server port (default: 8080)
- `ADDRESS`: Bind address (default: [::] for dual-stack IPv4/IPv6)

### TLS Configuration (via Envoy)

- Server operates in HTTP-only mode (port 8080)
- Envoy handles TLS termination (port 8443)
- Mutual TLS configured via Kubernetes secrets:
    - `/etc/ssl/tls.crt`: Server certificate
    - `/etc/ssl/tls.key`: Server private key
    - `/etc/ssl/ca.crt`: Client CA certificate

## Deployment Architecture

### Docker Setup

- **Multi-stage build**: Alpine Linux builder + minimal runtime
- **Container size**: Optimized with stripped binaries
- **Base images**: Alpine Linux (security-focused, minimal)
- **Registry**: GitHub Container Registry (`ghcr.io/cagataygurturk/homepage:cpp`)

### Kubernetes Configuration

```yaml
# Key deployment features:
- Replicas: 2 (high availability)
- Node selector: ARM64 architecture, Berlin region
- Pod anti-affinity: Spread across different nodes
- Init container: Envoy proxy sidecar
- Volume mounts: Hardware monitoring access (/sys/devices/platform/cooling_fan/hwmon/)
- Health checks: Liveness and readiness probes on port 8080
- TLS secrets: Cloudflare certificates mounted to /etc/ssl
```

### Envoy Proxy Features

- **TLS termination**: HTTPS on port 8443 with mutual TLS
- **Compression**: Brotli (quality 6) and gzip (best compression) for web content
- **Load balancing**: Round-robin to backend services
- **Health checks**: Monitors backend health on / endpoint
- **WebSocket support**: Upgrade handling for real-time connections
- **Structured logging**: JSON access logs with request tracing
- **Admin interface**: Port 9901 for monitoring and diagnostics

### Networking

- **Load Balancer**: Cilium BGP control plane
- **IP**: 172.16.199.254
- **SSL termination**: Port 443 → 8443 (Envoy) → 8080 (App)

## Build System

### CMake Configuration

- **C++20 standard** with modern features
- **Drogon features**: Minimal build (ORM, CTL, examples disabled)
- **TLS**: Disabled in Drogon (handled by Envoy)
- **Static linking**: spdlog header-only, Drogon static
- **Optimization**: Release builds with stripped binaries

### Build Commands

```bash
make build          # Configure and build project
make clean          # Clean build artifacts
./build/homepage    # Run the application
```

## System Metrics Details

### CPU Usage Calculation

- **Method**: Delta calculation between `/proc/stat` readings
- **Formula**: `(total_delta - idle_delta) / total_delta * 100`
- **Accuracy**: Global system CPU utilization percentage
- **Update interval**: Real-time with 2-second WebSocket broadcasts

### Temperature Monitoring

- **Linux**: `/sys/class/hwmon/hwmon*/temp*_input` (millicelsius)
- **macOS**: System-specific APIs
- **Availability**: Optional (returns `null` if unavailable)

### Fan Speed Detection

- **Raspberry Pi**: `/sys/devices/platform/cooling_fan/hwmon/`
- **Generic hwmon**: Auto-detection of fan RPM sensors
- **Units**: Revolutions per minute (RPM)

## Development Guidelines

### Code Standards

- **No static methods**: Do not use static methods in class designs. Prefer instance methods for better testability, dependency injection, and object-oriented design principles.
- **C++20 features**: Modern syntax, concepts, ranges
- **Namespace organization**: `homepage::services`, `homepage::controllers`
- **RAII principles**: Smart pointers, automatic resource management
- **Error handling**: Exception-based with logging
- **Thread safety**: Detached threads for WebSocket connections
- **Code quality**: Use clang-format (Google style) and address clang-tidy warnings, suppress false positives with NOLINT comments

### Build Requirements

- Always run `make build` after CMake or dependency changes
- All dependencies managed via CPM (no system dependencies)
- No git submodules - vendor dependencies in third_party/ if needed
- Prefer static linking for deployment consistency

### Logging

- **Structured JSON**: Timestamp, level, thread ID, logger name, message
- **spdlog integration**: High-performance logging library
- **Trantor integration**: Drogon framework logging via spdlog

## Security Features

### TLS/SSL

- **Mutual TLS**: Client certificate validation
- **Certificate management**: Kubernetes secrets with Cloudflare CA
- **Protocol support**: TLS 1.2+ via Envoy
- **Cipher suites**: Modern, secure configurations

### Container Security

- **Non-root execution**: Alpine Linux security model
- **Minimal attack surface**: Stripped binaries, essential libraries only
- **Read-only filesystems**: Configuration and certificates mounted read-only
- **Network policies**: Kubernetes-level traffic control

## Performance Characteristics

### Resource Usage

- **Memory**: Minimal footprint with static linking
- **CPU**: Low overhead metrics collection
- **Network**: Efficient WebSocket streaming
- **Disk**: Stateless operation (no persistent storage)

### Scalability

- **Horizontal**: Multiple replicas with load balancing
- **Connection handling**: One thread per WebSocket connection
- **Compression**: Reduces bandwidth usage by 20-25% (Brotli)
- **Health monitoring**: Automatic failover and recovery

## Monitoring and Observability

### Health Checks

- **Liveness probe**: HTTP GET / every 30s (5s timeout, 3 failures)
- **Readiness probe**: HTTP GET / every 10s (3s timeout, 3 failures)
- **Envoy admin**: Port 9901 for proxy metrics and configuration

### Logging

- **Application logs**: JSON structured format
- **Access logs**: Envoy with request tracing
- **Error tracking**: Exception handling with stack traces
- **Performance metrics**: Request duration, response sizes

## Important Implementation Notes

### Controller Registration

- WebSocket controllers with custom constructors require manual registration
- Use `WebSocketController<T, false>` to disable auto-creation
- Register via `app().registerController(controller_instance)`

### Platform Compatibility

- **Linux**: Full hardware monitoring support
- **macOS**: Limited to CPU usage (development/testing)
- **Architecture**: Optimized for ARM64 (Raspberry Pi, Apple Silicon)

### Dependency Management

- Never assume system dependencies are available
- All dependencies fetched via CPM during build
- Version pinning for reproducible builds (Drogon v1.9.8)
- Header-only libraries preferred for static linking

# Important Instructions

Do what has been asked; nothing more, nothing less.
NEVER create files unless they're absolutely necessary for achieving your goal.
ALWAYS prefer editing an existing file to creating a new one.  
NEVER proactively create documentation files (*.md) or README files. Only create documentation files if explicitly
requested by the User.