#!/bin/bash

# This script replaces the placeholder in app.js with the actual API key from environment variables.
# It is intended to run during the Vercel build process.

if [ -z "$FIREBASE_API_KEY" ]; then
  echo "Error: FIREBASE_API_KEY environment variable is not set."
  exit 1
fi

sed -i "s/__FIREBASE_API_KEY__/$FIREBASE_API_KEY/g" app.js

echo "API key successfully injected into app.js."
