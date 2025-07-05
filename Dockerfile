# Build stage
FROM rust:1.80-alpine AS builder

# Install build dependencies
RUN apk add --no-cache musl-dev gcc

WORKDIR /app

# Copy dependency files first
COPY Cargo.toml ./

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

# Runtime stage
FROM alpine:3.20

# Install runtime dependencies
RUN apk add --no-cache ca-certificates

# Create app directory
WORKDIR /app

# Copy the binary (templates are now embedded)
COPY --from=builder /app/target/release/homepage /app/homepage

# Expose ports
EXPOSE 8080 6443 8081

ENTRYPOINT ["/app/homepage"]