#!/bin/bash
# Build script for Proton Web Server Docker image
# Supports multi-architecture builds using Docker Buildx

set -e

# Default values
VERSION="${VERSION:-0.1.0}"
REGISTRY="${REGISTRY:-}"
IMAGE_NAME="proton-web-server"
PLATFORMS="linux/amd64,linux/arm64"
PUSH="${PUSH:-false}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Functions
print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_usage() {
    cat << EOF
Usage: $0 [OPTIONS]

Build Docker image for Proton Web Server

OPTIONS:
    -v, --version VERSION     Image version tag (default: 0.1.0)
    -r, --registry REGISTRY   Docker registry prefix (e.g., docker.io/username)
    -p, --push                Push image to registry after build
    -m, --multi-arch          Build for multiple architectures (amd64, arm64)
    -h, --help                Show this help message

EXAMPLES:
    # Build local image
    $0

    # Build specific version
    $0 --version 1.0.0

    # Build and push to registry
    $0 --registry docker.io/myuser --push

    # Build multi-arch and push
    $0 --registry docker.io/myuser --multi-arch --push

ENVIRONMENT VARIABLES:
    VERSION      Image version (default: 0.1.0)
    REGISTRY     Docker registry prefix
    PUSH         Set to 'true' to push after build

EOF
}

# Parse arguments
MULTI_ARCH=false
while [[ $# -gt 0 ]]; do
    case $1 in
        -v|--version)
            VERSION="$2"
            shift 2
            ;;
        -r|--registry)
            REGISTRY="$2"
            shift 2
            ;;
        -p|--push)
            PUSH=true
            shift
            ;;
        -m|--multi-arch)
            MULTI_ARCH=true
            shift
            ;;
        -h|--help)
            print_usage
            exit 0
            ;;
        *)
            print_error "Unknown option: $1"
            print_usage
            exit 1
            ;;
    esac
done

# Build image tags
if [ -n "$REGISTRY" ]; then
    FULL_IMAGE_NAME="${REGISTRY}/${IMAGE_NAME}"
else
    FULL_IMAGE_NAME="${IMAGE_NAME}"
fi

TAG_VERSION="${FULL_IMAGE_NAME}:${VERSION}"
TAG_LATEST="${FULL_IMAGE_NAME}:latest"

# Print build configuration
print_info "Build Configuration:"
echo "  Image Name:      ${IMAGE_NAME}"
echo "  Version:         ${VERSION}"
echo "  Registry:        ${REGISTRY:-<none>}"
echo "  Full Image:      ${FULL_IMAGE_NAME}"
echo "  Multi-arch:      ${MULTI_ARCH}"
echo "  Push to Registry: ${PUSH}"
echo ""

# Check if Docker is available
if ! command -v docker &> /dev/null; then
    print_error "Docker is not installed or not in PATH"
    exit 1
fi

# Build the image
if [ "$MULTI_ARCH" = true ]; then
    print_info "Building multi-architecture image..."
    
    # Check if buildx is available
    if ! docker buildx version &> /dev/null; then
        print_error "Docker Buildx is not available"
        print_info "Install it with: docker buildx install"
        exit 1
    fi
    
    # Create builder if it doesn't exist
    if ! docker buildx inspect proton-builder &> /dev/null; then
        print_info "Creating buildx builder instance..."
        docker buildx create --name proton-builder --use
    else
        docker buildx use proton-builder
    fi
    
    # Build command
    BUILD_CMD="docker buildx build --platform ${PLATFORMS}"
    
    if [ "$PUSH" = true ]; then
        BUILD_CMD="${BUILD_CMD} --push"
    else
        BUILD_CMD="${BUILD_CMD} --load"
        print_warning "Multi-arch build without --push will only load one architecture"
    fi
    
    BUILD_CMD="${BUILD_CMD} -t ${TAG_VERSION} -t ${TAG_LATEST} ."
    
    print_info "Executing: ${BUILD_CMD}"
    eval $BUILD_CMD
    
else
    print_info "Building single-architecture image..."
    
    # Build for current platform
    docker build \
        -t "${TAG_VERSION}" \
        -t "${TAG_LATEST}" \
        .
    
    # Push if requested
    if [ "$PUSH" = true ]; then
        print_info "Pushing image to registry..."
        docker push "${TAG_VERSION}"
        docker push "${TAG_LATEST}"
    fi
fi

# Success message
print_info "Build completed successfully!"
echo ""
print_info "Image tags:"
echo "  - ${TAG_VERSION}"
echo "  - ${TAG_LATEST}"
echo ""

if [ "$PUSH" = false ]; then
    print_info "To run the image:"
    echo "  docker run -d -p 8080:8080 ${TAG_VERSION}"
    echo ""
    print_info "To push to registry:"
    echo "  docker push ${TAG_VERSION}"
    echo "  docker push ${TAG_LATEST}"
fi
