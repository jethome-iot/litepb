#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
FILTER_FILE="${PROJECT_ROOT}/.github_sync_filter"

PRIVATE_REPO=""
PUBLIC_REPO=""
PRIVATE_BRANCH="dev"
PUBLIC_BASE_BRANCH="master"
DRY_RUN=false
AUTO_CONFIRM=false

show_help() {
    cat << EOF
GitHub Sync - Ongoing Sync to Public Repository

Syncs changes from private Replit repository to public GitHub repository.
This script pulls from the private dev branch, filters Replit-specific files,
creates a new branch in the public repo, and opens a PR to master.

USAGE:
    $(basename "$0") [OPTIONS]

OPTIONS:
    -h, --help              Show this help message and exit
    --dry-run               Show what would be done without making changes
    -y, --yes               Auto-confirm all prompts (skip confirmation)
    --private-repo REPO     Private repository URL (optional, auto-detected from git remote)
    --public-repo REPO      Public repository URL (optional, derived from private repo name)
    --private-branch BRANCH Branch to pull from private repo (default: dev)
    --public-branch BRANCH  Base branch in public repo (default: master)

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

    Sync with custom branches:
        $(basename "$0") --private-branch feature-x --public-branch main

    Manual override:
        $(basename "$0") --private-repo https://github.com/user/private-replit.git --public-repo https://github.com/user/private.git

    Dry run to preview:
        $(basename "$0") --dry-run

WORKFLOW:
    1. Auto-detect or use specified repositories
    2. Clone private repository
    3. Pull latest changes from specified private branch
    4. Filter out Replit-specific files
    5. Create new branch in public repository
    6. Push changes and create Pull Request
    7. Output PR URL for review

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
    
    if ! gh auth status &> /dev/null; then
        echo "❌ Error: GitHub CLI is not authenticated" >&2
        echo "Please run 'gh auth login' first." >&2
        exit 1
    fi
    
    echo "✓ GitHub CLI is authenticated"
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
            --private-branch)
                PRIVATE_BRANCH="$2"
                shift 2
                ;;
            --public-branch)
                PUBLIC_BASE_BRANCH="$2"
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
    echo "  FROM: $PRIVATE_REPO (branch: $PRIVATE_BRANCH)"
    echo "  TO:   $PUBLIC_REPO (base branch: $PUBLIC_BASE_BRANCH)"
    echo ""
    echo "Current branch: $current_branch"
    echo ""
    echo "Operation: Create PR with filtered changes"
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

sync_to_public() {
    print_section "Step 4: Performing sync to public repository"
    
    local temp_dir
    temp_dir=$(mktemp -d)
    trap "rm -rf '${temp_dir}'" EXIT
    
    local timestamp
    timestamp=$(date +%Y%m%d-%H%M%S)
    local sync_branch="sync-from-private-${timestamp}"
    
    echo "📦 Cloning public repository..."
    if [ "$DRY_RUN" = true ]; then
        echo "[DRY RUN] Cloning (will still execute to show structure)..."
    fi
    git clone "$PUBLIC_REPO" "$temp_dir/public"
    echo "✓ Successfully cloned public repository"
    
    cd "$temp_dir/public"
    
    echo ""
    echo "📥 Checking out $PUBLIC_BASE_BRANCH branch..."
    if [ "$DRY_RUN" = true ]; then
        echo "[DRY RUN] Checking out (will still execute)..."
    fi
    git checkout "$PUBLIC_BASE_BRANCH"
    echo "✓ Checked out $PUBLIC_BASE_BRANCH"
    
    echo ""
    echo "🌿 Creating sync branch: $sync_branch"
    if [ "$DRY_RUN" = true ]; then
        echo "[DRY RUN] Would create branch $sync_branch (will still execute)..."
    fi
    git checkout -b "$sync_branch"
    echo "✓ Created sync branch (based on $PUBLIC_BASE_BRANCH with shared history)"
    
    echo ""
    echo "📦 Cloning private repository..."
    if [ "$DRY_RUN" = true ]; then
        echo "[DRY RUN] Cloning (will still execute to show filtered files)..."
    fi
    git clone "$PRIVATE_REPO" "$temp_dir/private"
    echo "✓ Successfully cloned private repository"
    
    cd "$temp_dir/private"
    
    echo ""
    echo "📥 Pulling latest changes from $PRIVATE_BRANCH branch..."
    if [ "$DRY_RUN" = true ]; then
        echo "[DRY RUN] Checking out and pulling (will still execute)..."
    fi
    git checkout "$PRIVATE_BRANCH"
    git pull origin "$PRIVATE_BRANCH"
    echo "✓ Checked out and pulled $PRIVATE_BRANCH"
    
    echo ""
    echo "🔍 Filtering Replit-specific files from private repo..."
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
    echo "📋 Copying filtered files to public repository..."
    cd "$temp_dir/public"
    
    if [ "$DRY_RUN" = true ]; then
        echo "[DRY RUN] Would copy files from private to public repo (excluding .git)"
    else
        # Remove all files except .git
        find "$temp_dir/public" -mindepth 1 -maxdepth 1 ! -name '.git' -exec rm -rf {} +
        
        # Copy all files from private except .git
        find "$temp_dir/private" -mindepth 1 -maxdepth 1 ! -name '.git' -exec cp -r {} "$temp_dir/public/" \;
        
        echo "✓ Copied filtered files to public repository"
    fi
    
    echo ""
    echo "🔍 Checking for changes in public repository..."
    if [ "$DRY_RUN" = false ]; then
        git add -A
        if git diff --staged --quiet; then
            echo ""
            echo "ℹ️  No changes to sync after filtering"
            return 0
        fi
        git commit -m "Sync from private repo (filtered for public release)"
        echo "✓ Created commit with filtered changes"
    else
        echo "[DRY RUN] Would stage and commit changes"
    fi
    
    echo ""
    echo "📤 Pushing to public repository..."
    if [ "$DRY_RUN" = true ]; then
        echo "[DRY RUN] git push origin $sync_branch"
    else
        if git push origin "$sync_branch"; then
            echo "✓ Successfully pushed to public repository!"
        else
            echo "❌ Error: Failed to push to public repository" >&2
            exit 1
        fi
    fi
    
    echo ""
    echo "🔀 Creating Pull Request..."
    if [ "$DRY_RUN" = true ]; then
        echo "[DRY RUN] gh pr create --repo <public-repo> --base $PUBLIC_BASE_BRANCH --head $sync_branch"
        echo ""
        echo "📋 PR would be created with:"
        echo "   Title: Sync from private repository"
        echo "   Body:  Automated sync from private Replit repository"
        echo "   Base:  $PUBLIC_BASE_BRANCH"
        echo "   Head:  $sync_branch"
    else
        local public_repo_name
        public_repo_name=$(basename "$PUBLIC_REPO" .git)
        local public_owner
        public_owner=$(extract_repo_owner "$PUBLIC_REPO")
        
        if pr_url=$(gh pr create \
            --repo "${public_owner}/${public_repo_name}" \
            --base "$PUBLIC_BASE_BRANCH" \
            --head "$sync_branch" \
            --title "Sync from private repository" \
            --body "Automated sync from private Replit repository (filtered for public release)" 2>&1); then
            echo "✓ Pull Request created successfully!"
            echo ""
            echo "🔗 PR URL: $pr_url"
        else
            echo "❌ Error: Failed to create Pull Request" >&2
            echo "$pr_url" >&2
            exit 1
        fi
    fi
}

show_final_summary() {
    print_section "✨ Sync complete!"
    
    echo "Summary:"
    echo "  Private repo: $PRIVATE_REPO (branch: $PRIVATE_BRANCH)"
    echo "  Public repo:  $PUBLIC_REPO (base: $PUBLIC_BASE_BRANCH)"
    echo ""
    
    if [ "$DRY_RUN" = true ]; then
        echo "⚠️  This was a DRY RUN - no actual changes were made"
        echo "Run without --dry-run to perform the actual sync"
    else
        echo "✅ Changes have been synced and a PR has been created"
        echo "📝 Review and merge the PR to complete the sync"
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
    
    sync_to_public
    show_final_summary
}

main "$@"
