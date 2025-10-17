#!/bin/bash
######################################################################
set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/lib/common.sh"
######################################################################

# Change to project root directory
cd "$(dirname "$0")/.." || exit 1

# Files containing version information
CMAKE_FILE="cmake/CMakeLists.txt"
PYTHON_FILE="generator/__init__.py"
DOCKER_FILE="docker/Dockerfile"

# Function to extract current version from CMakeLists.txt
get_current_version() {
    grep 'VERSION' "$CMAKE_FILE" | grep -E '[0-9]+\.[0-9]+\.[0-9]+' | sed -E 's/.*VERSION ([0-9]+\.[0-9]+\.[0-9]+).*/\1/' | head -1
}

# Function to validate version format (semantic versioning: X.Y.Z)
validate_version() {
    if [[ ! "$1" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
        echo "❌ Error: Invalid version format '$1'. Must be X.Y.Z (e.g., 1.0.1)"
        exit 1
    fi
}

# Function to update version in all files (portable sed replacement)
update_version_files() {
    local old_version=$1
    local new_version=$2
    
    echo "Updating version files..."
    
    # Update CMakeLists.txt (portable sed with temp file)
    sed "s/VERSION ${old_version}/VERSION ${new_version}/" "$CMAKE_FILE" > "${CMAKE_FILE}.tmp" && mv "${CMAKE_FILE}.tmp" "$CMAKE_FILE"
    echo "  ✓ Updated $CMAKE_FILE"
    
    # Update Python __init__.py
    sed "s/__version__ = '${old_version}'/__version__ = '${new_version}'/" "$PYTHON_FILE" > "${PYTHON_FILE}.tmp" && mv "${PYTHON_FILE}.tmp" "$PYTHON_FILE"
    echo "  ✓ Updated $PYTHON_FILE"
    
    # Update Dockerfile
    sed "s/org.opencontainers.image.version=\"${old_version}\"/org.opencontainers.image.version=\"${new_version}\"/" "$DOCKER_FILE" > "${DOCKER_FILE}.tmp" && mv "${DOCKER_FILE}.tmp" "$DOCKER_FILE"
    echo "  ✓ Updated $DOCKER_FILE"
}

# Function to commit version changes
commit_version_changes() {
    local version=$1
    
    echo ""
    echo "Committing version changes..."
    
    # Stage the modified files
    git add "$CMAKE_FILE" "$PYTHON_FILE" "$DOCKER_FILE"
    
    # Check if there are changes to commit
    if git diff --cached --quiet; then
        echo "⚠️  Warning: No changes to commit (files may already be at version ${version})"
        return 0
    fi
    
    # Create commit
    git commit -m "Bump version to ${version}"
    echo "  ✓ Created commit: Bump version to ${version}"
}

# Function to create git tag
create_git_tag() {
    local version=$1
    local tag="v${version}"
    
    echo ""
    echo "Creating git tag ${tag}..."
    
    if git rev-parse "$tag" >/dev/null 2>&1; then
        echo "⚠️  Warning: Tag $tag already exists. Skipping tag creation."
    else
        git tag -a "$tag" -m "Release version ${version}"
        echo "  ✓ Created tag: $tag"
    fi
}

# Main script
echo "LitePB Version Update Script"
echo "============================"
echo ""

# Get current version
current_version=$(get_current_version)
echo "Current version: $current_version"
echo ""

# Prompt for new version
read -p "Enter new version (e.g., 1.0.1): " new_version

# Validate the new version
validate_version "$new_version"

# Check if version is actually changing
if [ "$new_version" = "$current_version" ]; then
    echo "❌ Error: New version ($new_version) is the same as current version ($current_version)"
    echo "Please specify a different version."
    exit 1
fi

# Confirm the update
echo ""
echo "This will update the version from $current_version to $new_version in:"
echo "  - $CMAKE_FILE"
echo "  - $PYTHON_FILE"
echo "  - $DOCKER_FILE"
echo ""
read -p "Continue? (y/N): " confirm

if [[ ! "$confirm" =~ ^[Yy]$ ]]; then
    echo "❌ Update cancelled."
    exit 0
fi

echo ""

# Update version in all files
update_version_files "$current_version" "$new_version"

# Commit the changes
commit_version_changes "$new_version"

# Create git tag
create_git_tag "$new_version"

echo ""
echo "✅ Version successfully updated to $new_version!"
echo ""
echo "Next steps:"
echo "  1. Review the changes: git show"
echo "  2. Push the commit: git push"
echo "  3. Push the tag: git push origin v${new_version}"
