#!/bin/bash
# Commit script for toxvpn2 changes

cd /media/mike/BigData/Nyrds/qwen_sandbox/toxvpn

echo "=== Checking current status ==="
git status

echo "=== Adding all changed files ==="
git add .
git add -u

echo "=== Showing files to be committed ==="
git status

echo "=== Committing changes ==="
git commit -m "feat: Add TCP relay access control and pre-created TUN interface support

- Implement TCP relay whitelist functionality with 5 new commands:
  * tcp_acl_enable/disable
  * tcp_acl_add/remove
  * tcp_acl_status
- Add -t <device> flag to use pre-created TUN interfaces
- Enable running without full sudo privileges
- Update all platform implementations (Linux/Mac/Windows)
- Rename binary from toxvpn to toxvpn2
- Add comprehensive documentation and tests"

echo "=== Commit completed ==="
echo "Files modified:"
git diff --name-only HEAD~1