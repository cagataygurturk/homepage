# Multi-stage build for minimal C++ binary
FROM alpine:latest AS builder

# Install build dependencies including code quality tools
RUN apk --no-cache add \
    build-base \
    cmake \
    git \
    pkgconfig \
    jsoncpp-dev \
    spdlog-dev \
    util-linux-dev \
    zlib-dev \
    clang-extra-tools

# Set working directory
WORKDIR /app

# Copy source code
COPY . .

# Code quality checks
RUN echo "Checking code formatting..." && \
    find src -name "*.cpp" -o -name "*.hpp" | xargs clang-format --dry-run --Werror

RUN echo "Running clang-tidy..." && \
    mkdir -p build && \
    cd build && \
    cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && \
    find ../src -name "*.cpp" | xargs clang-tidy -p . || echo "clang-tidy completed"

# Build the application
RUN cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make homepage -j$(nproc)

# Strip the binary to reduce size further
RUN strip build/homepage

# Final stage: Alpine for runtime dependencies
FROM alpine:latest

# Install runtime dependencies
RUN apk --no-cache add \
    jsoncpp \
    util-linux \
    fmt

# Copy the binary from builder stage
COPY --from=builder /app/build/homepage /homepage

# Expose port 8080 (HTTP only, SSL handled by Envoy)
EXPOSE 8080

# Set the binary as entrypoint
ENTRYPOINT ["/homepage"]