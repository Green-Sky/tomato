# taken from llama.cpp
# needs BRANCH_NAME set
SHORT_HASH="$(git rev-parse --short=7 HEAD)"
DEPTH="$(git rev-list --count HEAD)"
if [[ "${BRANCH_NAME}" == "master" ]]; then
	echo "name=dev-${DEPTH}-${SHORT_HASH}" >> $GITHUB_OUTPUT
else
	SAFE_NAME=$(echo "${BRANCH_NAME}" | tr '/' '-')
	echo "name=dev-${SAFE_NAME}-${SHORT_HASH}" >> $GITHUB_OUTPUT
fi
