# Homepage C++ Project

## Project Overview

A modern C++ server that displays real-time system metrics (CPU usage, temperature, fan speed) via WebSocket
connections. Built with the Drogon framework and designed for high-performance, low-latency metrics streaming. Deployed
on Kubernetes behind ingress-nginx, which terminates TLS, verifies Cloudflare's origin-pull client certificate and
compresses responses.

## Architecture

### Core Framework

- **Framework**: Drogon v1.9.8 (High-performance C++ web framework)
- **Language**: C++20
- **Build System**: CMake 3.14+
- **Dependency Management**: CPM (CMake Package Manager)
- **Logging**: spdlog with structured JSON logging

### System Architecture

```
[Client] → [Cloudflare] → [ingress-nginx] → [Homepage App]
            443 HTTPS      443 HTTPS         8080 HTTP
            (WAF, cache)   (TLS, mTLS, gzip)
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
├── views/
│   └── index.csp            # Drogon CSP view for the page, compiled by drogon_ctl at build time
└── kubernetes/              # K8s deployment manifests
```

### Key Components

#### 1. MetricsService

- **Base class** (`metrics_service.cpp`): Single sampler thread (2 s interval) publishing a mutex-protected snapshot via `latest()`, plus JSON serialization
- **Platform-specific implementations**:
    - `metrics_service_linux.cpp`: Uses `/proc/stat` for CPU usage and libsensors (lm-sensors) for temperature and fan speed
    - `metrics_service_mac.cpp`: Uses system APIs for macOS
- **Metrics collected**:
    - CPU usage percentage (calculated from idle/total time delta)
    - CPU temperature (when available via hardware monitoring)
    - Fan speed in RPM (supports Raspberry Pi and generic hwmon interfaces)

#### 2. MetricsWebSocket Controller

- **Custom constructor**: Requires `MetricsService` dependency injection
- **Manual registration**: Uses `WebSocketController<T, false>` to disable auto-creation
- **Real-time streaming**: Subscribes to `MetricsService` and broadcasts one JSON message to every open connection after each sample (every 2 seconds); new connections get the latest snapshot immediately
- **Connection management**: Keeps a mutex-protected set of open connections; no thread per connection

#### 3. HomeController

- Renders the `home` CSP view with the latest metrics snapshot (node name, CPU, temperature, fan) so the page
  shows real values before the WebSocket connects; values are HTML-escaped via `HttpViewData::htmlTranslate`
- Takes `MetricsService` by reference and is registered manually like the WebSocket controller
- WebSocket client implementation for real-time metrics display
- Responsive web interface for viewing system metrics

## Environment Variables

### Basic Configuration

- `PORT`: Server port (default: 8080)
- `ADDRESS`: Bind address (default: [::] for dual-stack IPv4/IPv6)

### TLS Configuration (via ingress-nginx)

- Server operates in HTTP-only mode (port 8080)
- The Ingress (`kubernetes/homepage/ingress.yaml`, class `nginx`, host `cagataygurturk.com`) terminates TLS with the
  `cloudflare-tls` secret (Cloudflare origin certificate and key)
- Client certificate verification via the `auth-tls-*` annotations against the CA in the same secret, so only
  Cloudflare can reach the origin
- Secret sources live in `kubernetes/homepage/cloudflare-tls/`

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
- Volume mounts: Hardware monitoring access (/sys/devices/platform/cooling_fan/hwmon/)
- Health checks: Liveness and readiness probes on port 8080
- Service: ClusterIP port 80 → container port 8080
- Ingress: nginx class, TLS + client certificate verification with the cloudflare-tls secret
```

### ingress-nginx

- **TLS termination**: HTTPS on port 443 with client certificate verification (Cloudflare origin pull)
- **Compression**: gzip level 6 for text content, set cluster-wide in the `ingress-nginx-controller` ConfigMap
  (`use-gzip`, `gzip-level`, `gzip-min-length`); the ConfigMap is Helm-managed, so keep the values in the chart values too
- **Load balancing**: Round-robin across the two homepage pods
- **WebSocket support**: Upgrade handling for real-time connections
- **Deployment**: Helm chart `ingress-nginx` in namespace `ingress-nginx`, three controller replicas

### Networking

- **Load Balancer**: Cilium BGP control plane
- **IP**: 172.16.199.254 (the ingress-nginx controller's LoadBalancer Service)
- **Path**: Cloudflare → 443 (ingress-nginx) → Service 80 → 8080 (App)

## Build System

### CMake Configuration

- **C++20 standard** with modern features
- **Drogon features**: Minimal build (ORM and examples disabled); `drogon_ctl` is built because it compiles the
  CSP views (`drogon_create_views` in CMakeLists.txt generates `build/views/home.{h,cc}`)
- **TLS**: Disabled in Drogon (handled by ingress-nginx)
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

- **Linux**: libsensors enumerates hwmon chips; CPU chips are preferred by name (`cpu_thermal` on Raspberry Pi,
  `k10temp`, `coretemp`, `zenpower` on x86), then any chip whose name contains `cpu` or `soc`
- **macOS**: System-specific APIs
- **Availability**: Optional (returns `null` if unavailable)

### Fan Speed Detection

- **Linux**: libsensors, first fan input reporting a non-zero RPM (`pwmfan` on Raspberry Pi 5)
- **Units**: Revolutions per minute (RPM)

## Development Guidelines

### Code Standards

- **No static methods**: Do not use static methods in class designs. Prefer instance methods for better testability, dependency injection, and object-oriented design principles.
- **C++20 features**: Modern syntax, concepts, ranges
- **Namespace organization**: `homepage::services`, `homepage::controllers`
- **RAII principles**: Smart pointers, automatic resource management
- **Error handling**: Exception-based with logging
- **Thread safety**: Metrics are sampled and broadcast from one thread; Drogon's `WebSocketConnection::send` is safe to call from any thread
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
- **Protocol support**: TLS 1.2+ via ingress-nginx
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
- **Connection handling**: Event-driven; one sampler thread serves all WebSocket connections
- **Compression**: gzip at ingress-nginx between Cloudflare and the origin
- **Health monitoring**: Automatic failover and recovery

## Monitoring and Observability

### Health Checks

- **Liveness probe**: HTTP GET / every 30s (5s timeout, 3 failures)
- **Readiness probe**: HTTP GET / every 10s (3s timeout, 3 failures)

### Logging

- **Application logs**: JSON structured format
- **Access logs**: ingress-nginx controller logs
- **Error tracking**: Exception handling with stack traces
- **Performance metrics**: Request duration, response sizes

## Important Implementation Notes

### Controller Registration

- Controllers with custom constructors require manual registration
- Use `HttpController<T, false>` / `WebSocketController<T, false>` to disable auto-creation
- Register via `app().registerController(controller_instance)`

### Platform Compatibility

- **Linux**: Full hardware monitoring support
- **macOS**: Limited to CPU usage (development/testing)
- **Architecture**: Optimized for ARM64 (Raspberry Pi, Apple Silicon)

### Dependency Management

- Never assume system dependencies are available
- All dependencies fetched via CPM during build, except libsensors which comes from the OS package
  (`lm-sensors-dev` at build time, `lm-sensors-libs` at runtime on Alpine)
- Version pinning for reproducible builds (Drogon v1.9.8)
- Header-only libraries preferred for static linking

# Important Instructions

Do what has been asked; nothing more, nothing less.
NEVER create files unless they're absolutely necessary for achieving your goal.
ALWAYS prefer editing an existing file to creating a new one.  
NEVER proactively create documentation files (*.md) or README files. Only create documentation files if explicitly
requested by the User.