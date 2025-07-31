# Build stage
FROM rust:1.80-alpine AS builder

# Install build dependencies
RUN apk add --no-cache musl-dev gcc

WORKDIR /app

# Copy all files
COPY . .

# Build the application
RUN cargo build --release

# Final stage
FROM gcr.io/distroless/cc-debian12:nonroot

# Copy the statically linked binary
COPY --from=builder --chmod=755 /app/target/release/homepage /homepage

# Expose ports
EXPOSE 8080 6443 8081

# Run the application
CMD ["/homepage"]
