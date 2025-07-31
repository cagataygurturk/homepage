# Build stage
FROM rust:1.80-alpine AS builder

# Install build dependencies (musl target already available for current arch)
RUN apk add --no-cache musl-dev gcc

WORKDIR /app

# Copy dependency files first
COPY Cargo.toml Cargo.lock ./

# Create a dummy src/main.rs to build dependencies
RUN mkdir src && echo "fn main() {}" > src/main.rs

# Build dependencies (this layer will be cached)
RUN cargo build --release

# Remove dummy files
RUN rm -rf src

# Copy actual source code and templates
COPY src ./src
COPY templates ./templates

# Build the actual application (only this layer rebuilds when code changes)
RUN cargo build --release

# Runtime stage - using distroless
FROM gcr.io/distroless/static-debian12:nonroot

# Copy the statically linked binary
COPY --from=builder /app/target/release/homepage /homepage

# Expose ports
EXPOSE 8080 6443 8081

# Use exec form and ensure proper signal handling
ENTRYPOINT ["/homepage"]