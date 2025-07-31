# Build stage
FROM rust:1.80-alpine AS builder

# Install build dependencies and MUSL target for ARM64
RUN apk add --no-cache musl-dev gcc && \
    rustup target add aarch64-unknown-linux-musl

WORKDIR /app

# Copy manifests for dependency caching
COPY Cargo.toml Cargo.lock ./

# Build only the dependencies and then remove the dummy binary to ensure a clean final build
RUN mkdir src && \
    echo 'fn main() {}' > src/main.rs && \
    cargo build --release --target aarch64-unknown-linux-musl && \
    rm -f target/aarch64-unknown-linux-musl/release/homepage* && \
    rm -rf src

# Now, copy the actual source code and templates
COPY src ./src
COPY templates ./templates

# Build the actual application
RUN cargo build --release --target aarch64-unknown-linux-musl

# Final stage
FROM gcr.io/distroless/cc-debian12:nonroot

WORKDIR /app

# Copy templates
COPY --from=builder /app/templates ./templates

# Copy the statically linked binary
COPY --from=builder --chmod=755 /app/target/aarch64-unknown-linux-musl/release/homepage .

# Expose ports
EXPOSE 8080 6443 8081

# Run the application
CMD ["/app/homepage"]
