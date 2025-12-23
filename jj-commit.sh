#!/bin/bash
set -e

STATUS=$(jj status)
SHOW=$(jj show)

RESPONSE=$(curl -s https://api.anthropic.com/v1/messages \
  -H "Content-Type: application/json" \
  -H "x-api-key: $MY_ANTHROPIC_API_KEY" \
  -H "anthropic-version: 2023-06-01" \
  -d "$(jq -n --arg status "$STATUS" --arg show "$SHOW" '{
    model: "claude-haiku-4-5",
    max_tokens: 100,
    messages: [{
      role: "user",
      content: "Generate a brief one-line commit message (no quotes, no prefix like feat/fix) for these changes:\n\nStatus:\n\($status)\n\nDiff:\n\($show)"
    }]
  }')")

MESSAGE=$(echo "$RESPONSE" | jq -r '.content[0].text')
if [ "$MESSAGE" = "null" ] || [ -z "$MESSAGE" ]; then
  echo "API Error: $RESPONSE"
  exit 1
fi

echo "Commit message: $MESSAGE"
read -p "Continue? (y/n) " -n 1 -r
echo
[[ $REPLY =~ ^[Yy]$ ]] || exit 1
jj describe -m "$MESSAGE"
jj new -m "wip"
echo "Done"
