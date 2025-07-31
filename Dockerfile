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

# Build the actual application with static linking (only this layer rebuilds when code changes)
RUN cargo build --release

FROM gcr.io/distroless/cc-debian12:nonroot

# Copy the statically linked binary with executable permissions
COPY --from=builder --chmod=755 /app/target/release/homepage /homepage

# Expose ports
EXPOSE 8080 6443 8081

# Use CMD instead of ENTRYPOINT for easier override
CMD ["/homepage"]