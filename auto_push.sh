#!/bin/bash

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Change to the repository directory
cd "$(dirname "$0")"

# Initialize git if not already initialized
if [ ! -d .git ]; then
    git init
    git add .
    git commit -m "Initial commit"
    git branch -M main
    git remote add origin "https://github.com/siwanetzu/Sprites-Editor.git"
    git push -u origin main
fi

echo -e "${GREEN}Starting auto-push script for Sprites Editor${NC}"
echo -e "${YELLOW}Press Ctrl+C to stop the script${NC}"

while true; do
    # Get current timestamp
    TIMESTAMP=$(date +"%Y-%m-%d %H:%M:%S")
    
    # Check if there are any changes
    if [[ $(git status --porcelain) ]]; then
        echo -e "\n${GREEN}Changes detected at $TIMESTAMP${NC}"
        
        # Add all changes
        git add .
        
        # Commit with timestamp
        git commit -m "Auto-commit: $TIMESTAMP"
        
        # Push to remote
        git push origin main
        echo -e "${GREEN}Push complete${NC}"
    else
        echo -e "${YELLOW}No changes detected at $TIMESTAMP${NC}"
    fi
    
    # Wait for 2 minutes
    sleep 120
done 