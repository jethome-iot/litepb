#!/bin/bash

################################################################################
# GitHub Actions Workflow Testing Script for Replit Environment
################################################################################
#
# Purpose:
#   This script provides convenient commands for analyzing GitHub Actions 
#   workflows using the 'act' tool. While actual workflow execution is not 
#   possible in the Replit environment (due to lack of Docker support), this 
#   script enables valuable workflow analysis, validation, and visualization.
#
# Limitations in Replit:
#   - Cannot execute workflows (no Docker support)
#   - Cannot run actual jobs or steps
#   - Focus is on static analysis and validation only
#
# Benefits Despite Limitations:
#   - Validate workflow syntax before pushing to GitHub
#   - Analyze workflow structure and dependencies
#   - List and inspect jobs and triggers
#   - Understand workflow configuration without running it
#
# Requirements:
#   - act must be installed (https://github.com/nektos/act)
#   - GitHub Actions workflows in .github/workflows/
#
################################################################################

set -euo pipefail

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
BOLD='\033[1m'
RESET='\033[0m'

# Default workflow directory
WORKFLOW_DIR=".github/workflows"

# Script name for usage
SCRIPT_NAME=$(basename "$0")

# Check if act is installed
check_act_installed() {
    if ! command -v act &> /dev/null; then
        echo -e "${RED}Error: 'act' is not installed${RESET}"
        echo -e "${YELLOW}Please install act first: https://github.com/nektos/act${RESET}"
        echo -e "${CYAN}Installation options:${RESET}"
        echo "  - curl -s https://raw.githubusercontent.com/nektos/act/master/install.sh | sudo bash"
        echo "  - brew install act (macOS)"
        echo "  - scoop install act (Windows)"
        exit 1
    fi
}

# Check if workflow directory exists
check_workflow_dir() {
    if [[ ! -d "$WORKFLOW_DIR" ]]; then
        echo -e "${RED}Error: Workflow directory '$WORKFLOW_DIR' not found${RESET}"
        echo -e "${YELLOW}Please ensure you're in the root of a GitHub repository${RESET}"
        exit 1
    fi
}

# Print section header
print_header() {
    local title="$1"
    local length=${#title}
    local border=$(printf '%*s' "$((length + 4))" | tr ' ' '=')
    
    echo
    echo -e "${CYAN}${border}${RESET}"
    echo -e "${CYAN}= ${BOLD}${title}${RESET}${CYAN} =${RESET}"
    echo -e "${CYAN}${border}${RESET}"
    echo
}

# Print info message
print_info() {
    echo -e "${BLUE}ℹ ${RESET} $1"
}

# Print success message
print_success() {
    echo -e "${GREEN}✓${RESET} $1"
}

# Print warning message
print_warning() {
    echo -e "${YELLOW}⚠${RESET} $1"
}

# Print error message
print_error() {
    echo -e "${RED}✗${RESET} $1"
}

# List all workflows and jobs
cmd_list() {
    print_header "Listing All Workflows and Jobs"
    
    check_workflow_dir
    check_act_installed
    
    print_info "Analyzing workflows in $WORKFLOW_DIR..."
    echo
    
    # List workflows
    local workflow_count=0
    shopt -s nullglob  # Handle cases where no files match
    for workflow_file in "$WORKFLOW_DIR"/*.yml "$WORKFLOW_DIR"/*.yaml; do
        if [[ -f "$workflow_file" ]]; then
            ((workflow_count++))
            local workflow_name=$(basename "$workflow_file")
            echo -e "${BOLD}${MAGENTA}📋 Workflow: ${workflow_name}${RESET}"
            
            # Use act to list jobs for this workflow
            echo -e "${CYAN}   Jobs:${RESET}"
            if act -W "$workflow_file" -l 2>/dev/null | grep -E "^Stage" | while read -r line; do
                job_info=$(echo "$line" | sed 's/Stage//')
                echo -e "     ${GREEN}•${RESET}${job_info}"
            done; then
                :
            else
                # Fallback: parse YAML manually if act fails
                if command -v yq &> /dev/null; then
                    yq '.jobs | keys[]' "$workflow_file" 2>/dev/null | while read -r job; do
                        echo -e "     ${GREEN}•${RESET} ${job//\"/}"
                    done
                else
                    grep -E "^\s{2}[a-zA-Z_-]+:" "$workflow_file" | sed 's/://' | awk '{print $1}' | while read -r job; do
                        echo -e "     ${GREEN}•${RESET} $job"
                    done
                fi
            fi
            echo
        fi
    done
    
    if [[ $workflow_count -eq 0 ]]; then
        print_warning "No workflow files found in $WORKFLOW_DIR"
    else
        print_success "Found $workflow_count workflow file(s)"
    fi
}

# Show workflow dependency graph
cmd_graph() {
    print_header "Workflow Dependency Graph"
    
    check_workflow_dir
    check_act_installed
    
    print_info "Generating dependency graph for all workflows..."
    print_warning "Note: In Replit, graphical output may be limited to text representation"
    echo
    
    # Use act to show graph
    if act --graph 2>/dev/null; then
        print_success "Graph generated successfully"
    else
        print_warning "Unable to generate graph with act, showing text representation..."
        echo
        
        # Alternative: show job dependencies manually
        shopt -s nullglob
        for workflow_file in "$WORKFLOW_DIR"/*.yml "$WORKFLOW_DIR"/*.yaml; do
            if [[ -f "$workflow_file" ]]; then
                local workflow_name=$(basename "$workflow_file")
                echo -e "${BOLD}${MAGENTA}Workflow: ${workflow_name}${RESET}"
                
                # Try to extract needs dependencies
                if command -v yq &> /dev/null; then
                    yq '.jobs | to_entries[] | select(.value.needs) | "\(.key) → \(.value.needs)"' "$workflow_file" 2>/dev/null | while read -r dep; do
                        echo "  ${dep//\"/}"
                    done
                else
                    grep -A 1 "needs:" "$workflow_file" | grep -v "^--$" | paste - - 2>/dev/null | while read -r line; do
                        job=$(echo "$line" | awk '{print $1}' | sed 's/://')
                        deps=$(echo "$line" | sed 's/.*needs: *//')
                        echo "  $job → $deps"
                    done
                fi
                echo
            fi
        done
    fi
}

# Validate workflow syntax
cmd_validate() {
    print_header "Validating Workflow Syntax"
    
    check_workflow_dir
    check_act_installed
    
    print_info "Running dry-run validation on all workflows..."
    echo
    
    local success_count=0
    local error_count=0
    
    shopt -s nullglob
    for workflow_file in "$WORKFLOW_DIR"/*.yml "$WORKFLOW_DIR"/*.yaml; do
        if [[ -f "$workflow_file" ]]; then
            local workflow_name=$(basename "$workflow_file")
            echo -n -e "${BOLD}Validating: ${workflow_name}${RESET} ... "
            
            # Use act with dryrun to validate
            if act -W "$workflow_file" --dryrun &>/dev/null; then
                echo -e "${GREEN}✓ Valid${RESET}"
                ((success_count++))
            else
                echo -e "${RED}✗ Invalid${RESET}"
                ((error_count++))
                
                # Show error details
                echo -e "${RED}  Error details:${RESET}"
                act -W "$workflow_file" --dryrun 2>&1 | grep -E "Error|error" | head -5 | while read -r error_line; do
                    echo "    $error_line"
                done
            fi
        fi
    done
    
    echo
    if [[ $error_count -eq 0 ]]; then
        print_success "All $success_count workflow(s) are valid!"
    else
        print_warning "Found $error_count invalid workflow(s) out of $((success_count + error_count)) total"
    fi
}

# List jobs triggered by push events
cmd_push() {
    print_header "Jobs Triggered by Push Events"
    
    check_workflow_dir
    
    print_info "Analyzing push event triggers..."
    echo
    
    local found_push=false
    
    shopt -s nullglob
    for workflow_file in "$WORKFLOW_DIR"/*.yml "$WORKFLOW_DIR"/*.yaml; do
        if [[ -f "$workflow_file" ]]; then
            # Check if workflow has push trigger
            if grep -q "^\s*push:" "$workflow_file" || grep -q "on:.*push" "$workflow_file"; then
                found_push=true
                local workflow_name=$(basename "$workflow_file")
                echo -e "${BOLD}${MAGENTA}📤 ${workflow_name}${RESET}"
                
                # Extract push trigger details
                echo -e "${CYAN}   Push Configuration:${RESET}"
                sed -n '/^\s*push:/,/^[^ ]/p' "$workflow_file" | head -n -1 | grep -E "branches|tags|paths" | while read -r config; do
                    echo "     $config"
                done
                
                # List jobs
                echo -e "${CYAN}   Jobs:${RESET}"
                if command -v yq &> /dev/null; then
                    yq '.jobs | keys[]' "$workflow_file" 2>/dev/null | while read -r job; do
                        echo -e "     ${GREEN}•${RESET} ${job//\"/}"
                    done
                else
                    grep -E "^\s{2}[a-zA-Z_-]+:" "$workflow_file" | sed 's/://' | awk '{print $1}' | while read -r job; do
                        echo -e "     ${GREEN}•${RESET} $job"
                    done
                fi
                echo
            fi
        fi
    done
    
    if [[ "$found_push" == "false" ]]; then
        print_warning "No workflows with push triggers found"
    fi
}

# List jobs triggered by pull requests
cmd_pull() {
    print_header "Jobs Triggered by Pull Request Events"
    
    check_workflow_dir
    
    print_info "Analyzing pull request event triggers..."
    echo
    
    local found_pr=false
    
    shopt -s nullglob
    for workflow_file in "$WORKFLOW_DIR"/*.yml "$WORKFLOW_DIR"/*.yaml; do
        if [[ -f "$workflow_file" ]]; then
            # Check if workflow has pull_request trigger
            if grep -q "^\s*pull_request:" "$workflow_file" || grep -q "on:.*pull_request" "$workflow_file"; then
                found_pr=true
                local workflow_name=$(basename "$workflow_file")
                echo -e "${BOLD}${MAGENTA}🔀 ${workflow_name}${RESET}"
                
                # Extract PR trigger details
                echo -e "${CYAN}   Pull Request Configuration:${RESET}"
                sed -n '/^\s*pull_request:/,/^[^ ]/p' "$workflow_file" | head -n -1 | grep -E "branches|types|paths" | while read -r config; do
                    echo "     $config"
                done
                
                # List jobs
                echo -e "${CYAN}   Jobs:${RESET}"
                if command -v yq &> /dev/null; then
                    yq '.jobs | keys[]' "$workflow_file" 2>/dev/null | while read -r job; do
                        echo -e "     ${GREEN}•${RESET} ${job//\"/}"
                    done
                else
                    grep -E "^\s{2}[a-zA-Z_-]+:" "$workflow_file" | sed 's/://' | awk '{print $1}' | while read -r job; do
                        echo -e "     ${GREEN}•${RESET} $job"
                    done
                fi
                echo
            fi
        fi
    done
    
    if [[ "$found_pr" == "false" ]]; then
        print_warning "No workflows with pull request triggers found"
    fi
}

# Analyze specific job details
cmd_job() {
    local job_name="${1:-}"
    
    if [[ -z "$job_name" ]]; then
        print_error "Job name is required"
        echo "Usage: $SCRIPT_NAME job <job-name>"
        exit 1
    fi
    
    print_header "Analyzing Job: $job_name"
    
    check_workflow_dir
    
    print_info "Searching for job '$job_name' in all workflows..."
    echo
    
    local found_job=false
    
    shopt -s nullglob
    for workflow_file in "$WORKFLOW_DIR"/*.yml "$WORKFLOW_DIR"/*.yaml; do
        if [[ -f "$workflow_file" ]]; then
            # Check if job exists in this workflow
            if grep -q "^\s\s${job_name}:" "$workflow_file"; then
                found_job=true
                local workflow_name=$(basename "$workflow_file")
                echo -e "${BOLD}${MAGENTA}Found in: ${workflow_name}${RESET}"
                echo
                
                # Extract job details
                if command -v yq &> /dev/null; then
                    echo -e "${CYAN}Job Configuration:${RESET}"
                    yq ".jobs.${job_name}" "$workflow_file" 2>/dev/null | head -50
                else
                    echo -e "${CYAN}Job Configuration:${RESET}"
                    sed -n "/^\s\s${job_name}:/,/^\s\s[a-zA-Z_-]*:/p" "$workflow_file" | head -n -1 | head -50
                fi
                
                echo
                
                # Show job metadata
                echo -e "${CYAN}Job Metadata:${RESET}"
                
                # Check for runs-on
                if grep -q "runs-on:" <(sed -n "/^\s\s${job_name}:/,/^\s\s[a-zA-Z_-]*:/p" "$workflow_file"); then
                    local runs_on=$(grep "runs-on:" <(sed -n "/^\s\s${job_name}:/,/^\s\s[a-zA-Z_-]*:/p" "$workflow_file") | head -1 | sed 's/.*runs-on: *//')
                    echo -e "  ${GREEN}•${RESET} Runs on: $runs_on"
                fi
                
                # Check for needs (dependencies)
                if grep -q "needs:" <(sed -n "/^\s\s${job_name}:/,/^\s\s[a-zA-Z_-]*:/p" "$workflow_file"); then
                    local needs=$(grep "needs:" <(sed -n "/^\s\s${job_name}:/,/^\s\s[a-zA-Z_-]*:/p" "$workflow_file") | head -1 | sed 's/.*needs: *//')
                    echo -e "  ${GREEN}•${RESET} Depends on: $needs"
                fi
                
                # Count steps
                local step_count=$(sed -n "/^\s\s${job_name}:/,/^\s\s[a-zA-Z_-]*:/p" "$workflow_file" | grep -c "^\s\s\s\s\s\s- " || true)
                if [[ $step_count -gt 0 ]]; then
                    echo -e "  ${GREEN}•${RESET} Number of steps: $step_count"
                fi
                
                echo
            fi
        fi
    done
    
    if [[ "$found_job" == "false" ]]; then
        print_error "Job '$job_name' not found in any workflow"
        echo
        print_info "Available jobs:"
        shopt -s nullglob
        for workflow_file in "$WORKFLOW_DIR"/*.yml "$WORKFLOW_DIR"/*.yaml; do
            if [[ -f "$workflow_file" ]]; then
                local workflow_name=$(basename "$workflow_file")
                echo -e "  ${BOLD}${workflow_name}:${RESET}"
                grep -E "^\s{2}[a-zA-Z_-]+:" "$workflow_file" | sed 's/://' | awk '{print $1}' | while read -r job; do
                    echo "    • $job"
                done
            fi
        done
    fi
}

# Show help/usage information
cmd_help() {
    cat << EOF
${BOLD}GitHub Actions Workflow Testing Script${RESET}

${CYAN}Usage:${RESET}
  $SCRIPT_NAME <command> [options]

${CYAN}Commands:${RESET}
  ${GREEN}list${RESET}       List all workflows and their jobs
  ${GREEN}graph${RESET}      Show workflow dependency graph
  ${GREEN}validate${RESET}   Validate workflow syntax using --dryrun
  ${GREEN}push${RESET}       List jobs triggered by push events
  ${GREEN}pull${RESET}       List jobs triggered by pull requests
  ${GREEN}job${RESET}        Analyze specific job details
             Usage: $SCRIPT_NAME job <job-name>
  ${GREEN}help${RESET}       Show this help message

${CYAN}Examples:${RESET}
  $SCRIPT_NAME list               # List all workflows and jobs
  $SCRIPT_NAME validate           # Check syntax of all workflows
  $SCRIPT_NAME push               # Show push-triggered jobs
  $SCRIPT_NAME pull               # Show PR-triggered jobs
  $SCRIPT_NAME job test           # Analyze job named 'test'
  $SCRIPT_NAME graph              # Display dependency graph

${YELLOW}Note:${RESET}
  This script provides static analysis of GitHub Actions workflows.
  Actual workflow execution is not possible in Replit environment
  due to lack of Docker support. Use this tool for validation and
  analysis before pushing to GitHub.

${CYAN}Requirements:${RESET}
  • act tool must be installed
  • Workflows must be in .github/workflows/
  • Run from repository root directory

EOF
}

# Main command handler
main() {
    local command="${1:-help}"
    
    case "$command" in
        list)
            cmd_list
            ;;
        graph)
            cmd_graph
            ;;
        validate)
            cmd_validate
            ;;
        push)
            cmd_push
            ;;
        pull)
            cmd_pull
            ;;
        job)
            shift
            cmd_job "$@"
            ;;
        help|--help|-h)
            cmd_help
            ;;
        *)
            print_error "Unknown command: $command"
            echo
            cmd_help
            exit 1
            ;;
    esac
}

# Run main function with all arguments
main "$@"