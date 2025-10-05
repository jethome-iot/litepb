#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
FILTER_FILE="${PROJECT_ROOT}/.github_sync_filter"

PRIVATE_REPO=""
PUBLIC_REPO=""
DRY_RUN=false
AUTO_CONFIRM=false

show_help() {
    cat << EOF
GitHub Sync - Initial Setup

Performs a one-time initial sync from private Replit repository to public GitHub repository.
This script filters out Replit-specific files and pushes the code directly to the public repo's master branch.

USAGE:
    $(basename "$0") [OPTIONS]

OPTIONS:
    -h, --help              Show this help message and exit
    --dry-run               Show what would be done without making changes
    -y, --yes               Auto-confirm all prompts (skip confirmation)
    --private-repo REPO     Private repository URL (optional, auto-detected from git remote)
    --public-repo REPO      Public repository URL (optional, derived from private repo name)

AUTO-DETECTION:
    If --private-repo and --public-repo are not provided:
    1. Gets current repo URL from 'git remote get-url origin'
    2. If repo name ends with '-replit', that's the private repo
    3. Public repo = private repo name with '-replit' suffix removed

EXAMPLES:
    Auto-detect repositories (recommended):
        $(basename "$0")

    With confirmation disabled:
        $(basename "$0") --yes

    Manual override:
        $(basename "$0") --private-repo https://github.com/user/private-replit.git --public-repo https://github.com/user/private.git

    Dry run to preview:
        $(basename "$0") --dry-run

EOF
}

print_section() {
    echo ""
    echo "=== $1 ==="
    echo ""
}

extract_repo_name() {
    local url="$1"
    local repo_name
    
    if [[ "$url" =~ git@github\.com:(.+)/(.+)\.git ]]; then
        repo_name="${BASH_REMATCH[2]}"
    elif [[ "$url" =~ https://github\.com/(.+)/(.+)\.git ]]; then
        repo_name="${BASH_REMATCH[2]}"
    elif [[ "$url" =~ https://github\.com/(.+)/(.+) ]]; then
        repo_name="${BASH_REMATCH[2]}"
    else
        echo ""
        return 1
    fi
    
    echo "$repo_name"
}

extract_repo_owner() {
    local url="$1"
    local owner
    
    if [[ "$url" =~ git@github\.com:(.+)/(.+)\.git ]]; then
        owner="${BASH_REMATCH[1]}"
    elif [[ "$url" =~ https://github\.com/(.+)/(.+)\.git ]]; then
        owner="${BASH_REMATCH[1]}"
    elif [[ "$url" =~ https://github\.com/(.+)/(.+) ]]; then
        owner="${BASH_REMATCH[1]}"
    else
        echo ""
        return 1
    fi
    
    echo "$owner"
}

detect_repositories() {
    print_section "Step 1: Detecting repositories"
    
    if [ -n "$PRIVATE_REPO" ] && [ -n "$PUBLIC_REPO" ]; then
        echo "✓ Using manually specified repositories"
        echo "  Private: $PRIVATE_REPO"
        echo "  Public:  $PUBLIC_REPO"
        return 0
    fi
    
    if ! git rev-parse --git-dir &> /dev/null; then
        echo "❌ Error: Not a git repository" >&2
        echo "Please run this script from within a git repository or use --private-repo and --public-repo flags." >&2
        exit 1
    fi
    
    local origin_url
    if ! origin_url=$(git remote get-url origin 2>&1); then
        echo "❌ Error: Could not get origin remote URL" >&2
        echo "Please ensure 'git remote get-url origin' works or use --private-repo and --public-repo flags." >&2
        exit 1
    fi
    
    echo "✓ Found git remote origin: $origin_url"
    
    local repo_name
    if ! repo_name=$(extract_repo_name "$origin_url"); then
        echo "❌ Error: Could not extract repository name from URL: $origin_url" >&2
        echo "Please use --private-repo and --public-repo flags to specify repositories manually." >&2
        exit 1
    fi
    
    local owner
    if ! owner=$(extract_repo_owner "$origin_url"); then
        echo "❌ Error: Could not extract repository owner from URL: $origin_url" >&2
        echo "Please use --private-repo and --public-repo flags to specify repositories manually." >&2
        exit 1
    fi
    
    echo "✓ Detected repository: $owner/$repo_name"
    
    if [[ "$repo_name" == *-replit ]]; then
        local public_name="${repo_name%-replit}"
        echo "✓ Repository ends with '-replit', treating as private repository"
        echo "  Private repo: $repo_name"
        echo "  Public repo:  $public_name"
        
        if [[ "$origin_url" =~ ^git@ ]]; then
            PRIVATE_REPO="git@github.com:${owner}/${repo_name}.git"
            PUBLIC_REPO="git@github.com:${owner}/${public_name}.git"
        else
            PRIVATE_REPO="https://github.com/${owner}/${repo_name}.git"
            PUBLIC_REPO="https://github.com/${owner}/${public_name}.git"
        fi
    else
        echo "❌ Error: Repository name does not end with '-replit'" >&2
        echo "Expected format: 'projectname-replit'" >&2
        echo "Please use --private-repo and --public-repo flags to specify repositories manually." >&2
        exit 1
    fi
    
    echo ""
    echo "Auto-detected repositories:"
    echo "  Private: $PRIVATE_REPO"
    echo "  Public:  $PUBLIC_REPO"
}

check_prerequisites() {
    print_section "Step 2: Checking prerequisites"
    
    local missing=()
    
    if ! command -v git &> /dev/null; then
        missing+=("git")
    fi
    
    if ! command -v gh &> /dev/null; then
        missing+=("gh (GitHub CLI)")
    fi
    
    if [ ${#missing[@]} -gt 0 ]; then
        echo "❌ Error: Missing required tools: ${missing[*]}" >&2
        echo "Please install them before running this script." >&2
        exit 1
    fi
    
    echo "✓ git is installed: $(git --version | head -n1)"
    echo "✓ gh is installed: $(gh --version | head -n1)"
    
    if [ ! -f "$FILTER_FILE" ]; then
        echo "❌ Error: Filter file not found at $FILTER_FILE" >&2
        exit 1
    fi
    
    echo "✓ Filter file found: $FILTER_FILE"
    echo ""
    echo "Files/patterns that will be filtered out:"
    while IFS= read -r pattern; do
        [ -z "$pattern" ] && continue
        [[ "$pattern" =~ ^[[:space:]]*# ]] && continue
        echo "  - $pattern"
    done < "$FILTER_FILE"
}

parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            --dry-run)
                DRY_RUN=true
                shift
                ;;
            -y|--yes)
                AUTO_CONFIRM=true
                shift
                ;;
            --private-repo)
                PRIVATE_REPO="$2"
                shift 2
                ;;
            --public-repo)
                PUBLIC_REPO="$2"
                shift 2
                ;;
            *)
                echo "❌ Error: Unknown option: $1" >&2
                echo "Use --help for usage information." >&2
                exit 1
                ;;
        esac
    done
}

run_command() {
    if [ "$DRY_RUN" = true ]; then
        echo "[DRY RUN] $*"
    else
        "$@"
    fi
}

confirm_action() {
    if [ "$AUTO_CONFIRM" = true ]; then
        return 0
    fi
    
    if [ "$DRY_RUN" = true ]; then
        return 0
    fi
    
    local response
    read -r -p "Proceed? (y/n): " response
    case "$response" in
        [yY][eE][sS]|[yY])
            return 0
            ;;
        *)
            echo ""
            echo "❌ Operation cancelled by user"
            exit 0
            ;;
    esac
}

show_sync_summary() {
    print_section "Step 3: Review sync operation"
    
    local current_branch
    if git rev-parse --git-dir &> /dev/null; then
        current_branch=$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "unknown")
    else
        current_branch="N/A (will clone)"
    fi
    
    echo "Sync Direction:"
    echo "  FROM: $PRIVATE_REPO"
    echo "  TO:   $PUBLIC_REPO (branch: master)"
    echo ""
    echo "Current branch: $current_branch"
    echo ""
    echo "Operation: Initial sync (force push to master)"
    echo ""
    
    local filter_count=0
    while IFS= read -r pattern; do
        [ -z "$pattern" ] && continue
        [[ "$pattern" =~ ^[[:space:]]*# ]] && continue
        ((++filter_count))
    done < "$FILTER_FILE"
    
    echo "Files to be filtered: $filter_count patterns"
    echo ""
    
    if [ "$DRY_RUN" = true ]; then
        echo "⚠️  DRY RUN MODE - No actual changes will be made"
        echo ""
    fi
}

show_files_to_sync() {
    print_section "Files to be synced (after filtering)"
    
    if ! git rev-parse --git-dir &> /dev/null; then
        echo "⚠️  Not in a git repository - cannot preview files"
        echo ""
        return 0
    fi
    
    local all_files
    all_files=$(git ls-files 2>/dev/null || echo "")
    
    if [ -z "$all_files" ]; then
        echo "⚠️  No tracked files found"
        echo ""
        return 0
    fi
    
    local sync_files=""
    while IFS= read -r file; do
        local should_exclude=false
        
        while IFS= read -r pattern; do
            [ -z "$pattern" ] && continue
            [[ "$pattern" =~ ^[[:space:]]*# ]] && continue
            
            local clean_pattern="${pattern%/}"
            
            if [[ "$pattern" == */ ]]; then
                if [[ "$file" == "$clean_pattern"* ]]; then
                    should_exclude=true
                    break
                fi
            else
                if [[ "$file" == "$pattern" ]] || [[ "$file" == "$pattern"/* ]]; then
                    should_exclude=true
                    break
                fi
            fi
        done < "$FILTER_FILE"
        
        if [ "$should_exclude" = false ]; then
            if [ -z "$sync_files" ]; then
                sync_files="$file"
            else
                sync_files="${sync_files}"$'\n'"${file}"
            fi
        fi
    done <<< "$all_files"
    
    local file_count=0
    if [ -n "$sync_files" ]; then
        file_count=$(echo "$sync_files" | wc -l)
    fi
    
    echo "Total files to be synced: $file_count"
    echo ""
    
    if [ "$file_count" -gt 0 ]; then
        echo "Files:"
        echo "$sync_files" | sed 's/^/  /'
    fi
    
    echo ""
}

initial_sync() {
    print_section "Step 4: Performing initial sync"
    
    local temp_dir
    temp_dir=$(mktemp -d)
    trap 'rm -rf "$temp_dir"' EXIT
    
    echo "📦 Cloning private repository..."
    if [ "$DRY_RUN" = true ]; then
        echo "[DRY RUN] Cloning (will still execute to show filtered files)..."
    fi
    git clone "$PRIVATE_REPO" "$temp_dir/private"
    echo "✓ Successfully cloned private repository"
    
    echo ""
    echo "🔍 Filtering Replit-specific files..."
    cd "$temp_dir/private"
    
    local filtered_count=0
    while IFS= read -r pattern; do
        [ -z "$pattern" ] && continue
        [[ "$pattern" =~ ^[[:space:]]*# ]] && continue
        
        if [ "$DRY_RUN" = false ]; then
            if rm -rf "$pattern" 2>/dev/null; then
                echo "  ✓ Removed: $pattern"
                ((++filtered_count))
            else
                echo "  - Skipped: $pattern (not found)"
            fi
        else
            echo "  [DRY RUN] Would remove: $pattern"
            ((++filtered_count))
        fi
    done < "$FILTER_FILE"
    
    echo ""
    echo "✓ Filtered $filtered_count patterns"
    
    echo ""
    echo "🔄 Creating fresh orphan branch (removing git history)..."
    if [ "$DRY_RUN" = true ]; then
        echo "[DRY RUN] git checkout --orphan fresh-start"
    else
        if git checkout --orphan fresh-start; then
            echo "✓ Created orphan branch 'fresh-start'"
        else
            echo "❌ Error: Failed to create orphan branch" >&2
            exit 1
        fi
    fi
    
    echo ""
    echo "🧹 Clearing git index..."
    if [ "$DRY_RUN" = true ]; then
        echo "[DRY RUN] git rm -r --cached . 2>/dev/null || true"
    else
        git rm -r --cached . 2>/dev/null || true
        echo "✓ Cleared git index"
    fi
    
    echo ""
    echo "📝 Staging all filtered files..."
    if [ "$DRY_RUN" = true ]; then
        echo "[DRY RUN] git add ."
    else
        if git add .; then
            echo "✓ Staged all files"
        else
            echo "❌ Error: Failed to stage files" >&2
            exit 1
        fi
    fi
    
    echo ""
    echo "💾 Creating initial commit..."
    if [ "$DRY_RUN" = true ]; then
        echo "[DRY RUN] git commit -m \"Initial commit\""
    else
        if git commit -m "Initial commit"; then
            echo "✓ Created initial commit"
        else
            echo "❌ Error: Failed to create commit" >&2
            exit 1
        fi
    fi
    
    echo ""
    echo "🚀 Setting up public repository remote..."
    if [ "$DRY_RUN" = true ]; then
        echo "[DRY RUN] Adding remote (will still execute)..."
    fi
    git remote add public "$PUBLIC_REPO"
    echo "✓ Added public remote"
    
    echo ""
    echo "📤 Pushing to public repository master branch..."
    if [ "$DRY_RUN" = true ]; then
        echo "[DRY RUN] git push public fresh-start:master --force"
    else
        if git push public fresh-start:master --force; then
            echo "✓ Successfully pushed to public repository!"
        else
            echo "❌ Error: Failed to push to public repository" >&2
            exit 1
        fi
    fi
}

show_final_summary() {
    print_section "✨ Initial sync complete!"
    
    echo "Summary:"
    echo "  Private repo: $PRIVATE_REPO"
    echo "  Public repo:  $PUBLIC_REPO"
    echo "  Target branch: master"
    echo ""
    
    if [ "$DRY_RUN" = true ]; then
        echo "⚠️  This was a DRY RUN - no actual changes were made"
        echo "Run without --dry-run to perform the actual sync"
    else
        echo "✅ Your public repository has been initialized with filtered code"
        echo "🔗 Public repository: $PUBLIC_REPO"
    fi
    echo ""
}

main() {
    parse_args "$@"
    detect_repositories
    check_prerequisites
    show_sync_summary
    show_files_to_sync
    
    echo ""
    confirm_action
    echo ""
    
    initial_sync
    show_final_summary
}

main "$@"
