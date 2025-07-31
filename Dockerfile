# Build stage
# Since this is built on an ARM host, Docker will use the ARM variant of the image.
# The default toolchain on Alpine is MUSL-based, so the resulting binary will be static.
FROM rust:1.80-alpine AS builder

# Install build dependencies.
RUN apk add --no-cache musl-dev gcc

WORKDIR /app

# Copy manifests for dependency caching
COPY Cargo.toml Cargo.lock ./

# Build only the dependencies and then remove the dummy binary to ensure a clean final build
RUN mkdir src && \
    echo 'fn main() {}' > src/main.rs && \
    cargo build --release && \
    rm -f target/release/homepage* && \
    rm -rf src

# Now, copy the actual source code, static assets and templates
COPY src ./src
COPY static ./static
COPY templates ./templates

# Build the actual application
RUN cargo build --release

# Final stage
FROM gcr.io/distroless/cc-debian12:nonroot

WORKDIR /app

# Copy static assets
COPY --from=builder /app/static ./static
COPY --from=builder /app/templates ./templates

# Copy the statically linked binary from the default release path
COPY --from=builder --chmod=755 /app/target/release/homepage .

# Expose ports
EXPOSE 8080 6443 8081

# Run the application
CMD ["/app/homepage"]
