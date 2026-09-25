#!/bin/bash
# setup-git.sh - Initialize git repo and push to GitHub

set -e

REPO_NAME="TaizouInjector"
GITHUB_USER=""  # Set your GitHub username
VISIBILITY="private"  # or "public"

echo "🚀 Setting up Git repository for $REPO_NAME"

# Check if git is available
if ! command -v git &> /dev/null; then
    echo "❌ Git not found. Install git first."
    exit 1
fi

# Check if gh CLI is available
if ! command -v gh &> /dev/null; then
    echo "⚠️  GitHub CLI (gh) not found. You'll need to create repo manually."
    echo "   Install: https://cli.github.com/"
    USE_GH=false
else
    USE_GH=true
fi

# Initialize git if not already
if [ ! -d ".git" ]; then
    echo "📁 Initializing git repository..."
    git init
    git branch -M main
else
    echo "✅ Git already initialized"
fi

# Create .gitignore if not exists
if [ ! -f ".gitignore" ]; then
    cat > .gitignore << 'EOF'
# Gradle
.gradle/
build/
*/build/
*.iml
.idea/
*.log

# Local config
local.properties
keystore.jks
*.keystore

# OS
.DS_Store
Thumbs.db

# Android
*.apk
*.aar
*.ap_
*.dex

# Native
*.so
*.o
*.obj

# IDE
*.swp
*.swo
*~

# Cache
*.cache
.caches/
EOF
    echo "📝 Created .gitignore"
fi

# Add all files
echo "📦 Adding files..."
git add .

# Check if there are changes to commit
if git diff --cached --quiet; then
    echo "ℹ️  No changes to commit"
else
    git commit -m "feat: Initial commit - TaizouInjector Android port

- Kotlin UI with ViewPager2 cheat menu (6 tabs)
- C++17 native core for memory patching (libunity.so, libanogs.so)
- JNI bridge for native operations
- Floating SYSTEM_ALERT_WINDOW overlay with drag support
- Config persistence (JSON + SharedPreferences)
- 50+ ARM64 native ELF binaries for skin patches
- Custom animations: analog clock, ECG wave, RGB toasts
- TTS announcements, anti-debug checks
- GitHub Actions CI/CD with release automation"
    echo "✅ Committed initial changes"
fi

# Create GitHub repo and push
if [ "$USE_GH" = true ]; then
    echo "🔐 Checking GitHub authentication..."
    if gh auth status &> /dev/null; then
        echo "✅ GitHub CLI authenticated"
        
        echo "📤 Creating private repository..."
        gh repo create "$REPO_NAME" --"$VISIBILITY" --source=. --push --description "TaizouInjector - Modern Android port of CODM injector (ALP)"
        
        echo "🎉 Repository created and pushed!"
        echo "🔗 https://github.com/$(gh api user --jq .login)/$REPO_NAME"
        
        # Setup branch protection (optional)
        echo "🛡️  Setting up branch protection..."
        gh api repos/$(gh api user --jq .login)/$REPO_NAME/branches/main/protection \
            --method PUT \
            -f required_status_checks='{"strict":true,"contexts":["Build APK (debug)","Build APK (release)"]}' \
            -f enforce_admins=false \
            -f required_pull_request_reviews='{"required_approving_review_count":1}' \
            -f restrictions=null 2>/dev/null || echo "   (Branch protection requires admin/repo settings access)"
    else
        echo "❌ GitHub CLI not authenticated. Run: gh auth login"
        echo "   Then run this script again or push manually:"
        echo "   git remote add origin https://github.com/$GITHUB_USER/$REPO_NAME.git"
        echo "   git push -u origin main"
    fi
else
    echo ""
    echo "📋 Manual steps to push to GitHub:"
    echo "   1. Create private repo at: https://github.com/new"
    echo "      - Name: $REPO_NAME"
    echo "      - Private: ✓"
    echo "      - Don't initialize with README/license/.gitignore"
    echo "   2. Run:"
    echo "      git remote add origin https://github.com/$GITHUB_USER/$REPO_NAME.git"
    echo "      git push -u origin main"
fi

echo ""
echo "🔧 Next steps for CI/CD:"
echo "   1. Go to repo Settings → Secrets and variables → Actions"
echo "   2. Add secrets:"
echo "      - KEYSTORE_BASE64 (base64 of keystore.jks)"
echo "      - KEYSTORE_PASSWORD"
echo "      - KEY_ALIAS (e.g., taizou)"
echo "      - KEY_PASSWORD"
echo "   3. Create keystore if needed:"
echo "      keytool -genkeypair -v -keystore keystore.jks -keyalg RSA -keysize 2048 -validity 10000 -alias taizou"
echo "      base64 -w0 keystore.jks  # Copy to KEYSTORE_BASE64"
echo "   4. Push a tag to trigger release:"
echo "      git tag v1.6.57 && git push origin v1.6.57"