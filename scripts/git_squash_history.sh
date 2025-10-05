#!/bin/bash

set -e

CURRENT_BRANCH=$(git branch --show-current)
BACKUP_BRANCH="backup-history-$(date +%Y%m%d-%H%M%S)"
COMMIT_MESSAGE="Initial commit"

echo "=========================================="
echo "Git History Squash Script"
echo "=========================================="
echo ""
echo "Current branch: $CURRENT_BRANCH"
echo "Backup branch: $BACKUP_BRANCH"
echo ""

read -p "This will squash all commits into one. Continue? (y/N): " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "Aborted."
    exit 1
fi

echo ""
echo "Step 1: Creating backup branch..."
git branch "$BACKUP_BRANCH"
echo "✓ Backup created: $BACKUP_BRANCH"

echo ""
echo "Step 2: Creating new orphan branch..."
git checkout --orphan new-squashed-branch

echo ""
echo "Step 3: Adding all files..."
git add -A

echo ""
echo "Step 4: Creating single initial commit..."
git commit -m "$COMMIT_MESSAGE"

echo ""
echo "Step 5: Replacing old branch..."
git branch -D "$CURRENT_BRANCH"
git branch -m "$CURRENT_BRANCH"

echo ""
echo "=========================================="
echo "✓ Done! Git history squashed successfully"
echo "=========================================="
echo ""
echo "Current state:"
git log --oneline
echo ""
echo "Your old history is safely stored in: $BACKUP_BRANCH"
echo "To restore it: git checkout $BACKUP_BRANCH"
echo "To delete backup: git branch -D $BACKUP_BRANCH"
echo ""
